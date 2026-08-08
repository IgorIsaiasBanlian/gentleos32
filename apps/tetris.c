/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: tetris.c - Tetris game
 */

#include <gui.h>

enum {
    GRID_CELL_WIDTH = 14,
    GRID_CELL_HEIGHT = 14,
    GRID_ROWS = 20,
    GRID_COLS = 10,
    GRID_BORDER = 1,
    GRID_WIDTH = GRID_WIDTH_SPACED(GRID_CELL_WIDTH, GRID_COLS, GRID_BORDER),
    GRID_HEIGHT = GRID_HEIGHT_SPACED(GRID_CELL_HEIGHT, GRID_ROWS, GRID_BORDER),
    GRID_X = 0,
    GRID_Y = TITLE_BAR_HEIGHT - 1,

    WINDOW_WIDTH = GRID_X + GRID_WIDTH,
    WINDOW_HEIGHT = GRID_Y + GRID_HEIGHT,

    DROP_TICKS = TICK_FREQUENCY * 3 / 10, /* 0.3s */
};

typedef struct {
    uint8_t window_pixels[WINDOW_WIDTH * WINDOW_HEIGHT];
    surface_st window_surface;
    window_st window;

    widget_st title_bar;
    widget_st close_button;
    widget_st *widgets[2];

    grid_st grid;

    uint8_t board[GRID_ROWS][GRID_COLS];
    int cur_piece;
    int cur_rot;
    int cur_col;
    int cur_row;
    int game_over;
    int game_paused;
    size_t score;
    size_t best_score;
} app_state_st;

static app_state_st *app_state = NULL;

static uint16_t pieces[7][4] = {
    { 0x4444, 0x0f00, 0x4444, 0x0f00 }, /* I */
    { 0x44c0, 0x8e00, 0x6440, 0x0e20 }, /* J */
    { 0x4460, 0x0e80, 0xc440, 0x2e00 }, /* L */
    { 0x0cc0, 0x0cc0, 0x0cc0, 0x0cc0 }, /* O */
    { 0x06c0, 0x4620, 0x06c0, 0x4620 }, /* S */
    { 0x4e00, 0x4640, 0x0e40, 0x4c40 }, /* T */
    { 0x0c60, 0x2640, 0x0c60, 0x2640 }, /* Z */
};

static void
update_status(void)
{
    app_state_st *a = app_state;

    const char *paused_msg = a->game_paused ? "  \xb3  Press 'p' to resume" : "";

    if (a->game_over) {
        gui_status_set("Game Over!  Score: %u  Best: %u", a->score, a->best_score);
    } else {
        gui_status_set("Score: %u  Best: %u%s", a->score, a->best_score, paused_msg);
    }
}

static void
pause_game(void)
{
    app_state_st *a = app_state;

    a->game_paused = 1;
    update_status();
}

static void
resume_game(void)
{
    app_state_st *a = app_state;

    a->game_paused = 0;
    update_status();
}

static void
update_score(int ds)
{
    app_state_st *a = app_state;

    a->score += ds;
    update_status();
}

static void
draw_cell(int row, int col, int active)
{
    app_state_st *a = app_state;

    rect_st cell = gui_grid_cell_rect(&a->grid, col, row);
    gui_surface_draw_rect(a->window.surface, cell, active ? COLOR_TETRIS_BLOCK : COLOR_WIDGET_BG);
    gui_wm_render_window_region(&a->window, cell);
}

static int
is_piece_valid(int piece_idx, int row, int col, int rot)
{
    app_state_st *a = app_state;

    uint16_t piece = pieces[piece_idx][rot];

    for (int dy = 0; dy < 4; ++dy) {
        for (int dx = 0; dx < 4; ++dx) {
            if (!(piece & (0x8000 >> (dy * 4 + dx)))) {
                continue;
            }

            int x = col + dx;
            int y = row + dy;

            if (x < 0 || x >= GRID_COLS || y < 0 || y >= GRID_ROWS) {
                return 0;
            }

            if (a->board[y][x]) {
                return 0;
            }
        }
    }

    return 1;
}

static void
draw_current_piece(int visible)
{
    app_state_st *a = app_state;

    uint16_t piece = pieces[a->cur_piece][a->cur_rot];

    for (int dy = 0; dy < 4; ++dy) {
        for (int dx = 0; dx < 4; ++dx) {
            if (!(piece & (0x8000 >> (dy * 4 + dx)))) {
                continue;
            }

            int col = a->cur_col + dx;
            int row = a->cur_row + dy;

            if (col >= 0 && col < GRID_COLS && row >= 0 && row < GRID_ROWS) {
                draw_cell(row, col, visible);
            }
        }
    }
}

static void
lock_current_piece(void)
{
    app_state_st *a = app_state;

    uint16_t piece = pieces[a->cur_piece][a->cur_rot];

    for (int dy = 0; dy < 4; ++dy) {
        for (int dx = 0; dx < 4; ++dx) {
            if (!(piece & (0x8000 >> (dy * 4 + dx)))) {
                continue;
            }

            int col = a->cur_col + dx;
            int row = a->cur_row + dy;

            if (col >= 0 && col < GRID_COLS && row >= 0 && row < GRID_ROWS) {
                a->board[row][col] = 1;
            }
        }
    }

    update_score(5);
}

static int
move_current_piece(int dy, int dx, int dr)
{
    app_state_st *a = app_state;

    int row = a->cur_row + dy;
    int col = a->cur_col + dx;
    int rot = (a->cur_rot + dr) % 4;

    if (!is_piece_valid(a->cur_piece, row, col, rot)) {
        return 0;
    }

    draw_current_piece(0);
    a->cur_col = col;
    a->cur_row = row;
    a->cur_rot = rot;
    draw_current_piece(1);

    return 1;
}

static int
is_row_full(int row)
{
    app_state_st *a = app_state;

    for (int col = 0; col < GRID_COLS; ++col) {
        if (!a->board[row][col]) {
            return 0;
        }
    }

    return 1;
}

static void
clear_rows(void)
{
    app_state_st *a = app_state;

    for (int row = GRID_ROWS - 1; row >= 0; --row) {
        if (!is_row_full(row)) {
            continue;
        }

        for (int row_to_shift = row; row_to_shift > 0; --row_to_shift) {
            for (int col = 0; col < GRID_COLS; ++col) {
                a->board[row_to_shift][col] = a->board[row_to_shift - 1][col];
            }
        }

        for (int col = 0; col < GRID_COLS; ++col) {
            a->board[0][col] = 0;
        }

        for (int row_to_draw = 0; row_to_draw <= row; ++row_to_draw) {
            for (int col = 0; col < GRID_COLS; ++col) {
                draw_cell(row_to_draw, col, a->board[row_to_draw][col]);
            }
        }

        update_score(20);
        ++row;
    }
}

static void
spawn_piece(void)
{
    app_state_st *a = app_state;

    a->cur_piece = rand() % 7;
    a->cur_rot = 0;
    a->cur_col = GRID_COLS / 2 - 1;
    a->cur_row = 0;

    if (!is_piece_valid(a->cur_piece, a->cur_row, a->cur_col, a->cur_rot)) {
        a->game_over = 1;

        if (a->score > a->best_score) {
            a->best_score = a->score;
        }

        update_status();
        return;
    }

    draw_current_piece(1);
}

static void
restart_game(void)
{
    app_state_st *a = app_state;

    a->game_over = 0;
    a->score = 0;

    for (int row = 0; row < GRID_ROWS; ++row) {
        for (int col = 0; col < GRID_COLS; ++col) {
            a->board[row][col] = 0;
            draw_cell(row, col, 0);
        }
    }

    spawn_piece();
    resume_game();
    update_status();
}

static void
draw_window(window_st *window)
{
    app_state_st *a = app_state;

    gui_window_draw(window, COLOR_WIDGET_BG);

    for (int row = 0; row < GRID_ROWS; ++row) {
        for (int col = 0; col < GRID_COLS; ++col) {
            draw_cell(row, col, a->board[row][col]);
        }
    }

    draw_current_piece(1);
}

static void
on_tick(window_st *window)
{
    app_state_st *a = app_state;
    static unsigned count = 0;

    if (!window->visible || a->game_over || a->game_paused) {
        return;
    }

    ++count;

    if (count < DROP_TICKS) {
        return;
    }

    count = 0;

    if (!move_current_piece(1, 0, 0)) {
        lock_current_piece();
        clear_rows();
        spawn_piece();
    }
}

static void
on_keyboard(window_st *w _unsd, event_st event)
{
    app_state_st *a = app_state;

    if (a->game_over) {
        restart_game();
        return;
    }

    if (event.key_code == KEY_P) {
        if (a->game_paused) {
            resume_game();
        } else {
            pause_game();
        }
        return;
    }

    if (a->game_paused) {
        return;
    }

    if (event.key_code == KEY_LEFT) {
        move_current_piece(0, -1, 0);
    } else if (event.key_code == KEY_RIGHT) {
        move_current_piece(0, 1, 0);
    } else if (event.key_code == KEY_DOWN) {
        move_current_piece(1, 0, 0);
    } else if (event.key_code == KEY_UP) {
        move_current_piece(0, 0, 1);
    } else if (event.key_code == KEY_SPACE) {
        while (move_current_piece(1, 0, 0)) {
            /* drop */
        };
    }
}

static void
on_active_change(window_st *win)
{
    if (win->active) {
        update_status();
    } else {
        pause_game();
    }
}

static void
close_window(window_st *window _unsd)
{
    gui_wm_remove_window(window);
    app_tetris.main_window = NULL;

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
    a->window.title = "Tetris";
    a->window.widgets = a->widgets;
    a->window.widgets_capacity = sizeof(a->widgets) / sizeof(a->widgets[0]);
    a->window.draw = draw_window;
    a->window.on_key_down = on_keyboard;
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
}

static int
init_app(void)
{
    ASSERT(!app_state);

    app_state = heap_alloc(sizeof(app_state_st), "Tetris app", 0);

    if (!app_state) {
        return E_NOT_ENOUGH_MEMORY;
    }

    init_window();
    init_grid();

    restart_game();

    app_tetris.main_window = &app_state->window;

    return E_OK;
}

global app_st app_tetris = {
    .icon = &icon_tetris,
    .init = init_app,
};
