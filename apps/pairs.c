/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: pairs.c - Pair matching / Memory game
 */

#include "lib.h"
#include <gui.h>

enum {
    GRID_CELL_WIDTH = 36,
    GRID_CELL_HEIGHT = 36,
    GRID_ROWS = 6,
    GRID_COLS = 7,
    GRID_CELL_COUNT = GRID_ROWS * GRID_COLS,
    GRID_BORDER = 1,
    GRID_WIDTH = GRID_WIDTH_SPACED(GRID_CELL_WIDTH, GRID_COLS, GRID_BORDER),
    GRID_HEIGHT = GRID_HEIGHT_SPACED(GRID_CELL_HEIGHT, GRID_ROWS, GRID_BORDER),
    GRID_X = 0,
    GRID_Y = TITLE_BAR_HEIGHT - 1,

    WINDOW_WIDTH = GRID_X + GRID_WIDTH,
    WINDOW_HEIGHT = GRID_Y + GRID_HEIGHT,

    PAIR_COUNT = GRID_CELL_COUNT / 2,

    MISMATCH_TICKS = TICK_FREQUENCY * 8 / 10, /* 0.8s */
};

typedef struct {
    uint8_t window_pixels[WINDOW_WIDTH * WINDOW_HEIGHT];
    surface_st window_surface;
    window_st window;

    widget_st title_bar;
    widget_st close_button;
    widget_st buttons[GRID_CELL_COUNT];
    widget_st *widgets[GRID_CELL_COUNT + 2];

    grid_st grid;

    uint8_t button_icons[GRID_CELL_COUNT];
    uint8_t button_states[GRID_CELL_COUNT];

    int first_pick;
    int second_pick;
    int tries;
    int matched_count;
    unsigned waiting;
} app_state_st;

static app_state_st *app_state = NULL;

static bitmap_st *icons[PAIR_COUNT] = {
    &glyph_mn_bbchick_2x,
    &glyph_mn_beaver_2x,
    &glyph_mn_cactus_2x,
    &glyph_mn_dolphin_2x,
    &glyph_mn_drmcamel_2x,
    &glyph_mn_elephant_2x,
    &glyph_mn_flamingo_2x,
    &glyph_mn_horsefac_2x,
    &glyph_mn_monkey_2x,
    &glyph_mn_mushroom_2x,
    &glyph_mn_octopus_2x,
    &glyph_mn_pandafac_2x,
    &glyph_mn_palmtree_2x,
    &glyph_mn_pumpkin_2x,
    &glyph_mn_rabbit_2x,
    &glyph_mn_robotfac_2x,
    &glyph_mn_sloth_2x,
    &glyph_mn_snail_2x,
    &glyph_mn_tigerfac_2x,
    &glyph_mn_trex_2x,
    &glyph_mn_tulip_2x,
};

enum {
    BUTTON_STATE_HIDDEN = 0,
    BUTTON_STATE_REVEALED = 1,
    BUTTON_STATE_MATCHED = 2,
};

static void
shuffle_icons(void)
{
    app_state_st *a = app_state;
    uint8_t deck[GRID_CELL_COUNT];

    for (int i = 0; i < PAIR_COUNT; i++) {
        deck[i * 2] = i;
        deck[i * 2 + 1] = i;
    }

    for (int i = GRID_CELL_COUNT - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        uint8_t tmp = deck[i];
        deck[i] = deck[j];
        deck[j] = tmp;
    }

    for (int i = 0; i < GRID_CELL_COUNT; i++) {
        a->button_icons[i] = deck[i];
    }
}

static void
draw_button(widget_st *widget)
{
    app_state_st *a = app_state;

    int idx = widget->tag1;
    uint8_t state = a->button_states[idx];
    rect_st rect = widget->rect;
    int pressed = widget->window->pressed_widget == widget && !a->waiting;

    if (state == BUTTON_STATE_HIDDEN && !pressed) {
        gui_surface_draw_rect(a->window.surface, rect, COLOR_WIDGET_BG);
    } else if (state == BUTTON_STATE_HIDDEN && pressed) {
        gui_surface_draw_rect(a->window.surface, rect, COLOR_WIDGET_SEL_BG);
    } else {
        gui_surface_draw_rect(a->window.surface, rect, COLOR_WIDGET_BG);
        gui_surface_draw_bitmap_centered(a->window.surface, rect, icons[a->button_icons[idx]],
            COLOR_WIDGET_FG);
    }

    gui_wm_render_window_region(&a->window, rect);
}

static void
reveal_icon(int idx)
{
    app_state_st *a = app_state;

    a->button_states[idx] = BUTTON_STATE_REVEALED;
    draw_button(&a->buttons[idx]);
}

static void
hide_icon(int idx)
{
    app_state_st *a = app_state;

    a->button_states[idx] = BUTTON_STATE_HIDDEN;
    draw_button(&a->buttons[idx]);
}

static void
update_status(void)
{
    app_state_st *a = app_state;

    if (a->matched_count == PAIR_COUNT) {
        gui_status_set("You won after %d tries! Click to play again", a->tries);
    } else {
        gui_status_set("Tries: %d", a->tries);
    }
}

static void
restart_game(void)
{
    app_state_st *a = app_state;

    shuffle_icons();

    a->first_pick = -1;
    a->second_pick = -1;
    a->tries = 0;
    a->matched_count = 0;
    a->waiting = 0;

    for (int i = 0; i < GRID_CELL_COUNT; i++) {
        a->button_states[i] = BUTTON_STATE_HIDDEN;
        draw_button(&a->buttons[i]);
    }

    update_status();
}

static void
on_tick(window_st *window _unsd)
{
    app_state_st *a = app_state;

    if (!a->waiting) {
        return;
    }

    if (--a->waiting) {
        return;
    }

    hide_icon(a->first_pick);
    a->first_pick = -1;

    hide_icon(a->second_pick);
    a->second_pick = -1;

    a->waiting = 0;

    update_status();
}

static void
on_cell_pointer_up(widget_st *widget, event_st event _unsd, point_st pos _unsd)
{
    app_state_st *a = app_state;

    if (a->matched_count == PAIR_COUNT) {
        restart_game();
        return;
    }

    if (a->waiting) {
        return;
    }

    int idx = widget->tag1;

    if (a->button_states[idx] != BUTTON_STATE_HIDDEN) {
        return;
    }

    if (a->first_pick == -1) {
        a->first_pick = idx;
        reveal_icon(a->first_pick);
        return;
    }

    a->second_pick = idx;
    reveal_icon(a->second_pick);
    a->tries++;

    if (a->button_icons[a->first_pick] == a->button_icons[a->second_pick]) {
        a->button_states[a->first_pick] = BUTTON_STATE_MATCHED;
        a->button_states[a->second_pick] = BUTTON_STATE_MATCHED;
        a->first_pick = -1;
        a->second_pick = -1;
        a->matched_count++;
    } else {
        a->waiting = MISMATCH_TICKS;
    }

    update_status();
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
    app_pairs.main_window = NULL;

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
    a->window.title = "Pairs";
    a->window.widgets = a->widgets;
    a->window.widgets_capacity = sizeof(a->widgets) / sizeof(a->widgets[0]);
    a->window.draw = draw_window;
    a->window.on_active_change = on_active_change;
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
    a->grid.border = GRID_BORDER;
    a->grid.x = GRID_X;
    a->grid.y = GRID_Y;

    for (int i = 0; i < GRID_CELL_COUNT; i++) {
        int col = i % GRID_COLS;
        int row = i / GRID_COLS;

        gui_button_init(&a->buttons[i]);
        a->buttons[i].rect = gui_grid_cell_rect(&a->grid, col, row);
        a->buttons[i].tag1 = i;
        a->buttons[i].draw = draw_button;
        a->buttons[i].on_pointer_up = on_cell_pointer_up;
        a->buttons[i].hide_border = 1;

        gui_window_add_widget(&a->window, &a->buttons[i]);
    }
}

static int
init_app(void)
{
    ASSERT(!app_state);

    app_state = heap_alloc(sizeof(app_state_st), "Pairs app", 0);

    if (!app_state) {
        return E_NOT_ENOUGH_MEMORY;
    }

    init_window();
    init_grid();

    restart_game();

    app_pairs.main_window = &app_state->window;

    return E_OK;
}

global app_st app_pairs = {
    .icon = &icon_pairs,
    .init = init_app,
};
