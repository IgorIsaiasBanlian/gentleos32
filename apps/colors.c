/*
 * Copyright (c) 2025-2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: colors.c - Color palette app
 */

#include <gui.h>

enum {
    GRID_CELL_WIDTH = 32,
    GRID_CELL_HEIGHT = 32,
    GRID_ROWS = 4,
    GRID_COLS = 4,
    GRID_CELLS_COUNT = (GRID_ROWS * GRID_COLS),
    GRID_WIDTH = GRID_WIDTH_SPACED(GRID_CELL_WIDTH, GRID_COLS),
    GRID_HEIGHT = GRID_HEIGHT_SPACED(GRID_CELL_HEIGHT, GRID_ROWS),
    GRID_X = 1,
    GRID_Y = TITLE_BAR_HEIGHT,

    WINDOW_WIDTH = GRID_X + GRID_WIDTH + 1,
    WINDOW_HEIGHT = GRID_Y + GRID_HEIGHT + 1,
};

typedef struct {
    uint8_t window_pixels[WINDOW_WIDTH * WINDOW_HEIGHT];
    surface_st window_surface;
    window_st window;

    widget_st title_bar;
    widget_st close_button;

    widget_st color_buttons[GRID_CELLS_COUNT];
    widget_st *active_color_button;

    widget_st *widgets[GRID_CELLS_COUNT + 2];

    grid_st grid;
} app_state_st;

static app_state_st *app_state = NULL;

static void
update_status(void)
{
    app_state_st *a = app_state;

    if (!a->active_color_button) {
        gui_status_set("");
        return;
    };

    gui_status_set("Hex:%02x Dec:%d", a->active_color_button->tag2,
        a->active_color_button->tag2);
}
static void
on_color_button_press(widget_st *widget, event_st event _unsd, point_st pos _unsd)
{
    app_state_st *a = app_state;

    widget_st *prev_active_color_button = a->active_color_button;

    a->active_color_button = widget;

    if (a->active_color_button == prev_active_color_button) {
        return;
    }

    if (prev_active_color_button) {
        gui_widget_draw(prev_active_color_button);
    }

    gui_widget_draw(a->active_color_button);

    update_status();
}

static void
draw_color_button(widget_st *widget)
{
    app_state_st *a = app_state;

    rect_st rect = widget->rect;

    if (widget == a->active_color_button) {
        gui_surface_draw_rect(widget->window->surface, rect, COLOR_BORDER);
        gui_surface_draw_rect(widget->window->surface, gui_rect_shrink(rect, 1),
            widget->tag2);
    } else {
        gui_surface_draw_rect(widget->window->surface, rect, widget->tag2);
    }

    gui_wm_render_window_region(widget->window, rect);
}

static void
draw_window(window_st *window)
{
    gui_window_draw(window, COLOR_BORDER);
}

static void
on_active_change(window_st *window)
{
    if (window->active) {
        update_status();
    }
}

static void
close_window(window_st *window _unsd)
{
    gui_wm_remove_window(window);
    app_colors.main_window = NULL;

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
    a->window.title = "Colors";
    a->window.widgets = a->widgets;
    a->window.widgets_capacity = sizeof(a->widgets) / sizeof(a->widgets[0]);
    a->window.draw = draw_window;
    a->window.on_active_change = on_active_change;
    a->window.on_close = close_window;

    gui_window_init_frame(&a->window, &a->title_bar, &a->close_button);
}

static void
init_color_buttons(void)
{
    app_state_st *a = app_state;

    a->grid.cell_width = GRID_CELL_WIDTH;
    a->grid.cell_height = GRID_CELL_HEIGHT;
    a->grid.cols = GRID_COLS;
    a->grid.rows = GRID_ROWS;
    a->grid.x = GRID_X;
    a->grid.y = GRID_Y;

    for (size_t i = 0; i < GRID_CELLS_COUNT; i++) {
        int col = i % a->grid.cols;
        int row = i / a->grid.cols;

        a->color_buttons[i].rect = gui_grid_cell_rect(&a->grid, col, row);
        a->color_buttons[i].tag2 = i;
        a->color_buttons[i].window = &a->window;
        a->color_buttons[i].draw = draw_color_button;
        a->color_buttons[i].on_pointer_down = on_color_button_press;
        a->color_buttons[i].press_on_move_in = 1;

        gui_window_add_widget(&a->window, &a->color_buttons[i]);
    };

    a->active_color_button = &a->color_buttons[0];
}

static int
init_app(void)
{
    ASSERT(!app_state);

    app_state = krn_heap_alloc(sizeof(app_state_st), "Colors app", 0);

    if (!app_state) {
        return E_NOT_ENOUGH_MEMORY;
    }

    init_window();
    init_color_buttons();

    app_colors.main_window = &app_state->window;

    return E_OK;
}

global app_st app_colors = {
    .icon = &icon_colors,
    .init = init_app,
};
