/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: logview.c - Debug log viewer app
 */

#include <gui.h>

enum {
    CHAR_WIDTH = 5,
    CHAR_HEIGHT = 8,

    TEXT_COLS = DEBUG_BUFFER_COLS,
    TEXT_ROWS = DEBUG_BUFFER_ROWS - 1,
    TEXT_PAD = 2,
    TEXT_X = 1 + TEXT_PAD,
    TEXT_Y = TITLE_BAR_HEIGHT + TEXT_PAD,
    TEXT_WIDTH = CHAR_WIDTH * TEXT_COLS,
    TEXT_HEIGHT = CHAR_HEIGHT * TEXT_ROWS,

    WINDOW_WIDTH = TEXT_X + TEXT_WIDTH + TEXT_PAD + 1,
    WINDOW_HEIGHT = TEXT_Y + TEXT_HEIGHT + TEXT_PAD + 1,
};

typedef struct {
    uint8_t window_pixels[WINDOW_WIDTH * WINDOW_HEIGHT];
    surface_st window_surface;
    window_st window;

    widget_st title_bar;
    widget_st close_button;
    widget_st *widgets[2];

    uint32_t last_buffer_gen;
} app_state_st;

static app_state_st *app_state = NULL;

static void
draw_logs(void)
{
    app_state_st *a = app_state;
    int row, col;
    char c;
    uint32_t gen = krn_debug_buffer_gen;
    rect_st rect;

    for (row = 0; row < TEXT_ROWS; ++row) {
        for (col = 0; col < TEXT_COLS; ++col) {
            c = krn_debug_buffer[row][col];

            if (c < 0x20) {
                c = ' ';
            }

            gui_surface_draw_char(a->window.surface,
                TEXT_X + col * CHAR_WIDTH,
                TEXT_Y + row * CHAR_HEIGHT,
                font_5x8, c,
                COLOR_WIDGET_FG, COLOR_WIDGET_BG
            );
        }
    }

    rect = gui_rect_make(TEXT_X, TEXT_Y, TEXT_WIDTH, TEXT_HEIGHT);
    gui_wm_render_window_region(&a->window, rect);

    a->last_buffer_gen = gen;
}

static void
draw_window(window_st *window)
{
    gui_window_draw(window, COLOR_WIDGET_BG);
    draw_logs();
}

static void
on_tick(window_st *window)
{
    if (!window->visible) {
        return;
    }

    if (krn_debug_buffer_gen != app_state->last_buffer_gen) {
        draw_logs();
    }
}

static void
close_window(window_st *window)
{
    gui_wm_remove_window(window);
    app_logview.main_window = NULL;

    heap_free(app_state);
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
    a->window.title = "Log Viewer";
    a->window.widgets = a->widgets;
    a->window.widgets_capacity = sizeof(a->widgets) / sizeof(a->widgets[0]);
    a->window.draw = draw_window;
    a->window.on_tick = on_tick;
    a->window.on_close = close_window;

    gui_window_init_frame(&a->window, &a->title_bar, &a->close_button);
}

static int
init_app(void)
{
    ASSERT(!app_state);

    app_state = heap_alloc(sizeof(app_state_st), "Log Viewer", 0);

    if (!app_state) {
        return E_NOT_ENOUGH_MEMORY;
    }

    init_window();

    app_logview.main_window = &app_state->window;

    return E_OK;
}

global app_st app_logview = {
    .init = init_app,
};
