/*
 * Copyright (c) 2025-2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: grid.c - Grid utilities
 */

#include <gui.h>

global rect_st
gui_grid_rect(grid_st *grid)
{
    return (rect_st) {
        .x = grid->x,
        .y = grid->y,
        .width = GRID_WIDTH_SPACED(grid->cell_width, grid->cols, grid->border),
        .height = GRID_HEIGHT_SPACED(grid->cell_height, grid->rows, grid->border),
    };
}

global rect_st
gui_grid_cell_rect(grid_st *grid, int col, int row)
{
    return (rect_st) {
        .x = grid->x + grid->border + col * grid->cell_width + col,
        .y = grid->y + grid->border + row * grid->cell_height + row,
        .width = grid->cell_width,
        .height = grid->cell_height,
    };
}

global grid_pos_st
gui_grid_cell_at(grid_st *grid, point_st pos)
{
    grid_pos_st ret;
    int x = pos.x - grid->x - grid->border;
    int y = pos.y - grid->y - grid->border;
    int step_x = grid->cell_width + 1;
    int step_y = grid->cell_height + 1;

    ret.col = x / step_x;
    ret.col = MAX(0, MIN(grid->cols - 1, ret.col));

    ret.row = y / step_y;
    ret.row = MAX(0, MIN(grid->rows - 1, ret.row));

    return ret;
}

global void
gui_grid_fill(grid_st *grid, window_st *window, uint8_t color)
{
    rect_st grid_rect = gui_grid_rect(grid);
    gui_surface_draw_rect(window->surface, grid_rect, color);
    gui_wm_render_window_region(window, grid_rect);
}
