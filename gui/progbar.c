/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: progbar.c - Progress bar widget
 */

#include <gui.h>

global void
gui_progress_bar_init(progress_bar_st *bar, int max_value, int cur_value)
{
    bar->_last_fill_width = 0;

    bar->widget.data = bar;
    bar->widget.draw = gui_progress_bar_draw;
    bar->widget.on_pointer_down = gui_progress_bar_on_pointer_down;

    gui_progress_bar_set_values(bar, max_value, cur_value);
}

static rect_st
gui_progress_bar_inner_rect(progress_bar_st *bar)
{
    return gui_rect_shrink(bar->widget.rect, 1);
}

static int
gui_progress_bar_fill_width(progress_bar_st *bar, int total_width)
{
    int width;

    if (bar->max_value <= 0 || total_width <= 0) {
        return 0;
    }

    width = (int)((uint32_t)total_width * (uint32_t)bar->cur_value / (uint32_t)bar->max_value);

    return MAX(0, MIN(total_width, width));
}

static void
gui_progress_bar_update_fill(progress_bar_st *bar)
{
    rect_st inner = gui_progress_bar_inner_rect(bar);
    int width = gui_progress_bar_fill_width(bar, inner.width);
    int prev = bar->_last_fill_width;
    rect_st strip = inner;

    if (width == prev || !bar->widget.window || bar->widget.hidden) {
        return;
    }

    strip.x = inner.x + MIN(width, prev);
    strip.width = MAX(width, prev) - MIN(width, prev);

    gui_surface_draw_rect(bar->widget.window->surface, strip,
        width > prev ? COLOR_BORDER : COLOR_WIDGET_BG);

    gui_wm_render_window_region(bar->widget.window, strip);

    bar->_last_fill_width = width;
}

global void
gui_progress_bar_draw(widget_st *widget)
{
    progress_bar_st *bar = widget->data;
    window_st *window = widget->window;
    surface_st *surface;
    rect_st inner = gui_progress_bar_inner_rect(bar);
    rect_st fill = inner;
    rect_st rest = inner;

    if (!window || widget->hidden) {
        return;
    }

    surface = window->surface;

    fill.width = gui_progress_bar_fill_width(bar, inner.width);
    rest.x += fill.width;
    rest.width = inner.width - fill.width;

    gui_surface_draw_border(surface, widget->rect, COLOR_BORDER);
    gui_surface_draw_rect(surface, fill, COLOR_BORDER);
    gui_surface_draw_rect(surface, rest, COLOR_WIDGET_BG);

    gui_wm_render_window_region(window, widget->rect);

    bar->_last_fill_width = fill.width;
}

global void
gui_progress_bar_set_values(progress_bar_st *bar, int max_value, int cur_value)
{
    bar->max_value = MAX(0, max_value);
    bar->cur_value = MAX(0, MIN(bar->max_value, cur_value));

    gui_progress_bar_update_fill(bar);
}

global void
gui_progress_bar_on_pointer_down(widget_st *widget, event_st event _unsd, point_st pos)
{
    progress_bar_st *bar = widget->data;
    rect_st inner = gui_progress_bar_inner_rect(bar);
    int offset;
    int value;

    if (bar->max_value <= 0 || inner.width <= 0) {
        return;
    }

    offset = MAX(0, MIN(inner.width, pos.x - inner.x));

    value = (int)((uint32_t)offset * (uint32_t)bar->max_value / (uint32_t)inner.width);

    if (bar->on_pointer_down) {
        bar->on_pointer_down(bar, value);
    }
}
