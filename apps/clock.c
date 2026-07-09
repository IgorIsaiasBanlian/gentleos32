/*
 * Copyright (c) 2025-2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: clock.c - Clock app
 */

#include <gui.h>

enum {
    GRID_PAD = 1,
    GRID_CELL_WIDTH = 8,
    GRID_CELL_HEIGHT = 8,
    GRID_COLS = 29,
    GRID_ROWS = 7,
    GRID_X = 1 + GRID_PAD,
    GRID_Y = TITLE_BAR_HEIGHT + GRID_PAD,
    GRID_WIDTH = GRID_WIDTH_SPACED(GRID_CELL_WIDTH, GRID_COLS),
    GRID_HEIGHT = GRID_HEIGHT_SPACED(GRID_CELL_HEIGHT, GRID_ROWS),

    REFRESH_FREQUENCY = 3,
    REFRESH_TICKS = TICK_FREQUENCY / REFRESH_FREQUENCY,

    WINDOW_WIDTH = GRID_X + GRID_WIDTH + GRID_PAD + 1,
    WINDOW_HEIGHT = GRID_Y + GRID_HEIGHT + GRID_PAD + 1,
};

typedef struct {
    uint8_t window_pixels[WINDOW_WIDTH * WINDOW_HEIGHT];
    surface_st window_surface;
    window_st window;

    widget_st title_bar;
    widget_st close_button;
    widget_st *widgets[2];

    grid_st grid;

    time_st last_time;
} app_state_st;

static app_state_st *app_state = NULL;

static void
draw_cell(int x, int y, int active)
{
    app_state_st *a = app_state;

    rect_st r = gui_grid_cell_rect(&a->grid, x, y);
    uint8_t color = active ? COLOR_WIDGET_FG : COLOR_WIDGET_BG;
    gui_surface_draw_rect(a->window.surface, r, color);
    gui_wm_render_window_region(&a->window, r);
}

static void
draw_digit(int x, int y, int digit)
{
    static const uint16_t digit_pixels[10] = {
        0xf6de, 0x592e, 0xe7ce, 0xe79e, 0xb792,
        0xf39e, 0xf3de, 0xe492, 0xf7de, 0xf79e,
    };

    for (int i = 0; i < 15; ++i) {
        uint8_t active = !!(digit_pixels[digit] & (1 << (15 - i)));
        draw_cell(x + i % 3, y + i / 3, active);
    }
}

static void
draw_time(void)
{
    app_state_st *a = app_state;
    time_st t;
    krn_rtc_get_time(&t);

    if (krn_rtc_are_times_equal(&t, &a->last_time)) {
        return;
    }

    a->last_time = t;

    draw_digit(1, 1, t.hour / 10);
    draw_digit(5, 1, t.hour % 10);
    draw_cell(9, 2, 1);
    draw_cell(9, 4, 1);
    draw_digit(11, 1, t.minute / 10);
    draw_digit(15, 1, t.minute % 10);
    draw_cell(19, 2, 1);
    draw_cell(19, 4, 1);
    draw_digit(21, 1, t.second / 10);
    draw_digit(25, 1, t.second % 10);
}

static void
draw_window(window_st *window)
{
    gui_window_draw(window, COLOR_WIDGET_BG);
    draw_time();
}

static void
on_tick(window_st *window _unsd)
{
    static unsigned count = 0;

    if (!window->visible) {
        return;
    }

    ++count;

    if (count < REFRESH_TICKS) {
        return;
    }

    count = 0;
    draw_time();
}

static void
close_window(window_st *window _unsd)
{
    gui_wm_remove_window(window);
    app_clock.main_window = NULL;

    krn_heap_free(app_state);
    app_state = NULL;
}

static void
init_window(void)
{
    app_state_st *a = app_state;

    a->window_surface.size.width = WINDOW_WIDTH;
    a->window_surface.size.height = WINDOW_HEIGHT;
    a->window_surface.pitch = WINDOW_WIDTH;
    a->window_surface.pixels = a->window_pixels;

    a->window.surface = &a->window_surface;
    a->window.title = "Clock";
    a->window.widgets = a->widgets;
    a->window.widgets_capacity = sizeof(a->widgets) / sizeof(a->widgets[0]);
    a->window.draw = draw_window;
    a->window.on_tick = on_tick;
    a->window.on_close = close_window;

    gui_window_init_frame(&a->window, &a->title_bar, &a->close_button);
}

static void
init_grid(void)
{
    app_state_st *a = app_state;

    a->grid.cell_width = GRID_CELL_WIDTH;
    a->grid.cell_height = GRID_CELL_HEIGHT;
    a->grid.cols = GRID_COLS;
    a->grid.rows = GRID_ROWS;
    a->grid.x = GRID_X;
    a->grid.y = GRID_Y;
}

static int
init_app(void)
{
    ASSERT(!app_state);

    app_state = krn_heap_alloc(sizeof(app_state_st), "Clock app", 0);

    if (!app_state) {
        return E_NOT_ENOUGH_MEMORY;
    }

    init_window();
    init_grid();

    app_clock.main_window = &app_state->window;

    return E_OK;
}

global app_st app_clock = {
    .icon = &icon_clock,
    .init = init_app,
};
