/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: blackjack.c - Blackjack game
 */

#include <gui.h>

enum {
    SPACING = 10,

    CARD_WIDTH = 30,
    CARD_HEIGHT = 45,
    CARD_SPACING = 4,
    CARDS_X = SPACING,
    CARDS_MAX = 6,
    CARDS_WIDTH = CARDS_MAX * CARD_WIDTH + (CARDS_MAX - 1) * CARD_SPACING,
    CARDS_DEALER_Y = TITLE_BAR_HEIGHT + SPACING,
    DIVIDER_Y = CARDS_DEALER_Y + CARD_HEIGHT + SPACING,
    CARDS_PLAYER_Y = DIVIDER_Y + 1 + SPACING,

    WINDOW_WIDTH = CARDS_X + CARDS_WIDTH + SPACING,

    BUTTON_WIDTH = 60,
    BUTTON_HEIGHT = 28,
    BUTTON_SPACING = SPACING * 2,
    BUTTONS_Y = CARDS_PLAYER_Y + CARD_HEIGHT + SPACING,
    BUTTONS_TOTAL_W = 2 * BUTTON_WIDTH + BUTTON_SPACING,
    BUTTON_HIT_X = (WINDOW_WIDTH - BUTTONS_TOTAL_W) / 2,
    BUTTON_STAND_X = BUTTON_HIT_X + BUTTON_WIDTH + BUTTON_SPACING,
    BUTTON_DEAL_X = (WINDOW_WIDTH - BUTTON_WIDTH) / 2,

    WINDOW_HEIGHT = BUTTONS_Y + BUTTON_HEIGHT + SPACING,

    HAND_SIZE_MAX = 11, /* 4*A + 4*2 + 3*3 */
};

enum {
    STATE_PLAYING = 0,
    STATE_OVER = 1,
};

typedef struct {
    uint8_t window_pixels[WINDOW_WIDTH * WINDOW_HEIGHT];
    surface_st window_surface;
    window_st window;

    widget_st title_bar;
    widget_st close_button;
    widget_st hit_button;
    widget_st stand_button;
    widget_st deal_button;
    widget_st *widgets[5];

    card_game_st game;

    card_t deck[CARD_DECK_SIZE];
    int deck_pos;

    card_t player_hand[HAND_SIZE_MAX];
    int player_hand_count;

    card_t dealer_hand[HAND_SIZE_MAX];
    int dealer_hand_count;

    const char *status_msg;
    int game_state;
    int wins;
    int losses;
} app_state_st;

static app_state_st *app_state = NULL;

static void
shuffle_deck(void)
{
    app_state_st *a = app_state;

    a->deck_pos = 0;
    card_deck_init(a->deck, CARD_DECK_SIZE);
    card_deck_shuffle(a->deck, CARD_DECK_SIZE);
}

static card_t
deal_card(void)
{
    app_state_st *a = app_state;

    return a->deck[a->deck_pos++];
}

static int
hand_score(card_t *hand, int count)
{
    int score = 0;
    int aces = 0;

    for (int i = 0; i < count; ++i) {
        int rank = CARD_RANK(hand[i]);

        if (rank == 0) {
            score += 11;
            aces++;
        } else if (rank >= 10) {
            score += 10;
        } else {
            score += rank + 1;
        }
    }

    while (score > 21 && aces > 0) {
        score -= 10;
        aces--;
    }

    return score;
}

static int
is_blackjack(card_t *hand, int count)
{
    return count == 2 && hand_score(hand, count) == 21;
}

static void
draw_card(int x, int y, card_t card, int face_up)
{
    app_state_st *a = app_state;

    if (face_up) {
        card_draw(&a->game, x, y, card, 0);
    } else {
        card_back_draw(&a->game, x, y);
    }
}

static void
draw_hand(card_t *hand)
{
    app_state_st *a = app_state;
    int is_player = (hand == a->player_hand);
    int count = is_player ? a->player_hand_count : a->dealer_hand_count;
    int y = is_player ? CARDS_PLAYER_Y : CARDS_DEALER_Y;
    int all_face_up = is_player || a->game_state == STATE_OVER;

    rect_st r = gui_rect_make(CARDS_X, y, CARDS_WIDTH, CARD_HEIGHT);
    gui_surface_draw_rect(a->window.surface, r, COLOR_WIDGET_BG);

    int step = CARD_WIDTH + CARD_SPACING;
    if (count > 1) {
        int max_step = (CARDS_WIDTH - CARD_WIDTH) / (count - 1);
        step = MIN(step, max_step);
    }

    for (int i = 0; i < count; i++) {
        int x = CARDS_X + i * step;
        draw_card(x, y, hand[i], all_face_up || i == 0);
    }

    gui_wm_render_window_region(&a->window, r);
}

static void
update_buttons(void)
{
    app_state_st *a = app_state;

    a->hit_button.hidden = a->game_state == STATE_OVER;
    a->stand_button.hidden = a->game_state == STATE_OVER;
    a->deal_button.hidden = a->game_state == STATE_PLAYING;

    rect_st r = gui_rect_make(1, BUTTONS_Y, WINDOW_WIDTH - 2, BUTTON_HEIGHT);
    gui_surface_draw_rect(a->window.surface, r, COLOR_WIDGET_BG);

    gui_widget_draw(&a->hit_button);
    gui_widget_draw(&a->stand_button);
    gui_widget_draw(&a->deal_button);

    gui_wm_render_window_region(&a->window, r);
}

static void
update_status(void)
{
    app_state_st *a = app_state;
    int player_score = hand_score(a->player_hand, a->player_hand_count);
    int dealer_score = hand_score(a->dealer_hand, a->dealer_hand_count);

    if (a->game_state == STATE_PLAYING) {
        gui_status_set("Dealer: ?  You:%2d  \xb3  W:%d  L:%d",
        player_score, a->wins, a->losses);
    } else {
        gui_status_set("Dealer:%2d  You:%2d  \xb3  %s  \xb3  W:%d  L:%d",
            dealer_score, player_score, a->status_msg, a->wins, a->losses);
    }
}

static void
end_game(const char *msg)
{
    app_state_st *a = app_state;

    a->game_state = STATE_OVER;
    a->status_msg = msg;

    draw_hand(a->dealer_hand);
    update_buttons();
    update_status();
}

static void
restart_game(void)
{
    app_state_st *a = app_state;

    shuffle_deck();

    a->player_hand_count = 0;
    a->dealer_hand_count = 0;

    a->player_hand[a->player_hand_count++] = deal_card();
    a->dealer_hand[a->dealer_hand_count++] = deal_card();
    a->player_hand[a->player_hand_count++] = deal_card();
    a->dealer_hand[a->dealer_hand_count++] = deal_card();

    a->game_state = STATE_PLAYING;
    update_buttons();
    draw_hand(a->player_hand);
    draw_hand(a->dealer_hand);

    int player_blackjack = is_blackjack(a->player_hand, a->player_hand_count);
    int dealer_blackjack = is_blackjack(a->dealer_hand, a->dealer_hand_count);

    if (player_blackjack && dealer_blackjack) {
        end_game("Both Blackjack! Push");
    } else if (player_blackjack) {
        a->wins++;
        end_game("Blackjack! You win!");
    } else if (dealer_blackjack) {
        a->losses++;
        end_game("Dealer Blackjack!");
    } else {
        update_status();
    }
}

static void
player_stand(void)
{
    app_state_st *a = app_state;
    int dealer_score = hand_score(a->dealer_hand, a->dealer_hand_count);
    int player_score = hand_score(a->player_hand, a->player_hand_count);

    while (dealer_score < 17 && a->dealer_hand_count < HAND_SIZE_MAX) {
        a->dealer_hand[a->dealer_hand_count++] = deal_card();
        dealer_score = hand_score(a->dealer_hand, a->dealer_hand_count);
    }

    if (dealer_score > 21) {
        a->wins++;
        end_game("Dealer busts! You win!");
    } else if (player_score > dealer_score) {
        a->wins++;
        end_game("You win!");
    } else if (dealer_score > player_score) {
        a->losses++;
        end_game("Dealer wins");
    } else {
        end_game("Push!");
    }
}

static void
player_hit(void)
{
    app_state_st *a = app_state;

    if (a->player_hand_count >= HAND_SIZE_MAX) {
        return;
    }

    a->player_hand[a->player_hand_count++] = deal_card();
    draw_hand(a->player_hand);

    int score = hand_score(a->player_hand, a->player_hand_count);

    if (score > 21) {
        a->losses++;
        end_game("Bust! You lose");
    } else if (score == 21) {
        player_stand();
    } else {
        update_status();
    }
}

static void
on_hit_button(widget_st *widget, event_st event, point_st pos)
{
    app_state_st *a = app_state;

    gui_button_on_pointer_up(widget, event, pos);

    if (a->game_state == STATE_PLAYING) {
        player_hit();
    }
}

static void
on_stand_button(widget_st *widget, event_st event, point_st pos)
{
    app_state_st *a = app_state;

    gui_button_on_pointer_up(widget, event, pos);

    if (a->game_state == STATE_PLAYING) {
        player_stand();
    }
}

static void
on_deal_button(widget_st *widget, event_st event, point_st pos)
{
    app_state_st *a = app_state;

    gui_button_on_pointer_up(widget, event, pos);

    if (a->game_state == STATE_OVER) {
        restart_game();
    }
}

static void
draw_window(window_st *window)
{
    app_state_st *a = app_state;

    gui_window_draw(window, COLOR_WIDGET_BG);
    gui_surface_draw_h_seg(window->surface, 1, DIVIDER_Y, WINDOW_WIDTH - 2, COLOR_BORDER);

    draw_hand(a->player_hand);
    draw_hand(a->dealer_hand);

    gui_widget_draw(&a->hit_button);
    gui_widget_draw(&a->stand_button);
    gui_widget_draw(&a->deal_button);
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
    app_blackjack.main_window = NULL;

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
    a->window.title = "Blackjack";
    a->window.widgets = a->widgets;
    a->window.widgets_capacity = sizeof(a->widgets) / sizeof(a->widgets[0]);
    a->window.draw = draw_window;
    a->window.on_active_change = on_active_change;
    a->window.on_close = close_window;

    a->game.surface = &a->window_surface;
    a->game.card_width = CARD_WIDTH;
    a->game.card_height = CARD_HEIGHT;

    gui_window_init_frame(&a->window, &a->title_bar, &a->close_button);
}

static void
init_buttons(void)
{
    app_state_st *a = app_state;

    gui_button_init(&a->hit_button);
    a->hit_button.rect = gui_rect_make(BUTTON_HIT_X, BUTTONS_Y, BUTTON_WIDTH, BUTTON_HEIGHT);
    a->hit_button.label = "Hit";
    a->hit_button.on_pointer_up = on_hit_button;
    gui_window_add_widget(&a->window, &a->hit_button);

    gui_button_init(&a->stand_button);
    a->stand_button.rect = gui_rect_make(BUTTON_STAND_X, BUTTONS_Y, BUTTON_WIDTH,
        BUTTON_HEIGHT);
    a->stand_button.label = "Stand";
    a->stand_button.on_pointer_up = on_stand_button;
    gui_window_add_widget(&a->window, &a->stand_button);

    gui_button_init(&a->deal_button);
    a->deal_button.rect = gui_rect_make(BUTTON_DEAL_X, BUTTONS_Y, BUTTON_WIDTH,
        BUTTON_HEIGHT);
    a->deal_button.label = "Deal";
    a->deal_button.hidden = 1;
    a->deal_button.on_pointer_up = on_deal_button;
    gui_window_add_widget(&a->window, &a->deal_button);
}

static int
init_app(void)
{
    ASSERT(!app_state);

    app_state = heap_alloc(sizeof(app_state_st), "Blackjack app", 0);

    if (!app_state) {
        return E_NOT_ENOUGH_MEMORY;
    }

    init_window();
    init_buttons();

    restart_game();

    app_blackjack.main_window = &app_state->window;

    return E_OK;
}

global app_st app_blackjack = {
    .icon = &icon_blackjack,
    .init = init_app,
};
