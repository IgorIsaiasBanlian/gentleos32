/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: player.c - Music player app
 */

#include <gui.h>

enum {
    PADDING = 8,

    TRANSPORT_BUTTON_COUNT = 7,
    TRANSPORT_BUTTON_WIDTH = 25,
    TRANSPORT_BUTTON_HEIGHT = 23,
    TRANSPORT_BUTTON_GAP = 4,

    CONTENT_X = PADDING,
    CONTENT_Y = TITLE_BAR_HEIGHT + PADDING - 1,
    CONTENT_WIDTH = TRANSPORT_BUTTON_COUNT * TRANSPORT_BUTTON_WIDTH +
        (TRANSPORT_BUTTON_COUNT - 1) * TRANSPORT_BUTTON_GAP,

    TITLE_Y = CONTENT_Y,
    TITLE_HEIGHT = 8,

    PROGRESS_Y = TITLE_Y + TITLE_HEIGHT + PADDING,
    PROGRESS_HEIGHT = 12,

    TIME_Y = PROGRESS_Y + PROGRESS_HEIGHT + 6,
    TIME_HEIGHT = 8,

    TRANSPORT_BUTTON_Y = TIME_Y + TIME_HEIGHT + PADDING,

    PLAY_LIST_COLS = 1,
    PLAY_LIST_ROWS = 8,
    PLAY_LIST_BORDER = 1,
    PLAY_LIST_CELL_WIDTH = CONTENT_WIDTH - 2 * PLAY_LIST_BORDER,
    PLAY_LIST_CELL_HEIGHT = 16,
    PLAY_LIST_X = CONTENT_X,
    PLAY_LIST_Y = TRANSPORT_BUTTON_Y + TRANSPORT_BUTTON_HEIGHT + PADDING,
    PLAY_LIST_HEIGHT = GRID_HEIGHT_SPACED(PLAY_LIST_CELL_HEIGHT, PLAY_LIST_ROWS, PLAY_LIST_BORDER),

    PAGE_BUTTON_WIDTH = 15,
    PAGE_BUTTON_HEIGHT = 15,
    PAGE_BUTTON_GAP = 4,
    PAGE_BUTTON_Y = PLAY_LIST_Y + PLAY_LIST_HEIGHT + PADDING,
    PAGE_BUTTON_PREV_X = CONTENT_X + (CONTENT_WIDTH - 2 * PAGE_BUTTON_WIDTH - PAGE_BUTTON_GAP) / 2,
    PAGE_BUTTON_NEXT_X = PAGE_BUTTON_PREV_X + PAGE_BUTTON_WIDTH + PAGE_BUTTON_GAP,

    WINDOW_WIDTH = CONTENT_X + CONTENT_WIDTH + PADDING,
    WINDOW_HEIGHT = PAGE_BUTTON_Y + PAGE_BUTTON_HEIGHT + PADDING,
};

enum {
    TRANSPORT_BUTTON_PREV,
    TRANSPORT_BUTTON_PLAY,
    TRANSPORT_BUTTON_PAUSE,
    TRANSPORT_BUTTON_STOP,
    TRANSPORT_BUTTON_NEXT,
    TRANSPORT_BUTTON_SHUFFLE,
    TRANSPORT_BUTTON_LOOP,
};

typedef struct {
    uint8_t window_pixels[WINDOW_WIDTH * WINDOW_HEIGHT];
    surface_st window_surface;
    window_st window;

    widget_st title_bar;
    widget_st close_button;
    widget_st transport_buttons[TRANSPORT_BUTTON_COUNT];
    list_widget_st play_list;
    widget_st prev_page_button;
    widget_st next_page_button;

    widget_st *widgets[TRANSPORT_BUTTON_COUNT + 5];
} app_state_st;

static app_state_st *app_state = NULL;

static bitmap_st *transport_button_icons[TRANSPORT_BUTTON_COUNT] = {
    &icon_player_prev,
    &icon_player_play,
    &icon_player_pause,
    &icon_player_stop,
    &icon_player_next,
    &icon_player_shuffle,
    &icon_player_loop,
};

static void
draw_title(void)
{
    app_state_st *a = app_state;
    rect_st rect = gui_rect_make(CONTENT_X, TITLE_Y, CONTENT_WIDTH, TITLE_HEIGHT);
    const char *title = "Song Title";

    gui_surface_draw_rect(a->window.surface, rect, COLOR_WIDGET_BG);
    gui_surface_draw_str_centered(a->window.surface, rect, font_8x8, title,
        COLOR_WIDGET_FG, COLOR_WIDGET_BG);

    gui_wm_render_window_region(&a->window, rect);
}

static void
draw_progress(void)
{
    app_state_st *a = app_state;
    rect_st outer = gui_rect_make(CONTENT_X, PROGRESS_Y, CONTENT_WIDTH, PROGRESS_HEIGHT);
    rect_st inner = gui_rect_shrink(outer, 1);

    gui_surface_draw_border(a->window.surface, outer, COLOR_BORDER);
    gui_surface_draw_rect(a->window.surface, inner, COLOR_WIDGET_BG);

    gui_wm_render_window_region(&a->window, outer);
}

static void
draw_time(void)
{
    app_state_st *a = app_state;
    rect_st rect = gui_rect_make(CONTENT_X, TIME_Y, CONTENT_WIDTH, TIME_HEIGHT);
    const char *time = "01:23 / 03:45";

    gui_surface_draw_rect(a->window.surface, rect, COLOR_WIDGET_BG);
    gui_surface_draw_str_centered(a->window.surface, rect, font_8x8, time,
        COLOR_WIDGET_FG, COLOR_WIDGET_BG);

    gui_wm_render_window_region(&a->window, rect);
}

static void
on_button_down(widget_st *widget, event_st event, point_st pos)
{
    switch (widget->tag1) {
    case TRANSPORT_BUTTON_SHUFFLE:
    case TRANSPORT_BUTTON_LOOP:
        widget->active = !widget->active;
        break;
    }

    gui_button_on_pointer_down(widget, event, pos);
}

static void
draw_window(window_st *window)
{
    gui_window_draw(window, COLOR_WIDGET_BG);
    draw_title();
    draw_progress();
    draw_time();
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

static void
init_transport_buttons(void)
{
    app_state_st *a = app_state;
    int i;
    widget_st *button;

    for (i = 0; i < TRANSPORT_BUTTON_COUNT; ++i) {
        button = &a->transport_buttons[i];

        gui_button_init(button);

        button->rect = gui_rect_make(
            CONTENT_X + i * (TRANSPORT_BUTTON_WIDTH + TRANSPORT_BUTTON_GAP),
            TRANSPORT_BUTTON_Y,
            TRANSPORT_BUTTON_WIDTH,
            TRANSPORT_BUTTON_HEIGHT
        );
        button->tag1 = i;
        button->bitmap = transport_button_icons[i];
        button->on_pointer_down = on_button_down;

        gui_window_add_widget(&a->window, button);
    }
}

static void
init_play_list(void)
{
    app_state_st *a = app_state;

    a->play_list.grid.cell_width = PLAY_LIST_CELL_WIDTH;
    a->play_list.grid.cell_height = PLAY_LIST_CELL_HEIGHT;
    a->play_list.grid.cols = PLAY_LIST_COLS;
    a->play_list.grid.rows = PLAY_LIST_ROWS;
    a->play_list.grid.border = PLAY_LIST_BORDER;
    a->play_list.grid.x = PLAY_LIST_X;
    a->play_list.grid.y = PLAY_LIST_Y;

    gui_list_widget_init(&a->play_list);
    gui_window_add_widget(&a->window, &a->play_list.widget);
}

static void
init_page_buttons(void)
{
    app_state_st *a = app_state;

    gui_list_widget_init_page_buttons(&a->play_list,
        &a->prev_page_button, &a->next_page_button);

    a->prev_page_button.rect = gui_rect_make(
        PAGE_BUTTON_PREV_X,
        PAGE_BUTTON_Y,
        PAGE_BUTTON_WIDTH,
        PAGE_BUTTON_HEIGHT
    );

    a->next_page_button.rect = gui_rect_make(
        PAGE_BUTTON_NEXT_X,
        PAGE_BUTTON_Y,
        PAGE_BUTTON_WIDTH,
        PAGE_BUTTON_HEIGHT
    );

    gui_window_add_widget(&a->window, &a->prev_page_button);
    gui_window_add_widget(&a->window, &a->next_page_button);
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
    init_transport_buttons();
    init_play_list();
    init_page_buttons();

    app_player.main_window = &app_state->window;

    return E_OK;
}

global app_st app_player = {
    .init = init_app,
};
