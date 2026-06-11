/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: mahjong.c - Mahjong game
 */

#include <gui.h>

enum {
    TILE_W = 26,
    TILE_H = 26,
    TILE_D = 2,

    TILE_TYPE_COUNT = 36,
    TILES_PER_TYPE = 4,
    TILE_EMPTY = 0,
    TILE_COUNT = 144,

    BOARD_LAYERS = 4,
    BOARD_COLS = 14,
    BOARD_ROWS = 8,

    BOARD_MARGIN = 0,
    BOARD_X = 1 + BOARD_MARGIN,
    BOARD_Y = TITLE_BAR_HEIGHT + BOARD_MARGIN,
    BOARD_W = BOARD_COLS * (TILE_W - 1) + 1 + TILE_D,
    BOARD_H = BOARD_ROWS * (TILE_H - 1) + 1 + TILE_D,

    WINDOW_WIDTH = BOARD_X + BOARD_W + BOARD_MARGIN + 1,
    WINDOW_HEIGHT = BOARD_Y + BOARD_H + BOARD_MARGIN + 1,

    STATE_DEFAULT = 0,
    STATE_WON = 1,
    STATE_STUCK = 2,
};

static bitmap_st *tile_bitmaps[] = {
    NULL,
    &sprite_mj_ci_1, &sprite_mj_ci_2, &sprite_mj_ci_3, &sprite_mj_ci_4,
    &sprite_mj_ci_5, &sprite_mj_ci_6, &sprite_mj_ci_7, &sprite_mj_ci_8, &sprite_mj_ci_9,
    &glyph_mn_bbchick, &sprite_mj_ba_2, &sprite_mj_ba_3, &sprite_mj_ba_4,
    &sprite_mj_ba_5, &sprite_mj_ba_6, &sprite_mj_ba_7, &sprite_mj_ba_8, &sprite_mj_ba_9,
    &glyph_mn_num_1, &glyph_mn_num_2, &glyph_mn_num_3, &glyph_mn_num_4,
    &glyph_mn_num_5, &glyph_mn_num_6, &glyph_mn_num_7, &glyph_mn_num_8, &glyph_mn_num_9,
    &glyph_mn_east, &glyph_mn_south, &glyph_mn_west, &glyph_mn_north,
    &glyph_mn_white, &glyph_mn_issue, &glyph_mn_central,
    &glyph_mn_tulip, &glyph_mn_herb,
};

/* Layer 0: 88, Layer 1: 44, Layer 2: 8, Layer 3: 4 */
static const uint8_t board_layout[BOARD_LAYERS][BOARD_ROWS][BOARD_COLS] = {
    {
        {0,0,0,1,1,1,1,1,1,1,1,0,0,0},
        {0,0,1,1,1,1,1,1,1,1,1,1,0,0},
        {0,1,1,1,1,1,1,1,1,1,1,1,1,0},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {0,1,1,1,1,1,1,1,1,1,1,1,1,0},
        {0,0,1,1,1,1,1,1,1,1,1,1,0,0},
        {0,0,0,1,1,1,1,1,1,1,1,0,0,0},
    },
    {
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,1,1,1,1,1,1,0,0,0,0},
        {0,0,0,1,1,1,1,1,1,1,1,0,0,0},
        {0,0,0,1,1,1,1,1,1,1,1,0,0,0},
        {0,0,0,1,1,1,1,1,1,1,1,0,0,0},
        {0,0,0,1,1,1,1,1,1,1,1,0,0,0},
        {0,0,0,0,1,1,1,1,1,1,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    },
    {
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,1,1,1,1,0,0,0,0,0},
        {0,0,0,0,0,1,1,1,1,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    },
    {
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,1,1,0,0,0,0,0,0},
        {0,0,0,0,0,0,1,1,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    },
};

static uint8_t board[BOARD_LAYERS][BOARD_ROWS][BOARD_COLS];
static uint8_t dirty[BOARD_LAYERS][BOARD_ROWS][BOARD_COLS];

static int sel_col;
static int sel_row;
static int sel_layer;

static int remaining_pairs;
static int valid_moves;
static int state;

static uint8_t window_pixels[WINDOW_WIDTH * WINDOW_HEIGHT];
static surface_st window_surface;
static window_st window;

static widget_st title_bar;
static widget_st close_button;
static widget_st board_widget;
static widget_st *widgets[3];

static void
update_status(void)
{
    if (state == STATE_WON) {
        gui_status_set("You Won! Press R to play again");
    } else {
        gui_status_set("Pairs: %d  Moves: %d  |  S: Shuffle  R: Restart",
            remaining_pairs, valid_moves);
    }
}

static int
topmost_layer_at(int col, int row)
{
    int layer;

    for (layer = BOARD_LAYERS - 1; layer >= 0; layer--) {
        if (board[layer][row][col] != TILE_EMPTY) {
            return layer;
        }
    }

    return -1;
}

static int
is_tile_free(int layer, int col, int row)
{
    int left_empty, right_empty;

    if (board[layer][row][col] == TILE_EMPTY) {
        return 0;
    }

    if (layer != topmost_layer_at(col, row)) {
        return 0;
    }

    left_empty = (col == 0 || board[layer][row][col - 1] == TILE_EMPTY);
    right_empty = (col == BOARD_COLS - 1 || board[layer][row][col + 1] == TILE_EMPTY);

    return left_empty || right_empty;
}

static inline point_st
tile_pos(int layer, int col, int row)
{
    return (point_st) {
        .x = BOARD_X + col * (TILE_W - 1) - layer * TILE_D,
        .y = BOARD_Y + row * (TILE_H - 1) - layer * TILE_D,
    };
}

static int
count_valid_moves(void)
{
    uint8_t free_counts[TILE_TYPE_COUNT + 1];
    int layer, col, row;
    uint8_t type;
    int total = 0;

    memset(free_counts, 0, sizeof(free_counts));

    for (layer = 0; layer < BOARD_LAYERS; ++layer) {
        for (row = 0; row < BOARD_ROWS; ++row) {
            for (col = 0; col < BOARD_COLS; ++col) {
                if (is_tile_free(layer, col, row)) {
                    type = board[layer][row][col];
                    ++free_counts[type];
                }
            }
        }
    }

    for (type = 1; type <= TILE_TYPE_COUNT; ++type) {
        total += free_counts[type] / 2;
    }

    return total;
}

static void
draw_tile(int layer, int col, int row)
{
    uint8_t type = board[layer][row][col];
    point_st pos;
    int x, y;
    int is_selected;
    uint8_t face_color;
    uint8_t glyph_color;
    int has_right;
    int has_bottom;
    int has_diag;
    rect_st rect;

    pos = tile_pos(layer, col, row);
    x = pos.x;
    y = pos.y;

    if (type == TILE_EMPTY) {
        rect = gui_rect_make(x, y, TILE_W + TILE_D, TILE_H + TILE_D);
        gui_surface_draw_rect(window.surface, rect, COLOR_WIDGET_BG);
        gui_wm_render_window_region(&window, rect);
        return;
    }

    is_selected = (col == sel_col && row == sel_row && layer == sel_layer);
    face_color = is_selected ? COLOR_MJ_FACE_SEL_BG : COLOR_MJ_FACE_BG;
    glyph_color = is_selected ? COLOR_MJ_FACE_SEL_FG : COLOR_MJ_FACE_FG;

    has_right = (col < BOARD_COLS - 1 && board[layer][row][col + 1] != TILE_EMPTY);
    has_bottom = (row < BOARD_ROWS - 1 && board[layer][row + 1][col] != TILE_EMPTY);
    has_diag = (col < BOARD_COLS - 1 && row < BOARD_ROWS - 1 &&
        board[layer][row + 1][col + 1] != TILE_EMPTY);

    rect = gui_rect_make(x, y, TILE_W, TILE_H);
    gui_surface_draw_rect(window.surface, rect, face_color);
    gui_surface_draw_border(window.surface, rect, COLOR_MJ_EDGE);
    gui_surface_draw_bitmap_centered(window.surface, rect, tile_bitmaps[type], glyph_color);

    if (!has_right) {
        rect = gui_rect_make(x + TILE_W, y + 2, 2, TILE_H - 2);
        gui_surface_draw_rect(window.surface, rect, COLOR_MJ_EDGE);
        gui_surface_draw_h_seg(window.surface, x + TILE_W, y + 1, 1, COLOR_MJ_EDGE);
    }

    if (!has_bottom) {
        rect = gui_rect_make(x + 2, y + TILE_H, TILE_W - 2, 2);
        gui_surface_draw_rect(window.surface, rect, COLOR_MJ_EDGE);
        gui_surface_draw_h_seg(window.surface, x + 1, y + TILE_H, 1, COLOR_MJ_EDGE);
    }

    if (!has_diag) {
        rect = gui_rect_make(x + TILE_W, y + TILE_H, 2, 2);
        gui_surface_draw_rect(window.surface, rect, COLOR_MJ_EDGE);
    }

    rect = gui_rect_make(x, y, TILE_W + TILE_D, TILE_H + TILE_D);
    gui_wm_render_window_region(&window, rect);
}

static void
draw_dirty_tiles(void)
{
    int layer, row, col;

    /* Clear empty tiles */
    for (layer = 0; layer < BOARD_LAYERS; ++layer) {
        for (row = 0; row < BOARD_ROWS; ++row) {
            for (col = 0; col < BOARD_COLS; ++col) {
                if (!dirty[layer][row][col] || board[layer][row][col] != TILE_EMPTY) {
                    continue;
                }

                draw_tile(layer, col, row);
                dirty[layer][row][col] = 0;
            }
        }
    }

    /* Draw present tiles */
    for (layer = 0; layer < BOARD_LAYERS; ++layer) {
        for (row = 0; row < BOARD_ROWS; ++row) {
            for (col = 0; col < BOARD_COLS; ++col) {
                if (!dirty[layer][row][col]) {
                    continue;
                }

                draw_tile(layer, col, row);
                dirty[layer][row][col] = 0;
            }
        }
    }
}

static void
mark_tile_dirty(int layer, int col, int row)
{
    static const int8_t dc[3] = {1, 0, 1};
    static const int8_t dr[3] = {0, 1, 1};
    int i, nl, nc, nr;

    if (col >= BOARD_COLS || row >= BOARD_ROWS || board[layer][row][col] == TILE_EMPTY) {
        return;
    }

    dirty[layer][row][col] = 1;

    /*
    * Recursively mark as dirty the bottom, right and bottom-right tiles
    * in higher layers, since they're drawn over the original cell
    */
    for (i = 0; i < 3; ++i) {
        nc = col + dc[i];
        nr = row + dr[i];

        if (nc >= BOARD_COLS || nr >= BOARD_ROWS) {
            continue;
        }

        for (nl = layer + 1; nl < BOARD_LAYERS; ++nl) {
            mark_tile_dirty(nl, nc, nr);
        }
    }
}

static void
redraw_board(void)
{
    int layer, row, col;
    rect_st rect;

    rect = gui_rect_make(BOARD_X, BOARD_Y, BOARD_W, BOARD_H);
    gui_surface_draw_rect(window.surface, rect, COLOR_WIDGET_BG);

    for (layer = 0; layer < BOARD_LAYERS; ++layer) {
        for (row = 0; row < BOARD_ROWS; ++row) {
            for (col = 0; col < BOARD_COLS; ++col) {
                dirty[layer][row][col] = (board[layer][row][col] != TILE_EMPTY);
            }
        }
    }

    draw_dirty_tiles();
    gui_wm_render_window_region(&window, rect);
}

static void
remove_tile(int layer, int col, int row)
{
    int nl, nc, nr;

    board[layer][row][col] = TILE_EMPTY;
    dirty[layer][row][col] = 1;

    /* All adjacent tiles need to be redrawn */
    for (nl = 0; nl < BOARD_LAYERS; ++nl) {
        for (nr = row - 1; nr <= row + 1; ++nr) {
            for (nc = col - 1; nc <= col + 1; ++nc) {
                if (nc < 0 || nc >= BOARD_COLS || nr < 0 || nr >= BOARD_ROWS) {
                    continue;
                }

                mark_tile_dirty(nl, nc, nr);
            }
        }
    }

    draw_dirty_tiles();
}

static void
shuffle_tiles(void)
{
    uint8_t deck[TILE_COUNT];
    uint8_t tmp;
    int count = 0;
    int layer, col, row, i, j;

    if (state == STATE_WON) {
        return;
    }

    for (layer = 0; layer < BOARD_LAYERS; ++layer) {
        for (row = 0; row < BOARD_ROWS; ++row) {
            for (col = 0; col < BOARD_COLS; ++col) {
                if (board[layer][row][col] != TILE_EMPTY) {
                    deck[count++] = board[layer][row][col];
                }
            }
        }
    }

    for (i = count - 1; i > 0; --i) {
        j = rand() % (i + 1);
        tmp = deck[i];
        deck[i] = deck[j];
        deck[j] = tmp;
    }

    i = 0;
    for (layer = 0; layer < BOARD_LAYERS; ++layer) {
        for (row = 0; row < BOARD_ROWS; ++row) {
            for (col = 0; col < BOARD_COLS; ++col) {
                if (board[layer][row][col] != TILE_EMPTY) {
                    board[layer][row][col] = deck[i++];
                }
            }
        }
    }

    sel_col = -1;
    valid_moves = count_valid_moves();
    state = valid_moves > 0 ? STATE_DEFAULT : STATE_STUCK;

    redraw_board();
    update_status();
}

static void
init_tiles(void)
{
    int layer, col, row, i = 0;

    memset(board, 0, sizeof(board));

    for (layer = 0; layer < BOARD_LAYERS; ++layer) {
        for (row = 0; row < BOARD_ROWS; ++row) {
            for (col = 0; col < BOARD_COLS; ++col) {
                if (board_layout[layer][row][col]) {
                    board[layer][row][col] = i / TILES_PER_TYPE + 1;
                    ++i;
                }
            }
        }
    }

    shuffle_tiles();
}

static void
select_tile(int layer, int col, int row)
{
    int prev_col, prev_row, prev_layer;

    if (state != STATE_DEFAULT) {
        return;
    }

    if (!is_tile_free(layer, col, row)) {
        return;
    }

    /* Clicked already selected tile */
    if (sel_col == col && sel_row == row && sel_layer == layer) {
        sel_col = -1;
        mark_tile_dirty(layer, col, row);
        draw_dirty_tiles();
        update_status();
        return;
    }

    /* First pick */
    if (sel_col == -1) {
        sel_col = col;
        sel_row = row;
        sel_layer = layer;
        mark_tile_dirty(layer, col, row);
        draw_dirty_tiles();
        update_status();
        return;
    }

    /* Second pick - no match */
    if (board[layer][row][col] != board[sel_layer][sel_row][sel_col]) {
        prev_col = sel_col;
        prev_row = sel_row;
        prev_layer = sel_layer;
        sel_col = -1;
        mark_tile_dirty(prev_layer, prev_col, prev_row);
        draw_dirty_tiles();
        gui_status_set("No match");
        return;
    }

    /* Second pick - match */
    prev_col = sel_col;
    prev_row = sel_row;
    prev_layer = sel_layer;
    sel_col = -1;
    --remaining_pairs;

    remove_tile(layer, col, row);
    remove_tile(prev_layer, prev_col, prev_row);

    valid_moves = count_valid_moves();

    if (remaining_pairs == 0) {
        state = STATE_WON;
    } else if (valid_moves == 0) {
        state = STATE_STUCK;
    }

    update_status();
}

static void
restart_game(void)
{
    sel_col = -1;
    remaining_pairs = TILE_COUNT / 2;
    state = STATE_DEFAULT;

    init_tiles();
}

static void
on_board_pointer_up(widget_st *widget _unsd, event_st event _unsd, point_st pos)
{
    int layer, col, row;
    rect_st rect;

    if (state != STATE_DEFAULT) {
        return;
    }

    rect.width = TILE_W;
    rect.height = TILE_H;

    for (layer = BOARD_LAYERS - 1; layer >= 0; --layer) {
        for (row = 0; row < BOARD_ROWS; ++row) {
            for (col = 0; col < BOARD_COLS; ++col) {
                if (board[layer][row][col] == TILE_EMPTY) {
                    continue;
                }

                rect.pos = tile_pos(layer, col, row);

                if (gui_rect_contains_point(rect, pos)) {
                    select_tile(layer, col, row);
                    return;
                }
            }
        }
    }
}

static void
on_key_down(window_st *w _unsd, event_st event)
{
    if (event.key_code == KEY_R) {
        restart_game();
        return;
    }

    if (state == STATE_WON) {
        return;
    }

    if (event.key_code == KEY_S) {
        shuffle_tiles();
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
init_window(void)
{
    window_surface.size.width = WINDOW_WIDTH;
    window_surface.size.height = WINDOW_HEIGHT;
    window_surface.pitch = WINDOW_WIDTH;
    window_surface.pixels = window_pixels;

    window.surface = &window_surface;
    window.title = "Mahjong";
    window.bg_color = COLOR_WIDGET_BG;
    window.widgets = widgets;
    window.widgets_capacity = sizeof(widgets) / sizeof(widgets[0]);
    window.on_key_down = on_key_down;
    window.on_active_change = on_active_change;

    gui_window_init_frame(&window, &title_bar, &close_button);

    board_widget.rect = gui_rect_make(BOARD_X, BOARD_Y, BOARD_W, BOARD_H);
    board_widget.on_pointer_up = on_board_pointer_up;
    gui_window_add_widget(&window, &board_widget);
}

static void
init_app(void)
{
    init_window();

    restart_game();
}

static void
show_app(void)
{
    (void)gui_wm_add_window(&window);
}

global app_st app_mahjong = {
    .icon = &glyph_mn_central_icon,
    .init = init_app,
    .show = show_app,
};
