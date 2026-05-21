// --------------------------------------------------------------------------------------
// Copyright (c) 2026 luke8086
// Distributed under the terms of GPL-2 License
// --------------------------------------------------------------------------------------
// File: freecell.c - FreeCell game
// --------------------------------------------------------------------------------------

#include <gui.h>

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

static uint8_t window_pixels[WINDOW_WIDTH * WINDOW_HEIGHT];
static surface_st window_surface;
static window_st window;

static widget_st title_bar;
static widget_st close_button;
static widget_st pile_widgets[PILE_COUNT];
static widget_st *widgets[PILE_COUNT + 2];

static card_t holds_cards[HOLD_COUNT];
static card_pile_st holds[HOLD_COUNT];

static card_t founds_cards[FOUND_COUNT];
static card_pile_st founds[FOUND_COUNT];

static card_t columns_cards[COLUMN_COUNT][COLUMN_CARDS_MAX];
static card_pile_st columns[COLUMN_COUNT];

static card_pile_st *all_piles[PILE_COUNT];

static card_game_st game;
static int state;
static int ticks_waited;

static int
remaining_cards(void)
{
    int i;
    int ret = 0;

    for (i = 0; i < HOLD_COUNT; ++i) {
        ret += holds[i].count;
    }

    for (i = 0; i < COLUMN_COUNT; ++i) {
        ret += columns[i].count;
    }

    return ret;
}

static void
deal_cards(void)
{
    card_t deck[CARD_COUNT];
    int i, col;

    card_deck_init(deck, CARD_COUNT);
    card_deck_shuffle(deck, CARD_COUNT);

    for (i = 0; i < HOLD_COUNT; ++i) {
        holds[i].count = 0;
    }

    for (i = 0; i < FOUND_COUNT; ++i) {
        founds[i].count = 0;
    }

    for (i = 0; i < COLUMN_COUNT; ++i) {
        columns[i].count = 0;
    }

    for (i = 0; i < CARD_COUNT; ++i) {
        col = i % COLUMN_COUNT;
        card_pile_push(&columns[col], deck[i]);
    }

    game.cur_move.src = NULL;

    state = STATE_DEFAULT;
}

static void
draw_piles(void)
{
    int i;

    for (i = 0; i < HOLD_COUNT; ++i) {
        card_pile_draw(&game, &holds[i]);
    }

    for (i = 0; i < FOUND_COUNT; ++i) {
        card_pile_draw(&game, &founds[i]);
    }

    for (i = 0; i < COLUMN_COUNT; ++i) {
        card_pile_draw(&game, &columns[i]);
    }
}

static void
update_status(void)
{
    int remaining = remaining_cards();

    if (state == STATE_WON) {
        gui_status_set("You won! Press R to restart");
    } else {
        gui_status_set("Remaining: %d  \xb3  R: restart", remaining);
    }
}

static void
check_win(void)
{
    int i;

    for (i = 0; i < FOUND_COUNT; ++i) {
        if (founds[i].count == 0 || CARD_RANK(CARD_PILE_TOP(&founds[i])) != 12) {
            return;
        }
    }

    state = STATE_WON;
    update_status();
}

static void
start_move(card_pile_st *pile, int count)
{
    game.cur_move.src = pile;
    game.cur_move.count = count;
    card_pile_draw(&game, pile);
    update_status();
}

static void
cancel_move(void)
{
    card_pile_st *old = game.cur_move.src;

    game.cur_move.src = NULL;

    if (old) {
        card_pile_draw(&game, old);
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
    int rank = CARD_RANK(card);
    int suit = CARD_SUIT(card);
    int color = CARD_COLOR(card);
    int i;

    if (founds[suit].count == 0) {
        if (rank != 0) {
            return 0;
        }
    } else if (rank != CARD_RANK(CARD_PILE_TOP(&founds[suit])) + 1) {
        return 0;
    }

    if (rank <= 1) {
        return 1;
    }

    for (i = 0; i < FOUND_COUNT; ++i) {
        if (CARD_COLOR(i * 13) == color) {
            continue;
        }

        if (founds[i].count == 0 || CARD_RANK(CARD_PILE_TOP(&founds[i])) < rank - 1) {
            return 0;
        }
    }

    return 1;
}

static void
set_auto_move(card_pile_st *src, card_pile_st *dst)
{
    game.cur_move.src = src;
    game.cur_move.dst = dst;
    game.cur_move.count = 1;
    state = STATE_AUTO_PENDING;
    ticks_waited = 0;
}

static void
check_auto_move(void)
{
    int i;
    card_t card;

    for (i = 0; i < HOLD_COUNT; ++i) {
        card = CARD_PILE_TOP(&holds[i]);

        if (card != CARD_EMPTY && card_should_auto_promote(card)) {
            set_auto_move(&holds[i], &founds[CARD_SUIT(card)]);
            return;
        }
    }

    for (i = 0; i < COLUMN_COUNT; ++i) {
        card = CARD_PILE_TOP(&columns[i]);

        if (card != CARD_EMPTY && card_should_auto_promote(card)) {
            set_auto_move(&columns[i], &founds[CARD_SUIT(card)]);
            return;
        }
    }
}

static void
exec_move(void)
{
    card_game_exec_cur_move(&game);
    update_status();
    check_win();

    if (state != STATE_WON) {
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
    int i;
    int avail_holds = 0;
    int avail_cols = 0;

    for (i = 0; i < HOLD_COUNT; ++i) {
        if (holds[i].count == 0) {
            ++avail_holds;
        }
    }

    for (i = 0; i < COLUMN_COUNT; ++i) {
        if (columns[i].count == 0 && dst != &columns[i]) {
            ++avail_cols;
        }
    }

    return (1 + avail_holds) * (1 + avail_cols);
}

static void
request_move_to_hold(void)
{
    if (game.cur_move.count != 1) {
        show_error("Invalid move");
        return;
    }

    if (game.cur_move.dst->count > 0) {
        show_error("Cell not empty");
        return;
    }

    exec_move();
}

static void
request_move_to_found(void)
{
    int expected_rank;
    card_t card;
    card_pile_st *found;

    if (game.cur_move.count != 1) {
        show_error("Invalid move");
        return;
    }

    card = CARD_PILE_TOP(game.cur_move.src);
    found = &founds[CARD_SUIT(card)];
    expected_rank = (found->count == 0) ? 0 : CARD_RANK(CARD_PILE_TOP(found)) + 1;

    if (CARD_RANK(card) != expected_rank) {
        show_error("Invalid move");
        return;
    }

    game.cur_move.dst = found;
    exec_move();
}

static void
request_move_to_nonempty_col(void)
{
    card_t dst_top, src_card;
    int n;

    if (game.cur_move.src->type == PILE_HOLDS) {
        src_card = CARD_PILE_TOP(game.cur_move.src);
        dst_top = CARD_PILE_TOP(game.cur_move.dst);

        if (CARD_RANK(dst_top) != CARD_RANK(src_card) + 1 ||
            CARD_COLOR(dst_top) == CARD_COLOR(src_card)) {
            show_error("Invalid move");
            return;
        }
    } else if (game.cur_move.src->type == PILE_COLUMNS) {
        n = game.cur_move.count;
        src_card = game.cur_move.src->cards[game.cur_move.src->count - n];
        dst_top = CARD_PILE_TOP(game.cur_move.dst);

        if (n > get_max_valid_sequence_len(game.cur_move.src) ||
            n > get_max_movable_cards_count(game.cur_move.dst) ||
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
    int max_seq_len, max_movable;

    if (game.cur_move.src->type == PILE_HOLDS) {
        exec_move();
        return;
    }

    ASSERT(game.cur_move.src->type == PILE_COLUMNS);

    max_seq_len = get_max_valid_sequence_len(game.cur_move.src);
    max_movable = MIN(max_seq_len, get_max_movable_cards_count(game.cur_move.dst));

    if (game.cur_move.count > max_movable) {
        show_error("Invalid move");
        return;
    }

    exec_move();
}

static void
request_move(void)
{
    card_pile_st *dst = game.cur_move.dst;

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
    card_pile_draw(&game, all_piles[widget->tag1]);
}

static void
on_pile_pointer_up(widget_st *widget, event_st event _unsd, point_st pos)
{
    card_pile_st *pile;
    int idx, count;

    if (state != STATE_DEFAULT) {
        return;
    }

    pile = all_piles[widget->tag1];

    if (game.cur_move.src == NULL) {
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
    } else if (pile == game.cur_move.src) {
        cancel_move();
    } else {
        game.cur_move.dst = pile;
        request_move();
    }
}

static void
on_pile_pointer_alt(widget_st *widget, event_st event _unsd, point_st pos _unsd)
{
    card_pile_st *pile = all_piles[widget->tag1];

    if (state != STATE_DEFAULT) {
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
    if (state == STATE_AUTO_PENDING) {
        return;
    }

    if (event.key_code == KEY_R) {
        restart_game();
    }
}

static void
on_tick(window_st *win _unsd)
{
    if (state != STATE_AUTO_PENDING) {
        return;
    }

    ++ticks_waited;

    if (ticks_waited == AUTO_MOVE_HIGHLIGHT_TICKS) {
        card_pile_draw(&game, game.cur_move.src);
        return;
    }

    if (ticks_waited > AUTO_MOVE_EXECUTE_TICKS) {
        state = STATE_DEFAULT;
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
    int x0 = PAD_X;
    int step = CARD_WIDTH + GAP_X;
    int i;

    game.surface = &window_surface;
    game.card_width = CARD_WIDTH;
    game.card_height = CARD_HEIGHT;
    game.card_step = COLUMN_STEP_MAX;

    for (i = 0; i < PILE_COUNT; ++i) {
        pile_widgets[i].type = WIDGET_TYPE_CUSTOM;
        pile_widgets[i].draw = pile_widget_draw;
        pile_widgets[i].on_pointer_up = on_pile_pointer_up;
        pile_widgets[i].on_pointer_alt = on_pile_pointer_alt;
        pile_widgets[i].tag1 = i;
    }

    for (i = 0; i < HOLD_COUNT; ++i) {
        holds[i].type = PILE_HOLDS;
        holds[i].index = i;
        holds[i].capacity = 1;
        holds[i].cards = &holds_cards[i];
        holds[i].widget = &pile_widgets[IDX_HOLDS_FIRST + i];
        holds[i].widget->rect = gui_rect_make(x0 + i * step, HOLDS_Y,
            CARD_WIDTH, CARD_HEIGHT);
        all_piles[IDX_HOLDS_FIRST + i] = &holds[i];
    }

    for (i = 0; i < FOUND_COUNT; ++i) {
        founds[i].type = PILE_FOUNDS;
        founds[i].index = i;
        founds[i].capacity = 1;
        founds[i].cards = &founds_cards[i];
        founds[i].replace_on_push = 1;
        founds[i].widget = &pile_widgets[IDX_FOUNDS_FIRST + i];
        founds[i].widget->rect = gui_rect_make(x0 + 2 * GAP_X + (i + HOLD_COUNT) * step,
            HOLDS_Y, CARD_WIDTH, CARD_HEIGHT);
        all_piles[IDX_FOUNDS_FIRST + i] = &founds[i];
    }

    for (i = 0; i < COLUMN_COUNT; ++i) {
        columns[i].type = PILE_COLUMNS;
        columns[i].index = i;
        columns[i].capacity = COLUMN_CARDS_MAX;
        columns[i].cards = columns_cards[i];
        columns[i].is_cascade = 1;
        columns[i].widget = &pile_widgets[IDX_COLUMNS_FIRST + i];
        columns[i].widget->rect = gui_rect_make(x0 + GAP_X + i * step, COLUMNS_Y,
            CARD_WIDTH, COLUMNS_H);
        all_piles[IDX_COLUMNS_FIRST + i] = &columns[i];
    }

    for (i = 0; i < PILE_COUNT; ++i) {
        gui_window_add_widget(&window, &pile_widgets[i]);
    }
}

static void
init_window(void)
{
    window_surface.size.width = WINDOW_WIDTH;
    window_surface.size.height = WINDOW_HEIGHT;
    window_surface.pitch = WINDOW_WIDTH;
    window_surface.pixels = window_pixels;

    window.surface = &window_surface;
    window.title = "FreeCell";
    window.bg_color = COLOR_WIDGET_BG;
    window.widgets = widgets;
    window.widgets_capacity = sizeof(widgets) / sizeof(widgets[0]);
    window.on_active_change = on_active_change;
    window.on_key_down = on_key_down;
    window.on_tick = on_tick;

    gui_window_init_frame(&window, &title_bar, &close_button);
}

static void
show_app(void)
{
    static int initialized = 0;

    if (!initialized) {
        init_window();
        init_game();
        initialized = 1;
    }

    restart_game();

    gui_wm_add_window(&window);
}

global app_st app_freecell = {
    .icon = &icon_freecell,
    .show = show_app,
};
