/*
 * Copyright (c) 2014-2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: feat.h - Shared application features
 */

#ifndef _FEAT_H_
#define _FEAT_H_

#include <gui.h>

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
#define CARD_SUIT(c)  (((c) / CARD_RANK_COUNT) % CARD_SUIT_COUNT)
#define CARD_COLOR(c) (CARD_SUIT(c) <= CARD_SUIT_DIAMONDS)
#define CARD_PILE_TOP(p) ((p)->count > 0 ? (p)->cards[(p)->count - 1] : CARD_EMPTY)
#define CARD_PILE_IS_SELECTED(game, pile) ((game)->cur_move.src == (pile))

#include "p_feat.h"

#endif /* _FEAT_H_ */
