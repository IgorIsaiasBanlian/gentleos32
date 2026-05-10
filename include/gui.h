// --------------------------------------------------------------------------------------
// Copyright (c) 2025-2026 luke8086
// Distributed under the terms of GPL-2 License
// --------------------------------------------------------------------------------------
// File: gui.h - GUI API
// --------------------------------------------------------------------------------------

#ifndef _GUI_H_
#define _GUI_H_

#include <kernel.h>

typedef struct {
    int x;
    int y;
} point_st;

typedef struct {
    int width;
    int height;
} size_st;

typedef struct {
    union {
        point_st pos;
        struct {
            int x;
            int y;
        };
    };

    union {
        size_st size;
        struct {
            int width;
            int height;
        };
    };
} rect_st;

enum {
    FONT_COUNT = 2,
};

#define font_8x16 (&fonts[0])
#define font_8x8 (&fonts[1])

typedef struct {
    size_st size;
    const char *name;
    const uint8_t *pixels;
} font_st;

typedef struct {
    size_st size;
    int bpp;
    int pitch;
    int foreground;
    int alpha;
    const uint8_t *pixels;
} bitmap_st;

typedef struct {
    size_st size;
    int pitch;
    uint8_t *pixels;
} surface_st;

enum {
    WIDGET_TYPE_UNKNOWN = 0,
    WIDGET_TYPE_BUTTON = 1,
    WIDGET_TYPE_CUSTOM = 2,
};

struct window;
typedef struct window window_st;

struct widget;
typedef struct widget widget_st;

struct widget {
    window_st *window;
    rect_st rect;

    int type;
    int tag1;
    int tag2;

    int active;
    int press_on_move_in;
    int press_sticky;
    int press_fixed;
    int hidden;
    int hide_border;

    void (*draw)(widget_st *);
    void (*on_pointer_down)(widget_st *, event_st event, point_st pos);
    void (*on_pointer_up)(widget_st *, event_st event, point_st pos);
    void (*on_pointer_out)(widget_st *, event_st event, point_st pos);
    void (*on_pointer_move)(widget_st *, event_st event, point_st pos);
    void (*on_pointer_alt)(widget_st *, event_st event, point_st pos);

    const char *label;
    font_st *font;
    bitmap_st *bitmap;
};

struct window {
    rect_st rect;
    surface_st *surface;

    int visible;
    int active;

    const char *title;
    uint8_t bg_color;

    widget_st **widgets;
    size_t widgets_count;
    size_t widgets_capacity;
    widget_st *pressed_widget;

    void (*render_region)(window_st *, rect_st reg);
    void (*on_pointer)(window_st *, event_st event);
    void (*on_key_down)(window_st *, event_st event);
    void (*on_key_up)(window_st *, event_st event);
    void (*on_tick)(window_st *);
    void (*on_active_change)(window_st *);
};

typedef struct {
    int cell_width;
    int cell_height;
    int cols;
    int rows;
    int x;
    int y;
} grid_st;

typedef struct {
    bitmap_st *icon;
    void (*show)(void);
} app_st;

enum {
    COLOR_BLACK = 0x00,
    COLOR_WHITE = 0x0f,
    COLOR_BLUE = 0x01,
    COLOR_GREEN = 0x02,
    COLOR_RED = 0x04,
    COLOR_TITLE_BAR_ACTIVE = 0x0e,
    COLOR_TITLE_BAR_INACTIVE = 0x07,
    COLOR_WINDOW = 0x07,
    COLOR_WINDOW_DARKER = 0x08,
    COLOR_BORDER = 0x00,
    COLOR_TEXT_ACTIVE = 0x00,
    COLOR_BUTTON_PRESSED = 0x00,
    COLOR_DESKTOP = 0x03,
    COLOR_DESKTOP_ALT = 0x01,
};

enum {
    TITLE_BAR_HEIGHT = 24,
    PANEL_WIDTH = 64,
    STATUS_HEIGHT = 24,
};

#define GRID_WIDTH_SPACED(cell_width, cols) ((cell_width) * (cols) + (cols) - 1)
#define GRID_HEIGHT_SPACED(cell_height, rows) ((cell_height) * (rows) + (rows) - 1)

enum {
    CARD_RANK_COUNT = 13,
    CARD_SUIT_COUNT = 4,
    CARD_DECK_SIZE = 52,

    CARD_SUIT_HEARTS = 0,
    CARD_SUIT_DIAMONDS = 1,
    CARD_SUIT_CLUBS = 2,
    CARD_SUIT_SPADES = 3,

    CARD_EMPTY = 0xff,
    CARD_PILE_ALL_FACE_DOWN = 0xff,
};

typedef uint8_t card_t;

typedef struct {
    int type;
    int index;
    int count;
    int capacity;
    int face_up_from;
    int is_cascade;
    int replace_on_push;
    int step;
    card_t *cards;
    widget_st *widget;
} card_pile_st;

typedef struct {
    card_pile_st *src;
    card_pile_st *dst;
    int count;
} card_move_st;

typedef struct {
    surface_st *surface;
    int card_width;
    int card_height;
    int card_step;

    card_move_st cur_move;
} card_game_st;


#define CARD_RANK(c)  ((c) % CARD_RANK_COUNT)
#define CARD_SUIT(c)  ((c) / CARD_RANK_COUNT)
#define CARD_COLOR(c) (CARD_SUIT(c) <= CARD_SUIT_DIAMONDS)
#define CARD_PILE_TOP(p) ((p)->count > 0 ? (p)->cards[(p)->count - 1] : CARD_EMPTY)
#define CARD_PILE_IS_SELECTED(game, pile) ((game)->cur_move.src == (pile))

#include "p_gui.h"
#include "p_build.h"
#include "p_apps.h"

#endif // _GUI_H_
