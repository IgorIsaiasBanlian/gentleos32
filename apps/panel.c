/*
 * Copyright (c) 2025-2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: panel.c - Side panel app
 */

#include <gui.h>

enum {
    APP_BUTTON_MARGIN = 8,
    APP_BUTTON_SIZE = 48,
    APP_BUTTON_STRIDE = APP_BUTTON_SIZE + APP_BUTTON_MARGIN,

    NAV_WIDTH = PANEL_WIDTH / 2,
    NAV_HEIGHT = STATUS_HEIGHT,
};

static int current_page = 0;

static surface_st window_surface;
static window_st window;

static size_t app_buttons_count;
static widget_st *app_buttons;
static widget_st prev_button;
static widget_st next_button;

static void
set_page(int page)
{
    current_page = page;

    rect_st area = { .x = 1, .y = 0, .width = window.rect.width - 1,
        .height = window.rect.height - STATUS_HEIGHT };
    gui_surface_draw_rect(window.surface, area, COLOR_WIDGET_BG);

    for (size_t i = 0; i < app_buttons_count; i++) {
        size_t app_idx = current_page * app_buttons_count + i;

        if (app_idx >= gui_apps_count) {
            app_buttons[i].hidden = 1;
            continue;
        }

        app_buttons[i].bitmap = gui_apps[app_idx]->icon;
        app_buttons[i].tag1 = app_idx;
        app_buttons[i].hidden = 0;

        gui_widget_draw(&app_buttons[i]);
    }

    gui_wm_render_window_region(&window, gui_window_area(&window));
}

static void
on_button_pointer_up(widget_st *widget, event_st event, point_st pos)
{
    gui_button_on_pointer_up(widget, event, pos);

    if (gui_apps[widget->tag1]) {
        gui_run_app(gui_apps[widget->tag1]);
    }
}

static void
on_prev_pointer_up(widget_st *widget, event_st event, point_st pos)
{
    gui_button_on_pointer_up(widget, event, pos);

    if (current_page > 0) {
        set_page(current_page - 1);
    }
}

static void
on_next_pointer_up(widget_st *widget, event_st event, point_st pos)
{
    gui_button_on_pointer_up(widget, event, pos);

    int max_page = (gui_apps_count + app_buttons_count - 1) / app_buttons_count - 1;

    if (current_page < max_page) {
        set_page(current_page + 1);
    }
}

static void
draw_window(window_st *window)
{
    gui_surface_draw_rect(window->surface, gui_window_area(window), COLOR_WIDGET_BG);
    gui_surface_draw_v_seg(window->surface, 0, 0, window->rect.height - STATUS_HEIGHT + 1,
        COLOR_BORDER);

    gui_widget_draw(&prev_button);
    gui_widget_draw(&next_button);

    set_page(current_page);
}

static void
init_window(void)
{
    system_info_st *si = &krn_system_info;
    int width = PANEL_WIDTH;
    int height = si->fb_height;

    app_buttons_count = (height - STATUS_HEIGHT - APP_BUTTON_MARGIN) / APP_BUTTON_STRIDE;

    window_surface.size.width = width;
    window_surface.size.height = height;
    window_surface.pitch = width;
    window_surface.pixels = krn_heap_alloc(width * height, "Panel pixels", 1);

    window.rect.x = si->fb_width - width;
    window.rect.y = 0;
    window.rect.width = width;
    window.rect.height = height;
    window.surface = &window_surface;
    window.widgets_capacity = app_buttons_count + 2;
    window.widgets = krn_heap_alloc(sizeof(widget_st *) * window.widgets_capacity,
        "Panel widgets", 1);
    window.visible = 1;
    window.draw = draw_window;
}

static void
init_app_buttons(void)
{
    app_buttons = krn_heap_alloc(sizeof(widget_st) * app_buttons_count,
        "Panel app buttons", 1);

    for (size_t i = 0; i < app_buttons_count; i++) {
        gui_button_init(&app_buttons[i]);
        app_buttons[i].rect.x = APP_BUTTON_MARGIN;
        app_buttons[i].rect.y = APP_BUTTON_MARGIN + (i * APP_BUTTON_STRIDE);
        app_buttons[i].rect.width = APP_BUTTON_SIZE;
        app_buttons[i].rect.height = APP_BUTTON_SIZE;
        app_buttons[i].window = &window;
        app_buttons[i].on_pointer_up = on_button_pointer_up;
        app_buttons[i].hidden = 1;

        gui_window_add_widget(&window, &app_buttons[i]);
    }
}

static void
init_nav_buttons(void)
{
    gui_button_init(&prev_button);
    prev_button.rect.x = 1;
    prev_button.rect.y = window.rect.height - NAV_HEIGHT;
    prev_button.rect.width = NAV_WIDTH - 1;
    prev_button.rect.height = NAV_HEIGHT;
    prev_button.window = &window;
    prev_button.label = "<";
    prev_button.hide_border = 1;
    prev_button.on_pointer_up = on_prev_pointer_up;
    gui_window_add_widget(&window, &prev_button);

    gui_button_init(&next_button);
    next_button.rect.x = NAV_WIDTH;
    next_button.rect.y = window.rect.height - NAV_HEIGHT;
    next_button.rect.width = NAV_WIDTH;
    next_button.rect.height = NAV_HEIGHT;
    next_button.window = &window;
    next_button.label = ">";
    next_button.hide_border = 1;
    next_button.on_pointer_up = on_next_pointer_up;
    gui_window_add_widget(&window, &next_button);
}

static void
init_app(void)
{
    init_window();
    init_app_buttons();
    init_nav_buttons();
}

static void
show_app(void)
{
    gui_wm_set_panel_window(&window);
}

global app_st app_panel = {
    .init = init_app,
    .show = show_app,
};
