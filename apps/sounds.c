/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: sounds.c - Sound playing app
 */

#include <gui.h>

enum {
    KEY_W_WIDTH = 30,
    KEY_W_HEIGHT = 130,
    KEY_W_COUNT = 15,

    KEY_B_WIDTH = 17,
    KEY_B_HEIGHT = 80,
    KEY_B_COUNT = 10,

    KEYBOARD_Y = (TITLE_BAR_HEIGHT - 1),
    KEYBOARD_HEIGHT = (KEY_W_HEIGHT),

    WINDOW_WIDTH = ((KEY_W_COUNT * KEY_W_WIDTH) - (KEY_W_COUNT - 1)),
    WINDOW_HEIGHT = (KEYBOARD_Y + KEYBOARD_HEIGHT),

    TAG_KEY_W = 1,
    TAG_KEY_B = 2,
};

typedef struct {
    uint8_t window_pixels[WINDOW_WIDTH * WINDOW_HEIGHT];
    surface_st window_surface;
    window_st window;

    widget_st keys_w[KEY_W_COUNT];
    widget_st keys_b[KEY_B_COUNT];
    widget_st title_bar;
    widget_st close_button;
    widget_st *widgets[KEY_W_COUNT + KEY_B_COUNT + 2];
} app_state_st;

static app_state_st *app_state = NULL;

static void
draw_key_w(widget_st *widget)
{
    rect_st rect_base = gui_rect_shrink(widget->rect, 1);
    uint8_t color = (widget == widget->window->pressed_widget)
        ? COLOR_PIANO_KEY_SEL : COLOR_PIANO_KEY_WHITE;

    int octave = widget->tag2 / 7;
    int ofs = widget->tag2 % 7;

    rect_st rect_top = rect_base;
    if (ofs == 1 || ofs == 2 || ofs == 4 || ofs == 5 || ofs == 6) {
        rect_top.x += KEY_B_WIDTH / 2;
        rect_top.width -= KEY_B_WIDTH / 2;
    }
    if ((ofs == 0 && octave < 2) || ofs == 1 || ofs == 3 || ofs == 4 || ofs == 5) {
        rect_top.width -= KEY_B_WIDTH / 2;
    }
    gui_surface_draw_rect(widget->window->surface, rect_top, color);

    rect_st rect_bottom = rect_base;
    rect_bottom.y += KEY_B_HEIGHT;
    rect_bottom.height -= KEY_B_HEIGHT;
    gui_surface_draw_rect(widget->window->surface, rect_bottom, color);

    gui_wm_render_window_region(widget->window, widget->rect);
}

static void
draw_key_b(widget_st *widget)
{
    uint8_t color = (widget == widget->window->pressed_widget)
        ? COLOR_PIANO_KEY_SEL : COLOR_PIANO_KEY_BLACK;

    gui_surface_draw_rect(widget->window->surface, widget->rect, color);

    gui_wm_render_window_region(widget->window, widget->rect);
}

static unsigned
key_frequency(widget_st *widget)
{
    static unsigned freqs_w[] = { 131, 147, 165, 175, 196, 220, 247 };
    static unsigned freqs_b[] = { 139, 156, 185, 208, 233 };

    int is_w = widget->tag1 == TAG_KEY_W;
    unsigned *freqs = is_w ? freqs_w : freqs_b;
    unsigned octave = is_w ? widget->tag2 / 7 : widget->tag2 / 5;
    unsigned ofs = is_w ? widget->tag2 % 7 : widget->tag2 % 5;

    return freqs[ofs] * (1 << octave);
}

static void
on_key_pointer_down(widget_st *widget, event_st event, point_st pos)
{
    krn_speaker_play_freq(key_frequency(widget), &app_sounds);

    gui_button_on_pointer_down(widget, event, pos);
}

static void
on_key_pointer_up(widget_st *widget, event_st event, point_st pos)
{
    krn_speaker_stop(&app_sounds);

    gui_button_on_pointer_up(widget, event, pos);
}

static void
on_key_pointer_out(widget_st *widget, event_st event, point_st pos)
{
    krn_speaker_stop(&app_sounds);

    gui_button_on_pointer_out(widget, event, pos);
}

static void
draw_window(window_st *window)
{
    gui_window_draw(window, COLOR_BORDER);
}

static void
close_window(window_st *window _unsd)
{
    gui_wm_remove_window(window);
    app_sounds.main_window = NULL;

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
    a->window.title = "Sounds";
    a->window.draw = draw_window;
    a->window.widgets = a->widgets;
    a->window.widgets_capacity = sizeof(a->widgets) / sizeof(a->widgets[0]);
    a->window.on_close = close_window;

    gui_window_init_frame(&a->window, &a->title_bar, &a->close_button);
}

static void
init_keys(void)
{
    app_state_st *a = app_state;

    for (int i = 0; i < KEY_B_COUNT; i++) {
        int octave_no = i / 5;
        int octave_ofs = i % 5;
        int key_w_idx = (octave_no * 7) + octave_ofs + 1 + (octave_ofs > 1 ? 1 : 0);

        a->keys_b[i].rect.x = (key_w_idx * KEY_W_WIDTH) - key_w_idx - (KEY_B_WIDTH / 2);
        a->keys_b[i].rect.y = TITLE_BAR_HEIGHT;
        a->keys_b[i].rect.width = KEY_B_WIDTH;
        a->keys_b[i].rect.height = KEY_B_HEIGHT;
        a->keys_b[i].draw = draw_key_b;
        a->keys_b[i].tag1 = TAG_KEY_B;
        a->keys_b[i].tag2 = i;
        a->keys_b[i].press_on_move_in = 1;
        a->keys_b[i].on_pointer_down = on_key_pointer_down;
        a->keys_b[i].on_pointer_up = on_key_pointer_up;
        a->keys_b[i].on_pointer_out = on_key_pointer_out;
        gui_window_add_widget(&a->window, &a->keys_b[i]);
    }

    for (int i = 0; i < KEY_W_COUNT; i++) {
        a->keys_w[i].rect.x = (i * KEY_W_WIDTH) - i;
        a->keys_w[i].rect.y = TITLE_BAR_HEIGHT - 1;
        a->keys_w[i].rect.width = KEY_W_WIDTH;
        a->keys_w[i].rect.height = KEY_W_HEIGHT;
        a->keys_w[i].draw = draw_key_w;
        a->keys_w[i].tag1 = TAG_KEY_W;
        a->keys_w[i].tag2 = i;
        a->keys_w[i].press_on_move_in = 1;
        a->keys_w[i].on_pointer_down = on_key_pointer_down;
        a->keys_w[i].on_pointer_up = on_key_pointer_up;
        a->keys_w[i].on_pointer_out = on_key_pointer_out;
        gui_window_add_widget(&a->window, &a->keys_w[i]);
    }
}

static int
init_app(void)
{
    ASSERT(!app_state);

    app_state = heap_alloc(sizeof(app_state_st), "Sounds app", 0);

    if (!app_state) {
        return E_NOT_ENOUGH_MEMORY;
    }

    init_window();
    init_keys();

    app_sounds.main_window = &app_state->window;

    return E_OK;
}

global app_st app_sounds = {
    .icon = &icon_sounds,
    .init = init_app,
};
