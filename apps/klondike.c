/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: klondike.c - Klondike solitaire
 */

#include <feat.h>

enum {
    CARD_WIDTH = 38,
    CARD_HEIGHT = 44,

    COLUMN_CARDS_STEP = 11,
    COLUMN_CARDS_MAX = 24,

    CARD_COUNT = 52,
    FOUND_COUNT = 4,
    COLUMN_COUNT = 7,
    PILE_COUNT = 1 + 1 + FOUND_COUNT + COLUMN_COUNT,

    PAD_X = 8,
    PAD_Y = 8,
    GAP_X = 4,
    GAP_Y = 8,
    HOLDS_Y = TITLE_BAR_HEIGHT + PAD_Y,
    COLUMNS_Y = HOLDS_Y + CARD_HEIGHT + GAP_Y,
    COLUMNS_H = 150,

    WINDOW_WIDTH = 2 * PAD_X + COLUMN_COUNT * CARD_WIDTH + (COLUMN_COUNT - 1) * GAP_X,
    WINDOW_HEIGHT = COLUMNS_Y + COLUMNS_H + PAD_Y,

    PILE_STOCK = 1,
    PILE_WASTE = 2,
    PILE_FOUNDS = 3,
    PILE_COLUMNS = 4,

    PILE_STOCK_IDX = 0,
    PILE_WASTE_IDX = 1,
    PILE_FOUNDS_IDX = 2,
    PILE_COLUMNS_IDX = PILE_FOUNDS_IDX + FOUND_COUNT,

    STATE_DEFAULT = 0,
    STATE_AUTO_PENDING = 1,
    STATE_WON = 2,

    AUTO_MOVE_HIGHLIGHT_TICKS = 2,
    AUTO_MOVE_EXECUTE_TICKS = 6,
};

typedef struct {
    uint8_t window_pixels[WINDOW_WIDTH * WINDOW_HEIGHT];
    surface_st window_surface;
    window_st window;

    widget_st title_bar;
    widget_st close_button;
    widget_st pile_widgets[PILE_COUNT];
    widget_st *widgets[PILE_COUNT + 2];

    card_t stock_cards[CARD_COUNT];
    card_pile_st stock;

    card_t waste_cards[CARD_COUNT];
    card_pile_st waste;

    card_t found_cards[FOUND_COUNT][1];
    card_pile_st founds[FOUND_COUNT];

    card_t column_cards[COLUMN_COUNT][COLUMN_CARDS_MAX];
    card_pile_st columns[COLUMN_COUNT];

    card_pile_st *all_piles[PILE_COUNT];
    card_game_st game;
    int state;
    int ticks_waited;
} app_state_st;

static app_state_st *app_state = NULL;

static void
draw_all_piles(void)
{
    app_state_st *a = app_state;

    for (int i = 0; i < PILE_COUNT; ++i) {
        card_pile_draw(&a->game, a->all_piles[i]);
    }
}

static void
draw_window(window_st * window)
{
    gui_window_draw(window, COLOR_WIDGET_BG);
    draw_all_piles();
}

static int
remaining_cards(void)
{
    app_state_st *a = app_state;
    int i, ret;

    ret = a->stock.count + a->waste.count;

    for (i = 0; i < COLUMN_COUNT; ++i) {
        ret += a->columns[i].count;
    }

    return ret;
}

static void
deal_cards(void)
{
    app_state_st *a = app_state;
    card_t deck[CARD_COUNT];
    int i, j, k;

    card_deck_init(deck, CARD_COUNT);
    card_deck_shuffle(deck, CARD_COUNT);

    a->stock.count = 0;
    a->waste.count = 0;

    for (i = 0; i < FOUND_COUNT; ++i) {
        a->founds[i].count = 0;
    }

    for (i = 0; i < COLUMN_COUNT; ++i) {
        a->columns[i].count = 0;
        a->columns[i].face_up_from = 0;
    }

    k = 0;
    for (i = 0; i < COLUMN_COUNT; ++i) {
        for (j = 0; j <= i; ++j) {
            card_pile_push(&a->columns[i], deck[k++]);
        }
        a->columns[i].face_up_from = i;
    }

    while (k < CARD_COUNT) {
        card_pile_push(&a->stock, deck[k++]);
    }

    a->game.cur_move.src = NULL;
    a->state = STATE_DEFAULT;
}

static void
update_status(void)
{
    app_state_st *a = app_state;

    if (a->state == STATE_WON) {
        gui_status_set("You won! Press R to restart");
        return;
    }

    gui_status_set("Remaining: %d  \xb3  R: Restart", remaining_cards());
}

static void
check_win(void)
{
    app_state_st *a = app_state;
    int i;

    for (i = 0; i < FOUND_COUNT; ++i) {
        if (a->founds[i].count == 0 || CARD_RANK(CARD_PILE_TOP(&a->founds[i])) != 12) {
            return;
        }
    }

    a->state = STATE_WON;
    update_status();
}

static int
get_max_valid_sequence_len(card_pile_st *p)
{
    int i;
    card_t curr, prev;

    if (p->count == 0) {
        return 0;
    }

    for (i = p->count - 1; i > p->face_up_from; --i) {
        curr = p->cards[i];
        prev = p->cards[i - 1];

        if (CARD_RANK(prev) != CARD_RANK(curr) + 1) {
            break;
        }
        if (CARD_COLOR(prev) == CARD_COLOR(curr)) {
            break;
        }
    }

    return p->count - i;
}

static int
card_should_auto_promote(card_t card)
{
    app_state_st *a = app_state;
    int rank = CARD_RANK(card);
    int suit = CARD_SUIT(card);
    int color = CARD_COLOR(card);
    int i;

    if (a->founds[suit].count == 0) {
        if (rank != 0) {
            return 0;
        }
    } else if (rank != CARD_RANK(CARD_PILE_TOP(&a->founds[suit])) + 1) {
        return 0;
    }

    if (rank <= 1) {
        return 1;
    }

    for (i = 0; i < FOUND_COUNT; ++i) {
        if (CARD_COLOR(i * 13) == color) {
            continue;
        }

        if (a->founds[i].count == 0 || CARD_RANK(CARD_PILE_TOP(&a->founds[i])) < rank - 1) {
            return 0;
        }
    }

    return 1;
}

static void
set_auto_move(card_pile_st *src, card_pile_st *dst)
{
    app_state_st *a = app_state;

    a->game.cur_move.src = src;
    a->game.cur_move.dst = dst;
    a->game.cur_move.count = 1;
    a->state = STATE_AUTO_PENDING;
    a->ticks_waited = 0;
}

static void
check_auto_move(void)
{
    app_state_st *a = app_state;
    int i;
    card_t card;

    card = CARD_PILE_TOP(&a->waste);

    if (card != CARD_EMPTY && card_should_auto_promote(card)) {
        set_auto_move(&a->waste, &a->founds[CARD_SUIT(card)]);
        return;
    }

    for (i = 0; i < COLUMN_COUNT; ++i) {
        card = CARD_PILE_TOP(&a->columns[i]);

        if (card != CARD_EMPTY && card_should_auto_promote(card)) {
            set_auto_move(&a->columns[i], &a->founds[CARD_SUIT(card)]);
            return;
        }
    }
}

static void
exec_move(void)
{
    app_state_st *a = app_state;

    card_game_exec_cur_move(&a->game);
    update_status();
    check_win();

    if (a->state != STATE_WON) {
        check_auto_move();
    }
}

static void
start_move(card_pile_st *pile, int count)
{
    app_state_st *a = app_state;

    a->game.cur_move.src = pile;
    a->game.cur_move.count = count;
    card_pile_draw(&a->game, pile);
    update_status();
}

static void
cancel_move(void)
{
    app_state_st *a = app_state;
    card_pile_st *old = a->game.cur_move.src;

    a->game.cur_move.src = NULL;

    if (old) {
        card_pile_draw(&a->game, old);
    }

    update_status();
}

static void
show_error(const char *msg)
{
    cancel_move();
    gui_status_set("%s", msg);
}

static int
can_move_to_column(card_pile_st *col, card_t src_bottom)
{
    card_t dst_top;

    if (col->count == 0) {
        return CARD_RANK(src_bottom) == 12;
    }

    dst_top = CARD_PILE_TOP(col);

    if (CARD_RANK(dst_top) != CARD_RANK(src_bottom) + 1) {
        return 0;
    }

    if (CARD_COLOR(dst_top) == CARD_COLOR(src_bottom)) {
        return 0;
    }

    return 1;
}

static void
request_move_to_column(void)
{
    app_state_st *a = app_state;
    card_pile_st *src = a->game.cur_move.src;
    card_pile_st *dst = a->game.cur_move.dst;
    int n = a->game.cur_move.count;
    card_t src_bottom;

    if (src->type == PILE_COLUMNS && n > get_max_valid_sequence_len(src)) {
        show_error("Invalid move");
        return;
    }

    src_bottom = src->cards[src->count - n];

    if (!can_move_to_column(dst, src_bottom)) {
        show_error("Invalid move");
        return;
    }

    exec_move();
}

static int
can_move_to_found(card_pile_st *found, card_t card)
{
    if (CARD_SUIT(card) != found->index) {
        return 0;
    }

    if (found->count == 0) {
        return CARD_RANK(card) == 0;
    }

    return CARD_RANK(CARD_PILE_TOP(found)) + 1 == CARD_RANK(card);
}

static void
request_move_to_found(void)
{
    app_state_st *a = app_state;
    card_pile_st *src = a->game.cur_move.src;
    card_pile_st *found = a->game.cur_move.dst;

    if (a->game.cur_move.count != 1) {
        show_error("Invalid move");
        return;
    }

    card_t card = CARD_PILE_TOP(src);

    if (!can_move_to_found(found, card)) {
        show_error("Invalid move");
        return;
    }

    exec_move();
}

static void
request_move(void)
{
    app_state_st *a = app_state;
    card_pile_st *dst = a->game.cur_move.dst;

    if (dst->type == PILE_COLUMNS) {
        request_move_to_column();
    } else if (dst->type == PILE_FOUNDS) {
        request_move_to_found();
    } else {
        show_error("Invalid move");
    }
}

static void
draw_card_from_stock(void)
{
    app_state_st *a = app_state;

    if (a->stock.count > 0) {
        card_pile_push(&a->waste, card_pile_pop(&a->stock));
    } else if (a->waste.count > 0) {
        while (a->waste.count > 0) {
            card_pile_push(&a->stock, card_pile_pop(&a->waste));
        }
    }

    card_pile_draw(&a->game, &a->stock);
    card_pile_draw(&a->game, &a->waste);
    update_status();
    check_auto_move();
}

static void
restart_game(void)
{
    deal_cards();
    draw_all_piles();
    update_status();
}

static void
pile_draw(widget_st *widget)
{
    app_state_st *a = app_state;

    card_pile_draw(&a->game, a->all_piles[widget->tag1]);
}

static void
on_pile_pointer_up(widget_st *widget, event_st event _unsd, point_st pos)
{
    app_state_st *a = app_state;

    if (a->state != STATE_DEFAULT) {
        return;
    }

    card_pile_st *pile = a->all_piles[widget->tag1];

    if (pile->type == PILE_STOCK) {
        cancel_move();
        draw_card_from_stock();
        return;
    }

    if (a->game.cur_move.src == NULL) {
        if (pile->type == PILE_FOUNDS || pile->count == 0) {
            return;
        }

        int idx = card_pile_get_card_index_by_ypos(pile, pos.y - widget->rect.y);

        if (pile->type == PILE_COLUMNS && idx < pile->face_up_from) {
            return;
        }

        start_move(pile, pile->count - idx);
    } else if (pile == a->game.cur_move.src) {
        cancel_move();
    } else {
        a->game.cur_move.dst = pile;
        request_move();
    }
}

static void
on_pile_pointer_alt(widget_st *widget, event_st event _unsd, point_st pos _unsd)
{
    app_state_st *a = app_state;

    if (a->state != STATE_DEFAULT) {
        return;
    }

    card_pile_st *pile = a->all_piles[widget->tag1];

    if (pile->type != PILE_WASTE && pile->type != PILE_COLUMNS) {
        return;
    }

    if (pile->count == 0) {
        return;
    }

    cancel_move();
    start_move(pile, 1);

    card_t card = CARD_PILE_TOP(pile);
    a->game.cur_move.dst = &a->founds[CARD_SUIT(card)];
    request_move();
}

static void
on_key_down(window_st *win _unsd, event_st event)
{
    app_state_st *a = app_state;

    if (a->state == STATE_AUTO_PENDING) {
        return;
    }

    if (event.key_code == KEY_R) {
        restart_game();
    }
}

static void
on_tick(window_st *win _unsd)
{
    app_state_st *a = app_state;

    if (a->state != STATE_AUTO_PENDING) {
        return;
    }

    ++a->ticks_waited;

    if (a->ticks_waited == AUTO_MOVE_HIGHLIGHT_TICKS) {
        card_pile_draw(&a->game, a->game.cur_move.src);
        return;
    }

    if (a->ticks_waited >= AUTO_MOVE_EXECUTE_TICKS) {
        a->state = STATE_DEFAULT;
        exec_move();
    }
}

static void
on_active_change(window_st *w)
{
    if (w->active) {
        update_status();
    }
}

static void
init_game(void)
{
    app_state_st *a = app_state;
    int i;
    int x0 = PAD_X;
    int step_x = CARD_WIDTH + GAP_X;

    a->game.surface = &a->window_surface;
    a->game.card_width = CARD_WIDTH;
    a->game.card_height = CARD_HEIGHT;
    a->game.card_step = COLUMN_CARDS_STEP;

    for (i = 0; i < PILE_COUNT; ++i) {
        a->pile_widgets[i].draw = pile_draw;
        a->pile_widgets[i].on_pointer_up = on_pile_pointer_up;
        a->pile_widgets[i].on_pointer_alt = on_pile_pointer_alt;
        a->pile_widgets[i].tag1 = i;
    }

    a->stock.type = PILE_STOCK;
    a->stock.capacity = CARD_COUNT;
    a->stock.cards = a->stock_cards;
    a->stock.face_up_from = CARD_PILE_ALL_FACE_DOWN;
    a->stock.widget = &a->pile_widgets[PILE_STOCK_IDX];
    a->stock.widget->rect = gui_rect_make(x0 + 0 * step_x, HOLDS_Y, CARD_WIDTH, CARD_HEIGHT);
    a->all_piles[PILE_STOCK_IDX] = &a->stock;

    a->waste.type = PILE_WASTE;
    a->waste.capacity = CARD_COUNT;
    a->waste.cards = a->waste_cards;
    a->waste.widget = &a->pile_widgets[PILE_WASTE_IDX];
    a->waste.widget->rect = gui_rect_make(x0 + 1 * step_x, HOLDS_Y, CARD_WIDTH, CARD_HEIGHT);
    a->all_piles[PILE_WASTE_IDX] = &a->waste;

    for (i = 0; i < FOUND_COUNT; ++i) {
        a->founds[i].type = PILE_FOUNDS;
        a->founds[i].index = i;
        a->founds[i].capacity = 1;
        a->founds[i].cards = a->found_cards[i];
        a->founds[i].replace_on_push = 1;
        a->founds[i].widget = &a->pile_widgets[PILE_FOUNDS_IDX + i];
        a->founds[i].widget->rect = gui_rect_make(x0 + (i + 3) * step_x, HOLDS_Y,
            CARD_WIDTH, CARD_HEIGHT);
        a->all_piles[PILE_FOUNDS_IDX + i] = &a->founds[i];
    }

    for (i = 0; i < COLUMN_COUNT; ++i) {
        a->columns[i].type = PILE_COLUMNS;
        a->columns[i].index = i;
        a->columns[i].capacity = COLUMN_CARDS_MAX;
        a->columns[i].cards = a->column_cards[i];
        a->columns[i].is_cascade = 1;
        a->columns[i].widget = &a->pile_widgets[PILE_COLUMNS_IDX + i];
        a->columns[i].widget->rect = gui_rect_make(x0 + i * step_x, COLUMNS_Y,
            CARD_WIDTH, COLUMNS_H);
        a->all_piles[PILE_COLUMNS_IDX + i] = &a->columns[i];
    }

    for (i = 0; i < PILE_COUNT; ++i) {
        gui_window_add_widget(&a->window, &a->pile_widgets[i]);
    }
}

static void
close_window(window_st *window _unsd)
{
    gui_wm_remove_window(window);
    app_klondike.main_window = NULL;

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
    a->window.title = "Klondike";
    a->window.widgets = a->widgets;
    a->window.widgets_capacity = sizeof(a->widgets) / sizeof(a->widgets[0]);
    a->window.draw = draw_window;
    a->window.on_active_change = on_active_change;
    a->window.on_key_down = on_key_down;
    a->window.on_tick = on_tick;
    a->window.on_close = close_window;

    gui_window_init_frame(&a->window, &a->title_bar, &a->close_button);
}

static int
init_app(void)
{
    ASSERT(!app_state);

    app_state = heap_alloc(sizeof(app_state_st), "Klondike app", 0);

    if (!app_state) {
        return E_NOT_ENOUGH_MEMORY;
    }

    init_window();
    init_game();

    restart_game();

    app_klondike.main_window = &app_state->window;

    return E_OK;
}

global app_st app_klondike = {
    .icon = &icon_klondike,
    .init = init_app,
};
