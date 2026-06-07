// --------------------------------------------------------------------------------------
// Copyright (c) 2025-2026 luke8086
// Distributed under the terms of GPL-2 License
// --------------------------------------------------------------------------------------
// File: clock.c - Clock app
// --------------------------------------------------------------------------------------

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

static uint8_t window_pixels[WINDOW_WIDTH * WINDOW_HEIGHT];
static surface_st window_surface;
static window_st window;

static widget_st title_bar;
static widget_st close_button;
static widget_st *widgets[2];

static grid_st grid;

static time_st last_time;

static void
draw_cell(int x, int y, int active)
{
    rect_st r = gui_grid_cell_rect(&grid, x, y);
    uint8_t color = active ? COLOR_WIDGET_FG : COLOR_WIDGET_BG;
    gui_surface_draw_rect(window.surface, r, color);
    gui_wm_render_window_region(&window, r);
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
    time_st t;
    krn_rtc_get_time(&t);

    if (krn_rtc_are_times_equal(&t, &last_time)) {
        return;
    }

    last_time = t;

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
init_window(void)
{
    window_surface.size.width = WINDOW_WIDTH;
    window_surface.size.height = WINDOW_HEIGHT;
    window_surface.pitch = WINDOW_WIDTH;
    window_surface.pixels = window_pixels;

    window.surface = &window_surface;
    window.title = "Clock";
    window.bg_color = COLOR_WIDGET_BG;
    window.widgets = widgets;
    window.widgets_capacity = sizeof(widgets) / sizeof(widgets[0]);
    window.on_tick = on_tick;

    gui_window_init_frame(&window, &title_bar, &close_button);
}

static void
init_grid(void)
{
    grid.cell_width = GRID_CELL_WIDTH;
    grid.cell_height = GRID_CELL_HEIGHT;
    grid.cols = GRID_COLS;
    grid.rows = GRID_ROWS;
    grid.x = GRID_X;
    grid.y = GRID_Y;
}

static void
init_app(void)
{
    init_window();
    init_grid();
    draw_time();
}

static void
show_app(void)
{
    (void)gui_wm_add_window(&window);
}

global app_st app_clock = {
    .icon = &icon_clock,
    .init = init_app,
    .show = show_app,
};
