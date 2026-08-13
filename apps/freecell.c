/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: freecell.c - FreeCell game
 */

#include <feat.h>

enum {
    CARD_WIDTH = 38,
    CARD_HEIGHT = 45,

    COLUMN_STEP_MAX = 11,
    COLUMN_CARDS_MAX = 21,

    CARD_COUNT = 52,
    HOLD_COUNT = 4,
    FOUND_COUNT = 4,
    COLUMN_COUNT = 8,
    PILE_COUNT = HOLD_COUNT + FOUND_COUNT + COLUMN_COUNT,

    PAD_X = 8,
    PAD_Y = 8,
    GAP_X = 4,
    GAP_Y = 8,
    HOLDS_Y = TITLE_BAR_HEIGHT + PAD_Y,
    COLUMNS_Y = HOLDS_Y + CARD_HEIGHT + GAP_Y,
    COLUMNS_H = 200,

    WINDOW_WIDTH = 2 * PAD_X + COLUMN_COUNT * CARD_WIDTH + (COLUMN_COUNT + 1) * GAP_X,
    WINDOW_HEIGHT = COLUMNS_Y + COLUMNS_H + PAD_Y,

    PILE_HOLDS = 1,
    PILE_FOUNDS = 2,
    PILE_COLUMNS = 3,

    IDX_HOLDS_FIRST = 0,
    IDX_FOUNDS_FIRST = IDX_HOLDS_FIRST + HOLD_COUNT,
    IDX_COLUMNS_FIRST = IDX_FOUNDS_FIRST + FOUND_COUNT,

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

    card_t holds_cards[HOLD_COUNT];
    card_pile_st holds[HOLD_COUNT];

    card_t founds_cards[FOUND_COUNT];
    card_pile_st founds[FOUND_COUNT];

    card_t columns_cards[COLUMN_COUNT][COLUMN_CARDS_MAX];
    card_pile_st columns[COLUMN_COUNT];

    card_pile_st *all_piles[PILE_COUNT];

    card_game_st game;
    int state;
    int ticks_waited;
} app_state_st;

static app_state_st *app_state = NULL;

static int
remaining_cards(void)
{
    app_state_st *a = app_state;
    int i;
    int ret = 0;

    for (i = 0; i < HOLD_COUNT; ++i) {
        ret += a->holds[i].count;
    }

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
    int i, col;

    card_deck_init(deck, CARD_COUNT);
    card_deck_shuffle(deck, CARD_COUNT);

    for (i = 0; i < HOLD_COUNT; ++i) {
        a->holds[i].count = 0;
    }

    for (i = 0; i < FOUND_COUNT; ++i) {
        a->founds[i].count = 0;
    }

    for (i = 0; i < COLUMN_COUNT; ++i) {
        a->columns[i].count = 0;
    }

    for (i = 0; i < CARD_COUNT; ++i) {
        col = i % COLUMN_COUNT;
        card_pile_push(&a->columns[col], deck[i]);
    }

    a->game.cur_move.src = NULL;

    a->state = STATE_DEFAULT;
}

static void
draw_piles(void)
{
    app_state_st *a = app_state;
    int i;

    for (i = 0; i < HOLD_COUNT; ++i) {
        card_pile_draw(&a->game, &a->holds[i]);
    }

    for (i = 0; i < FOUND_COUNT; ++i) {
        card_pile_draw(&a->game, &a->founds[i]);
    }

    for (i = 0; i < COLUMN_COUNT; ++i) {
        card_pile_draw(&a->game, &a->columns[i]);
    }
}

static void
draw_window(window_st *window)
{
    gui_window_draw(window, COLOR_WIDGET_BG);
    draw_piles();
}

static void
update_status(void)
{
    app_state_st *a = app_state;
    int remaining = remaining_cards();

    if (a->state == STATE_WON) {
        gui_status_set("You won! Press R to restart");
    } else {
        gui_status_set("Remaining: %d  \xb3  R: Restart", remaining);
    }
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

    for (i = 0; i < HOLD_COUNT; ++i) {
        card = CARD_PILE_TOP(&a->holds[i]);

        if (card != CARD_EMPTY && card_should_auto_promote(card)) {
            set_auto_move(&a->holds[i], &a->founds[CARD_SUIT(card)]);
            return;
        }
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

static int
get_max_valid_sequence_len(card_pile_st *p)
{
    int count, i;
    card_t curr, prev;

    count = p->count;

    if (count == 0) {
        return 0;
    }

    for (i = count - 1; i > 0; --i) {
        curr = p->cards[i];
        prev = p->cards[i - 1];

        if (CARD_RANK(prev) != CARD_RANK(curr) + 1) {
            break;
        }

        if (CARD_COLOR(prev) == CARD_COLOR(curr)) {
            break;
        }
    }

    return count - i;
}

static int
get_max_movable_cards_count(card_pile_st *dst)
{
    app_state_st *a = app_state;
    int i;
    int avail_holds = 0;
    int avail_cols = 0;

    for (i = 0; i < HOLD_COUNT; ++i) {
        if (a->holds[i].count == 0) {
            ++avail_holds;
        }
    }

    for (i = 0; i < COLUMN_COUNT; ++i) {
        if (a->columns[i].count == 0 && dst != &a->columns[i]) {
            ++avail_cols;
        }
    }

    return (1 + avail_holds) * (1 + avail_cols);
}

static void
request_move_to_hold(void)
{
    app_state_st *a = app_state;

    if (a->game.cur_move.count != 1) {
        show_error("Invalid move");
        return;
    }

    if (a->game.cur_move.dst->count > 0) {
        show_error("Cell not empty");
        return;
    }

    exec_move();
}

static void
request_move_to_found(void)
{
    app_state_st *a = app_state;
    int expected_rank;
    card_t card;
    card_pile_st *found;

    if (a->game.cur_move.count != 1) {
        show_error("Invalid move");
        return;
    }

    card = CARD_PILE_TOP(a->game.cur_move.src);
    found = &a->founds[CARD_SUIT(card)];
    expected_rank = (found->count == 0) ? 0 : CARD_RANK(CARD_PILE_TOP(found)) + 1;

    if (CARD_RANK(card) != expected_rank) {
        show_error("Invalid move");
        return;
    }

    a->game.cur_move.dst = found;
    exec_move();
}

static void
request_move_to_nonempty_col(void)
{
    app_state_st *a = app_state;
    card_t dst_top, src_card;
    int n;

    if (a->game.cur_move.src->type == PILE_HOLDS) {
        src_card = CARD_PILE_TOP(a->game.cur_move.src);
        dst_top = CARD_PILE_TOP(a->game.cur_move.dst);

        if (CARD_RANK(dst_top) != CARD_RANK(src_card) + 1 ||
            CARD_COLOR(dst_top) == CARD_COLOR(src_card)) {
            show_error("Invalid move");
            return;
        }
    } else if (a->game.cur_move.src->type == PILE_COLUMNS) {
        n = a->game.cur_move.count;
        src_card = a->game.cur_move.src->cards[a->game.cur_move.src->count - n];
        dst_top = CARD_PILE_TOP(a->game.cur_move.dst);

        if (n > get_max_valid_sequence_len(a->game.cur_move.src) ||
            n > get_max_movable_cards_count(a->game.cur_move.dst) ||
            CARD_RANK(dst_top) != CARD_RANK(src_card) + 1 ||
            CARD_COLOR(dst_top) == CARD_COLOR(src_card)) {
            show_error("Invalid move");
            return;
        }
    }

    exec_move();
}

static void
request_move_to_empty_col(void)
{
    app_state_st *a = app_state;
    int max_seq_len, max_movable;

    if (a->game.cur_move.src->type == PILE_HOLDS) {
        exec_move();
        return;
    }

    ASSERT(a->game.cur_move.src->type == PILE_COLUMNS);

    max_seq_len = get_max_valid_sequence_len(a->game.cur_move.src);
    max_movable = MIN(max_seq_len, get_max_movable_cards_count(a->game.cur_move.dst));

    if (a->game.cur_move.count > max_movable) {
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

    if (dst->type == PILE_HOLDS) {
        request_move_to_hold();
    } else if (dst->type == PILE_FOUNDS) {
        request_move_to_found();
    } else if (dst->count == 0) {
        request_move_to_empty_col();
    } else {
        request_move_to_nonempty_col();
    }
}

static void
restart_game(void)
{
    deal_cards();
    draw_piles();
    update_status();
}

static void
pile_widget_draw(widget_st *widget)
{
    app_state_st *a = app_state;

    card_pile_draw(&a->game, a->all_piles[widget->tag1]);
}

static void
on_pile_pointer_up(widget_st *widget, event_st event _unsd, point_st pos)
{
    app_state_st *a = app_state;
    card_pile_st *pile;
    int idx, count;

    if (a->state != STATE_DEFAULT) {
        return;
    }

    pile = a->all_piles[widget->tag1];

    if (a->game.cur_move.src == NULL) {
        if (pile->type == PILE_FOUNDS || pile->count == 0) {
            return;
        }

        idx = card_pile_get_card_index_by_ypos(pile, pos.y - widget->rect.y);
        count = pile->count - idx;

        if (count > get_max_valid_sequence_len(pile)) {
            show_error("Invalid sequence");
            return;
        }

        start_move(pile, count);
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
    card_pile_st *pile = a->all_piles[widget->tag1];

    if (a->state != STATE_DEFAULT) {
        return;
    }

    if (pile->type != PILE_HOLDS && pile->type != PILE_COLUMNS) {
        return;
    }

    if (pile->count == 0) {
        return;
    }

    cancel_move();
    start_move(pile, 1);
    request_move_to_found();
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

    if (a->ticks_waited > AUTO_MOVE_EXECUTE_TICKS) {
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
    int x0 = PAD_X;
    int step = CARD_WIDTH + GAP_X;
    int i;

    a->game.surface = &a->window_surface;
    a->game.card_width = CARD_WIDTH;
    a->game.card_height = CARD_HEIGHT;
    a->game.card_step = COLUMN_STEP_MAX;

    for (i = 0; i < PILE_COUNT; ++i) {
        a->pile_widgets[i].draw = pile_widget_draw;
        a->pile_widgets[i].on_pointer_up = on_pile_pointer_up;
        a->pile_widgets[i].on_pointer_alt = on_pile_pointer_alt;
        a->pile_widgets[i].tag1 = i;
    }

    for (i = 0; i < HOLD_COUNT; ++i) {
        a->holds[i].type = PILE_HOLDS;
        a->holds[i].index = i;
        a->holds[i].capacity = 1;
        a->holds[i].cards = &a->holds_cards[i];
        a->holds[i].widget = &a->pile_widgets[IDX_HOLDS_FIRST + i];
        a->holds[i].widget->rect = gui_rect_make(x0 + i * step, HOLDS_Y,
            CARD_WIDTH, CARD_HEIGHT);
        a->all_piles[IDX_HOLDS_FIRST + i] = &a->holds[i];
    }

    for (i = 0; i < FOUND_COUNT; ++i) {
        a->founds[i].type = PILE_FOUNDS;
        a->founds[i].index = i;
        a->founds[i].capacity = 1;
        a->founds[i].cards = &a->founds_cards[i];
        a->founds[i].replace_on_push = 1;
        a->founds[i].widget = &a->pile_widgets[IDX_FOUNDS_FIRST + i];
        a->founds[i].widget->rect = gui_rect_make(x0 + 2 * GAP_X + (i + HOLD_COUNT) * step,
            HOLDS_Y, CARD_WIDTH, CARD_HEIGHT);
        a->all_piles[IDX_FOUNDS_FIRST + i] = &a->founds[i];
    }

    for (i = 0; i < COLUMN_COUNT; ++i) {
        a->columns[i].type = PILE_COLUMNS;
        a->columns[i].index = i;
        a->columns[i].capacity = COLUMN_CARDS_MAX;
        a->columns[i].cards = a->columns_cards[i];
        a->columns[i].is_cascade = 1;
        a->columns[i].widget = &a->pile_widgets[IDX_COLUMNS_FIRST + i];
        a->columns[i].widget->rect = gui_rect_make(x0 + GAP_X + i * step, COLUMNS_Y,
            CARD_WIDTH, COLUMNS_H);
        a->all_piles[IDX_COLUMNS_FIRST + i] = &a->columns[i];
    }

    for (i = 0; i < PILE_COUNT; ++i) {
        gui_window_add_widget(&a->window, &a->pile_widgets[i]);
    }
}

static void
close_window(window_st *window _unsd)
{
    gui_wm_remove_window(window);
    app_freecell.main_window = NULL;

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
    a->window.title = "FreeCell";
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

    app_state = heap_alloc(sizeof(app_state_st), "FreeCell app", 0);

    if (!app_state) {
        return E_NOT_ENOUGH_MEMORY;
    }

    init_window();
    init_game();

    restart_game();

    app_freecell.main_window = &app_state->window;

    return E_OK;
}

global app_st app_freecell = {
    .icon = &icon_freecell,
    .init = init_app,
};
