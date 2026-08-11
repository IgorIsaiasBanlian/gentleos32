/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: player.c - Music player app
 */

#include <gui.h>

enum {
    PADDING = 8,

    CONTENT_X = PADDING,
    CONTENT_Y = TITLE_BAR_HEIGHT + PADDING - 1,
    CONTENT_WIDTH = 200,
    CONTENT_HEIGHT = 120,

    WINDOW_WIDTH = CONTENT_X + CONTENT_WIDTH + PADDING,
    WINDOW_HEIGHT = CONTENT_Y + CONTENT_HEIGHT + PADDING,
};

typedef struct {
    uint8_t window_pixels[WINDOW_WIDTH * WINDOW_HEIGHT];
    surface_st window_surface;
    window_st window;

    widget_st title_bar;
    widget_st close_button;
    widget_st *widgets[2];
} app_state_st;

static app_state_st *app_state = NULL;

static void
draw_window(window_st *window)
{
    gui_window_draw(window, COLOR_WIDGET_BG);
}

static void
close_window(window_st *window)
{
    gui_wm_remove_window(window);
    app_player.main_window = NULL;

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
    a->window.title = "Player";
    a->window.widgets = a->widgets;
    a->window.widgets_capacity = sizeof(a->widgets) / sizeof(a->widgets[0]);
    a->window.draw = draw_window;
    a->window.on_close = close_window;

    gui_window_init_frame(&a->window, &a->title_bar, &a->close_button);
}

static int
init_app(void)
{
    ASSERT(!app_state);

    app_state = heap_alloc(sizeof(app_state_st), "Player app", 0);

    if (!app_state) {
        return E_NOT_ENOUGH_MEMORY;
    }

    init_window();

    app_player.main_window = &app_state->window;

    return E_OK;
}

global app_st app_player = {
    .init = init_app,
};
