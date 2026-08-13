/* feat/card.c */
extern const char *card_rank_str[CARD_RANK_COUNT];
extern const char *card_suit_str[CARD_SUIT_COUNT];
extern void card_deck_init(card_t *deck, int n);
extern void card_deck_shuffle(card_t *deck, int n);
extern void card_pile_update_step(card_game_st *game, card_pile_st *p);
extern int card_pile_get_card_index_by_ypos(card_pile_st *p, int ypos);
extern card_t card_pile_pop(card_pile_st *p);
extern void card_pile_push(card_pile_st *p, card_t card);
extern void card_pile_uncover_top(card_pile_st *p);
extern int card_pile_top_y(card_pile_st *p);
extern void card_draw(card_game_st *game, int x, int y, card_t card, int selected);
extern void card_stub_draw(card_game_st *game, int x, int y, int height, card_t card, int selected);
extern void card_back_draw(card_game_st *game, int x, int y);
extern void card_back_stub_draw(card_game_st *game, int x, int y, int height);
extern void card_pile_draw(card_game_st *game, card_pile_st *p);
extern void card_game_exec_cur_move(card_game_st *game);
