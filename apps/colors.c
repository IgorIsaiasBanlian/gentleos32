/*
 * Copyright (c) 2025-2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: colors.c - Color palette app
 */

#include <gui.h>

static surface_st window_surface;
static window_st window;

static size_t color_buttons_count;
static widget_st *color_buttons;
static widget_st *active_color_button;

static widget_st close_button;
static widget_st title_bar;

static grid_st grid;

static void
update_status(void)
{
    if (!active_color_button) {
        gui_status_set("");
        return;
    };

    gui_status_set("Hex:%02x Dec:%d", active_color_button->tag2,
        active_color_button->tag2);
}

static void
on_color_button_press(widget_st *widget, event_st event _unsd, point_st pos _unsd)
{
    widget_st *prev_active_color_button = active_color_button;

    active_color_button = widget;

    if (active_color_button == prev_active_color_button) {
        return;
    }

    if (prev_active_color_button) {
        gui_widget_draw(prev_active_color_button);
    }

    gui_widget_draw(active_color_button);

    update_status();
}

static void
draw_color_button(widget_st *widget)
{
    rect_st rect = widget->rect;

    if (widget == active_color_button) {
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
init_grid(void)
{
    system_info_st *si = &krn_system_info;

    if (si->fb_bpp == 8) {
        grid.cell_width = 18;
        grid.cell_height = 18;
        grid.cols = 16;
        grid.rows = 16;
    } else {
        grid.cell_width = 32;
        grid.cell_height = 32;
        grid.cols = 4;
        grid.rows = 4;
    }

    grid.x = 1;
    grid.y = TITLE_BAR_HEIGHT;
}

static void
init_window(void)
{
    int grid_width = GRID_WIDTH_SPACED(grid.cell_width, grid.cols);
    int grid_height = GRID_HEIGHT_SPACED(grid.cell_height, grid.rows);
    int width = grid.x + grid_width + 1;
    int height = grid.y + grid_height + 1;

    color_buttons_count = grid.cols * grid.rows;

    window_surface.size.width = width;
    window_surface.size.height = height;
    window_surface.pitch = width;
    window_surface.pixels = krn_heap_alloc(width * height, "Colors pixels", 1);

    window.surface = &window_surface;
    window.title = "Colors";
    window.widgets_capacity = color_buttons_count + 2;
    window.widgets = krn_heap_alloc(sizeof(widget_st *) * window.widgets_capacity,
        "Colors widgets", 1);
    window.draw = draw_window;
    window.on_active_change = on_active_change;

    gui_window_init_frame(&window, &title_bar, &close_button);
}

static void
init_color_buttons(void)
{
    color_buttons = krn_heap_alloc(sizeof(widget_st) * color_buttons_count,
        "Colors buttons", 1);
    active_color_button = &color_buttons[0];

    for (int i = 0; i < (grid.cols * grid.rows); i++) {
        int col = i % grid.cols;
        int row = i / grid.cols;

        color_buttons[i].rect = gui_grid_cell_rect(&grid, col, row);
        color_buttons[i].tag2 = i;
        color_buttons[i].window = &window;
        color_buttons[i].draw = draw_color_button;
        color_buttons[i].on_pointer_down = on_color_button_press;
        color_buttons[i].press_on_move_in = 1;

        gui_window_add_widget(&window, &color_buttons[i]);
    };
}

static void
init_app(void)
{
    init_grid();
    init_window();
    init_color_buttons();
}

static void
show_app(void)
{
    (void)gui_wm_add_window(&window);
}

global app_st app_colors = {
    .icon = &icon_colors,
    .init = init_app,
    .show = show_app,
};
