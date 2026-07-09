/*
 * Copyright (c) 2025-2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: snake.c - Snake game
 */

#include <gui.h>

enum {
    CELL_TYPE_FLOOR = 0,
    CELL_TYPE_WALL = 1,
    CELL_TYPE_SNAKE = 2,
    CELL_TYPE_FRUIT = 3,
    CELL_TYPE_COUNT = 4,

    GRID_CELL_WIDTH = 12,
    GRID_CELL_HEIGHT = 12,
    GRID_ROWS = 14,
    GRID_COLS = 24,
    GRID_COUNT = GRID_ROWS * GRID_COLS,
    GRID_WIDTH = GRID_WIDTH_SPACED(GRID_CELL_WIDTH, GRID_COLS),
    GRID_HEIGHT = GRID_HEIGHT_SPACED(GRID_CELL_HEIGHT, GRID_ROWS),
    GRID_X = 1,
    GRID_Y = TITLE_BAR_HEIGHT,

    WINDOW_WIDTH = GRID_X + GRID_WIDTH + 1,
    WINDOW_HEIGHT = GRID_Y + GRID_HEIGHT + 1,

    MOVE_TICKS = TICK_FREQUENCY * 12 / 100, /* 0.12s */
};

typedef struct {
    int x, y;
} coords_st;

typedef enum {
    DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT
} dir_et;

typedef struct {
    uint8_t window_pixels[WINDOW_WIDTH * WINDOW_HEIGHT];
    surface_st window_surface;
    window_st window;

    widget_st title_bar;
    widget_st close_button;
    widget_st *widgets[2];

    grid_st grid;

    uint8_t cells[GRID_COLS][GRID_ROWS];
    uint8_t cell_colors[CELL_TYPE_COUNT];

    struct {
        coords_st coords[GRID_COLS * GRID_ROWS];
        coords_st *head;
        coords_st *tail;
        int grow;
    } body;

    dir_et prev_dir, next_dir;

    int score;
    int best_score;
    int game_paused;
} app_state_st;

static app_state_st *app_state = NULL;

static void
update_status(void)
{
    app_state_st *a = app_state;

    const char *msg = a->game_paused ? "  \xb3  Press 'p' to resume" : "";

    gui_status_set("Score: %d  Best: %d%s", a->score, a->best_score, msg);
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
set_region(int x, int y, int w, int h, uint8_t val)
{
    app_state_st *a = app_state;

    for (int j = 0; j < h; ++j) {
        for (int i = 0; i < w; ++i) {
            a->cells[x + i][y + j] = val;
        }
    }
}

static void
clear_board(void)
{
    set_region(0, 0, GRID_COLS, GRID_ROWS, CELL_TYPE_FLOOR);
}

static void
draw_cell(int x, int y)
{
    app_state_st *a = app_state;

    uint8_t val = a->cells[x][y];
    rect_st r = gui_grid_cell_rect(&a->grid, x, y);
    gui_surface_draw_rect(a->window.surface, r, a->cell_colors[val]);
    gui_wm_render_window_region(&a->window, r);
}

static void
draw_board(void)
{
    for (int j = 0; j < GRID_ROWS; ++j) {
        for (int i = 0; i < GRID_COLS; ++i) {
            draw_cell(i, j);
        }
    }
}

static void
add_fruit(void) {
    app_state_st *a = app_state;
    coords_st c;

    do {
        c.x = rand() % GRID_COLS;
        c.y = rand() % GRID_ROWS;
    } while (a->cells[c.x][c.y] != CELL_TYPE_FLOOR);

    a->cells[c.x][c.y] = CELL_TYPE_FRUIT;
    draw_cell(c.x, c.y);
}

static coords_st
move_head(coords_st head)
{
    app_state_st *a = app_state;

    switch (a->next_dir) {
    case DIR_UP:    head.y--; break;
    case DIR_DOWN:  head.y++; break;
    case DIR_LEFT:  head.x--; break;
    case DIR_RIGHT: head.x++; break;
    }

    return head;
}

static void
move_snake(coords_st next_head)
{
    app_state_st *a = app_state;

    if (a->body.grow) {
        ++a->body.tail;
        --a->body.grow;
        update_status();
    } else {
        a->cells[a->body.tail->x][a->body.tail->y] = CELL_TYPE_FLOOR;
        draw_cell(a->body.tail->x, a->body.tail->y);
    }

    for (coords_st *c = a->body.tail; c != a->body.head; --c) {
        *c = *(c - 1);
    }

    *(a->body.head) = next_head;

    a->cells[next_head.x][next_head.y] = CELL_TYPE_SNAKE;
    draw_cell(next_head.x, next_head.y);
}

static void
restart_game(void)
{
    app_state_st *a = app_state;

    if (a->score > a->best_score) {
        a->best_score = a->score;
    }

    a->score = 0;

    a->body.coords[0].x = GRID_COLS / 2;
    a->body.coords[0].y = GRID_ROWS / 2;
    a->body.head = a->body.tail = a->body.coords;
    a->body.grow = 7;

    a->prev_dir = DIR_RIGHT;
    a->next_dir = DIR_RIGHT;

    clear_board();
    add_fruit();
    draw_board();
    update_status();
    resume_game();
}

static void
draw_window(window_st *window)
{
    gui_window_draw(window, COLOR_WIDGET_BG);
    draw_board();
}

static void
on_tick(window_st *window)
{
    app_state_st *a = app_state;
    static unsigned count = 0;

    if (!window->visible || a->game_paused) {
        return;
    }

    ++count;

    if (count < MOVE_TICKS) {
        return;
    }

    count = 0;

    coords_st next_head = move_head(*a->body.head);

    if (next_head.x < 0 || next_head.x >= GRID_COLS ||
        next_head.y < 0 || next_head.y >= GRID_ROWS) {
        restart_game();
        return;
    }

    uint8_t next_block = a->cells[next_head.x][next_head.y];

    if (next_block != CELL_TYPE_FRUIT && next_block != CELL_TYPE_FLOOR) {
        restart_game();
        return;
    }

    if (next_block == CELL_TYPE_FRUIT) {
        a->body.grow += 2;
        a->score += 5;
        update_status();
    }

    move_snake(next_head);

    if (next_block == CELL_TYPE_FRUIT) {
        add_fruit();
    }

    a->prev_dir = a->next_dir;
}

static void
on_keyboard(window_st *window _unsd, event_st event)
{
    app_state_st *a = app_state;

    if (event.key_code == KEY_P) {
        if (a->game_paused) {
            resume_game();
        } else {
            pause_game();
        }
        return;
    }

    int key = event.key_code;

    if (key == KEY_UP && a->prev_dir != DIR_DOWN) a->next_dir = DIR_UP;
    else if (key == KEY_DOWN && a->prev_dir != DIR_UP) a->next_dir = DIR_DOWN;
    else if (key == KEY_LEFT && a->prev_dir != DIR_RIGHT) a->next_dir = DIR_LEFT;
    else if (key == KEY_RIGHT && a->prev_dir != DIR_LEFT) a->next_dir = DIR_RIGHT;
}

static void
on_active_change(window_st *window)
{
    if (window->active) {
        update_status();
    } else {
        pause_game();
    }
}

static void
init_colors(void)
{
    app_state_st *a = app_state;

    a->cell_colors[CELL_TYPE_FLOOR] = COLOR_SNAKE_FLOOR;
    a->cell_colors[CELL_TYPE_WALL] = COLOR_SNAKE_WALL;
    a->cell_colors[CELL_TYPE_SNAKE] = COLOR_SNAKE_SNAKE;
    a->cell_colors[CELL_TYPE_FRUIT] = COLOR_SNAKE_FRUIT;
}

static void
close_window(window_st *window _unsd)
{
    gui_wm_remove_window(window);
    app_snake.main_window = NULL;

    krn_heap_free(app_state);
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
    a->window.title = "Snake";
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
    a->grid.x = GRID_X;
    a->grid.y = GRID_Y;
}

static int
init_app(void)
{
    ASSERT(!app_state);

    app_state = krn_heap_alloc(sizeof(app_state_st), "Snake app", 0);

    if (!app_state) {
        return E_NOT_ENOUGH_MEMORY;
    }

    init_colors();
    init_window();
    init_grid();

    restart_game();

    app_snake.main_window = &app_state->window;

    return E_OK;
}

global app_st app_snake = {
    .icon = &icon_snake,
    .init = init_app,
};
