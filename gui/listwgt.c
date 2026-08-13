/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: listwgt.c - Paginated list widget
 */

#include <gui.h>

enum {
    LABEL_H_PADDING = 4,
};

global void
gui_list_widget_init(list_widget_st *list)
{
    list->page_size = list->grid.cols * list->grid.rows;
    list->page_count = 1;
    list->item_count = 0;
    list->cur_page = 0;
    list->cur_index = 0;

    list->widget.rect = gui_grid_rect(&list->grid);
    list->widget.data = list;
    list->widget.draw = gui_list_widget_draw;
    list->widget.on_pointer_down = gui_list_widget_on_pointer_down;
}

global void
gui_list_widget_set_item_count(list_widget_st *list, int item_count)
{
    list->item_count = MAX(0, item_count);

    list->page_count = (list->item_count + list->page_size - 1) / list->page_size;
    list->page_count = MAX(1, list->page_count);

    list->cur_page = MAX(0, MIN(list->page_count - 1, list->cur_page));
    list->cur_index = MAX(0, MIN(list->item_count - 1, list->cur_index));

    gui_widget_draw(&list->widget);
}

global void
gui_list_widget_set_page(list_widget_st *list, int page)
{
    page = MAX(0, MIN(list->page_count - 1, page));

    if (page == list->cur_page) {
        return;
    }

    list->cur_page = page;

    gui_widget_draw(&list->widget);
}

static void
gui_list_widget_draw_cell(list_widget_st *list, int index, rect_st rect)
{
    surface_st *surface = list->widget.window->surface;
    font_st *font = list->widget.font ? list->widget.font : font_8x8;
    int is_sel = (index == list->cur_index);
    uint8_t fg = is_sel ? COLOR_WIDGET_SEL_FG : COLOR_WIDGET_FG;
    uint8_t bg = is_sel ? COLOR_WIDGET_SEL_BG : COLOR_WIDGET_BG;
    const char *label = list->get_label ? list->get_label(list, index) : NULL;
    const char *right_label = list->get_right_label ? list->get_right_label(list, index) : NULL;

    gui_surface_draw_rect(surface, rect, bg);

    if (label) {
        gui_surface_draw_str_cl(surface, rect, LABEL_H_PADDING, font, label, fg, bg);
    }

    if (right_label) {
        gui_surface_draw_str_cr(surface, rect, LABEL_H_PADDING, font, right_label, fg, bg);
    }
}

static void
gui_list_widget_draw_item(list_widget_st *list, int index)
{
    int cell = index - list->cur_page * list->page_size;
    rect_st rect;

    if (!list->widget.window) {
        return;
    }

    if (index >= list->item_count || cell < 0 || cell >= list->page_size) {
        return;
    }

    rect = gui_grid_cell_rect(&list->grid, cell % list->grid.cols, cell / list->grid.cols);

    gui_list_widget_draw_cell(list, index, rect);
    gui_wm_render_window_region(list->widget.window, rect);
}

global void
gui_list_widget_set_index(list_widget_st *list, int index)
{
    int prev_index = list->cur_index;
    int page;

    index = MAX(0, MIN(list->item_count - 1, index));
    page = index / list->page_size;

    if (index == list->cur_index && page == list->cur_page) {
        return;
    }

    list->cur_index = index;

    if (page == list->cur_page) {
        gui_list_widget_draw_item(list, prev_index);
        gui_list_widget_draw_item(list, index);
    } else {
        list->cur_page = page;
        gui_widget_draw(&list->widget);
    }
}

global void
gui_list_widget_draw(widget_st *widget)
{
    list_widget_st *list = widget->data;
    surface_st *surface;
    rect_st rect;
    int index;

    if (!widget->window) {
        return;
    }

    surface = widget->window->surface;

    gui_surface_draw_rect(surface, widget->rect, COLOR_WIDGET_BG);
    gui_surface_draw_border(surface, widget->rect, COLOR_BORDER);

    for (int cell = 0; cell < list->page_size; cell++) {
        rect = gui_grid_cell_rect(&list->grid,
            cell % list->grid.cols, cell / list->grid.cols);
        index = list->cur_page * list->page_size + cell;

        if (index < list->item_count) {
            gui_list_widget_draw_cell(list, index, rect);
        }
    }

    gui_wm_render_window_region(widget->window, widget->rect);
}

global void
gui_list_widget_on_pointer_down(widget_st *widget, event_st event _unsd, point_st pos)
{
    list_widget_st *list = widget->data;
    grid_pos_st gpos = gui_grid_cell_at(&list->grid, pos);
    int index = list->cur_page * list->page_size + gpos.row * list->grid.cols + gpos.col;

    if (index >= list->item_count) {
        return;
    }

    gui_list_widget_set_index(list, index);

    if (list->on_select) {
        list->on_select(list, index);
    }
}

static void
gui_list_widget_on_page_button_click(widget_st *widget, event_st event, point_st pos)
{
    list_widget_st *list = widget->data;
    int dp = widget->tag1;

    gui_list_widget_set_page(list, list->cur_page + dp);
    gui_button_on_pointer_down(widget, event, pos);
}

global void
gui_list_widget_init_page_buttons(list_widget_st *list, widget_st *prev_btn, widget_st *next_btn)
{
    gui_button_init(prev_btn);
    prev_btn->bitmap = &sprite_caret_l;
    prev_btn->data = list;
    prev_btn->tag1 = -1;
    prev_btn->on_pointer_down = gui_list_widget_on_page_button_click;

    gui_button_init(next_btn);
    next_btn->bitmap = &sprite_caret_r;
    next_btn->data = list;
    next_btn->tag1 = 1;
    next_btn->on_pointer_down = gui_list_widget_on_page_button_click;
}
