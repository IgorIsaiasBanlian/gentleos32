/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: patterns.c - Background pattern app
 */

#include <gui.h>

enum {
    PADDING = 12,
    LABEL_HEIGHT = 8,
    LABEL_SPACING = 3,
    GRID_BORDER = 1,

    PATTERN_COLS = 4,
    PATTERN_ROWS = 2,
    PATTERN_CELL_WIDTH = 49,
    PATTERN_CELL_HEIGHT = 32,
    PATTERN_COUNT = (PATTERN_COLS * PATTERN_ROWS),
    PATTERN_GRID_WIDTH = GRID_WIDTH_SPACED(PATTERN_CELL_WIDTH, PATTERN_COLS, GRID_BORDER),
    PATTERN_GRID_HEIGHT = GRID_HEIGHT_SPACED(PATTERN_CELL_HEIGHT, PATTERN_ROWS, GRID_BORDER),

    COLOR_COLS = 8,
    COLOR_ROWS = 2,
    COLOR_CELL_WIDTH = 24,
    COLOR_CELL_HEIGHT = 18,
    COLOR_COUNT = (COLOR_COLS * COLOR_ROWS),
    COLOR_GRID_WIDTH = GRID_WIDTH_SPACED(COLOR_CELL_WIDTH, COLOR_COLS, GRID_BORDER),
    COLOR_GRID_HEIGHT = GRID_HEIGHT_SPACED(COLOR_CELL_HEIGHT, COLOR_ROWS, GRID_BORDER),

    THEME_COUNT = GUI_THEME_COUNT,
    THEME_COLS = 1,
    THEME_ROWS = THEME_COUNT,
    THEME_CELL_HEIGHT = 17,
    THEME_CELL_WIDTH = COLOR_GRID_WIDTH - 2 * GRID_BORDER,
    THEME_GRID_WIDTH = GRID_WIDTH_SPACED(THEME_CELL_WIDTH, THEME_COLS, GRID_BORDER),
    THEME_GRID_HEIGHT = GRID_HEIGHT_SPACED(THEME_CELL_HEIGHT, THEME_ROWS, GRID_BORDER),

    THEME_LABEL_Y = TITLE_BAR_HEIGHT + PADDING,
    THEME_GRID_X = PADDING,
    THEME_GRID_Y = THEME_LABEL_Y + LABEL_HEIGHT + LABEL_SPACING,

    PATTERN_LABEL_Y = THEME_GRID_Y + THEME_GRID_HEIGHT + PADDING,
    PATTERN_GRID_X = PADDING,
    PATTERN_GRID_Y = PATTERN_LABEL_Y + LABEL_HEIGHT + LABEL_SPACING,

    COLOR1_LABEL_Y = PATTERN_GRID_Y + PATTERN_GRID_HEIGHT + PADDING,
    COLOR1_GRID_X = PADDING,
    COLOR1_GRID_Y = COLOR1_LABEL_Y + LABEL_HEIGHT + LABEL_SPACING,

    COLOR2_LABEL_Y = COLOR1_GRID_Y + COLOR_GRID_HEIGHT + PADDING,
    COLOR2_GRID_X = PADDING,
    COLOR2_GRID_Y = COLOR2_LABEL_Y + LABEL_HEIGHT + LABEL_SPACING,

    WINDOW_WIDTH = THEME_GRID_X + THEME_GRID_WIDTH + PADDING,
    WINDOW_HEIGHT = COLOR2_GRID_Y + COLOR_GRID_HEIGHT + PADDING,

    WIDGETS_COUNT = THEME_COUNT + PATTERN_COUNT + COLOR_COUNT + COLOR_COUNT + 2,
};

typedef struct {
    uint8_t window_pixels[WINDOW_WIDTH * WINDOW_HEIGHT];
    surface_st window_surface;
    window_st window;

    widget_st title_bar;
    widget_st close_button;

    widget_st theme_buttons[THEME_COUNT];
    widget_st pattern_buttons[PATTERN_COUNT];
    widget_st color1_buttons[COLOR_COUNT];
    widget_st color2_buttons[COLOR_COUNT];

    widget_st *widgets[WIDGETS_COUNT];

    grid_st theme_grid;
    grid_st pattern_grid;
    grid_st color1_grid;
    grid_st color2_grid;

    widget_st *active_theme_button;
    widget_st *active_pattern_button;
    widget_st *active_color1_button;
    widget_st *active_color2_button;
} app_state_st;

static app_state_st *app_state = NULL;

static bitmap_st *patterns[] = {
    NULL,
    &bitmap_pattern_1,
    &bitmap_pattern_2,
    &bitmap_pattern_3,
    &bitmap_pattern_4,
    &bitmap_pattern_5,
    &bitmap_pattern_6,
    &bitmap_pattern_7,
};

static const char *theme_names[THEME_COUNT] = {
    "Default",
    "Mono",
    "Neon",
};

static void
select_active_buttons(void)
{
    app_state_st *a = app_state;

    if (gui_theme.index >= 0 && gui_theme.index < THEME_COUNT) {
        a->active_theme_button = &a->theme_buttons[gui_theme.index];
    }

    for (int i = 0; i < PATTERN_COUNT; i++) {
        if (gui_theme.desktop_pattern == patterns[i]) {
            a->active_pattern_button = &a->pattern_buttons[i];
            break;
        }
    }

    for (int i = 0; i < COLOR_COUNT; i++) {
        if (gui_theme.desktop == i) {
            a->active_color1_button = &a->color1_buttons[i];
        }

        if (gui_theme.desktop_alt == i) {
            a->active_color2_button = &a->color2_buttons[i];
        }
    }
}

static void
draw_theme_button(widget_st *widget)
{
    app_state_st *a = app_state;

    rect_st rect = widget->rect;
    surface_st *sf = widget->window->surface;
    int is_active = (widget == a->active_theme_button);

    gui_surface_draw_rect(sf, rect, COLOR_WIDGET_BG);

    if (is_active) {
        gui_surface_draw_border(sf, rect, COLOR_BORDER);
    }

    gui_surface_draw_str(sf, rect.x + 5, rect.y + 5, font_8x8, theme_names[widget->tag1],
        COLOR_WIDGET_FG, COLOR_WIDGET_BG);

    gui_wm_render_window_region(widget->window, widget->rect);
}

static void
on_theme_button_press(widget_st *widget, event_st event _unsd, point_st pos _unsd)
{
    app_state_st *a = app_state;

    if (widget->tag1 == gui_theme.index) {
        return;
    }

    a->active_theme_button = widget;

    gui_theme_set(widget->tag1);

    select_active_buttons();

    gui_wm_redraw_all();
}

static void
draw_pattern_button(widget_st *widget)
{
    app_state_st *a = app_state;

    rect_st rect = widget->rect;
    surface_st *sf = widget->window->surface;
    int is_active = (widget == a->active_pattern_button);

    int idx = widget->tag1;

    if (widget->tag1 == 0) {
        gui_surface_draw_rect(sf, rect, COLOR_WIDGET_BG);
    } else {
        gui_surface_draw_pattern_rel(sf, rect, patterns[idx], COLOR_BORDER, COLOR_WIDGET_BG);
    }

    if (is_active) {
        gui_surface_draw_border(sf, rect, COLOR_BORDER);
        rect = gui_rect_shrink(rect, 1);
    }

    gui_wm_render_window_region(widget->window, widget->rect);
}

static void
on_pattern_button_press(widget_st *widget, event_st event _unsd, point_st pos _unsd)
{
    app_state_st *a = app_state;

    widget_st *prev = a->active_pattern_button;
    a->active_pattern_button = widget;

    gui_theme.desktop_pattern = patterns[widget->tag1];

    if (prev && prev != widget) {
        gui_widget_draw(prev);
    }

    gui_widget_draw(widget);

    gui_wm_render_desktop_region(gui_wm_container, NULL);
}

static void
draw_color_button(widget_st *widget)
{
    app_state_st *a = app_state;

    rect_st rect = widget->rect;
    surface_st *sf = widget->window->surface;

    if (widget == a->active_color1_button || widget == a->active_color2_button) {
        gui_surface_draw_rect(sf, rect, COLOR_BORDER);
        rect = gui_rect_shrink(rect, 1);
    }

    gui_surface_draw_rect(sf, rect, widget->tag2);

    gui_wm_render_window_region(widget->window, widget->rect);
}

static void
on_color1_button_press(widget_st *widget, event_st event _unsd, point_st pos _unsd)
{
    app_state_st *a = app_state;

    widget_st *prev = a->active_color1_button;
    a->active_color1_button = widget;

    gui_theme.desktop = widget->tag2;

    if (prev && prev != widget) {
        gui_widget_draw(prev);
    }

    gui_widget_draw(widget);

    gui_wm_render_desktop_region(gui_wm_container, NULL);
}

static void
on_color2_button_press(widget_st *widget, event_st event _unsd, point_st pos _unsd)
{
    app_state_st *a = app_state;

    widget_st *prev = a->active_color2_button;
    a->active_color2_button = widget;

    gui_theme.desktop_alt = widget->tag2;

    if (prev && prev != widget) {
        gui_widget_draw(prev);
    }

    gui_widget_draw(widget);

    gui_wm_render_desktop_region(gui_wm_container, NULL);
}

static void
draw_window(window_st *window)
{
    app_state_st *a = app_state;

    gui_window_draw_frame(window, COLOR_WIDGET_BG);

    gui_surface_draw_str(window->surface, THEME_GRID_X, THEME_LABEL_Y, font_8x8,
        "Theme", COLOR_WIDGET_FG, COLOR_WIDGET_BG);
    gui_surface_draw_str(window->surface, PATTERN_GRID_X, PATTERN_LABEL_Y, font_8x8,
        "Desktop pattern", COLOR_WIDGET_FG, COLOR_WIDGET_BG);
    gui_surface_draw_str(window->surface, COLOR1_GRID_X, COLOR1_LABEL_Y, font_8x8,
        "Desktop color 1", COLOR_WIDGET_FG, COLOR_WIDGET_BG);
    gui_surface_draw_str(window->surface, COLOR2_GRID_X, COLOR2_LABEL_Y, font_8x8,
        "Desktop color 2", COLOR_WIDGET_FG, COLOR_WIDGET_BG);

    gui_grid_fill(&a->theme_grid, window, COLOR_BORDER);
    gui_grid_fill(&a->pattern_grid, window, COLOR_BORDER);
    gui_grid_fill(&a->color1_grid, window, COLOR_BORDER);
    gui_grid_fill(&a->color2_grid, window, COLOR_BORDER);

    gui_window_draw_widgets(window);
}

static void
close_window(window_st *window _unsd)
{
    gui_wm_remove_window(window);
    app_patterns.main_window = NULL;

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
    a->window.title = "Patterns";
    a->window.widgets = a->widgets;
    a->window.widgets_capacity = sizeof(a->widgets) / sizeof(a->widgets[0]);
    a->window.draw = draw_window;
    a->window.on_close = close_window;

    gui_window_init_frame(&a->window, &a->title_bar, &a->close_button);
}

static void
init_theme_buttons(void)
{
    app_state_st *a = app_state;

    a->theme_grid.cell_width = THEME_CELL_WIDTH;
    a->theme_grid.cell_height = THEME_CELL_HEIGHT;
    a->theme_grid.cols = THEME_COLS;
    a->theme_grid.rows = THEME_ROWS;
    a->theme_grid.border = GRID_BORDER;
    a->theme_grid.x = THEME_GRID_X;
    a->theme_grid.y = THEME_GRID_Y;

    for (int i = 0; i < THEME_COUNT; i++) {
        a->theme_buttons[i].rect = gui_grid_cell_rect(&a->theme_grid, 0, i);
        a->theme_buttons[i].tag1 = i;
        a->theme_buttons[i].window = &a->window;
        a->theme_buttons[i].draw = draw_theme_button;
        a->theme_buttons[i].on_pointer_down = on_theme_button_press;

        gui_window_add_widget(&a->window, &a->theme_buttons[i]);
    }
}

static void
init_pattern_buttons(void)
{
    app_state_st *a = app_state;

    a->pattern_grid.cell_width = PATTERN_CELL_WIDTH;
    a->pattern_grid.cell_height = PATTERN_CELL_HEIGHT;
    a->pattern_grid.cols = PATTERN_COLS;
    a->pattern_grid.rows = PATTERN_ROWS;
    a->pattern_grid.border = GRID_BORDER;
    a->pattern_grid.x = PATTERN_GRID_X;
    a->pattern_grid.y = PATTERN_GRID_Y;

    for (int i = 0; i < PATTERN_COUNT; i++) {
        int col = i % PATTERN_COLS;
        int row = i / PATTERN_COLS;

        a->pattern_buttons[i].rect = gui_grid_cell_rect(&a->pattern_grid, col, row);
        a->pattern_buttons[i].tag1 = i;
        a->pattern_buttons[i].window = &a->window;
        a->pattern_buttons[i].draw = draw_pattern_button;
        a->pattern_buttons[i].on_pointer_down = on_pattern_button_press;
        a->pattern_buttons[i].press_on_move_in = 1;

        gui_window_add_widget(&a->window, &a->pattern_buttons[i]);
    }
}

static void
init_color_buttons(grid_st *grid, widget_st *buttons, int grid_x, int grid_y,
    void (*on_press)(widget_st *, event_st, point_st))
{
    app_state_st *a = app_state;

    grid->cell_width = COLOR_CELL_WIDTH;
    grid->cell_height = COLOR_CELL_HEIGHT;
    grid->cols = COLOR_COLS;
    grid->rows = COLOR_ROWS;
    grid->border = GRID_BORDER;
    grid->x = grid_x;
    grid->y = grid_y;

    for (int i = 0; i < COLOR_COUNT; i++) {
        int col = i % COLOR_COLS;
        int row = i / COLOR_COLS;

        buttons[i].rect = gui_grid_cell_rect(grid, col, row);
        buttons[i].tag2 = i;
        buttons[i].window = &a->window;
        buttons[i].draw = draw_color_button;
        buttons[i].on_pointer_down = on_press;
        buttons[i].press_on_move_in = 1;

        gui_window_add_widget(&a->window, &buttons[i]);
    }
}

static int
init_app(void)
{
    ASSERT(!app_state);

    app_state = heap_alloc(sizeof(app_state_st), "Patterns app", 0);

    if (!app_state) {
        return E_NOT_ENOUGH_MEMORY;
    }

    init_window();
    select_active_buttons();
    init_theme_buttons();
    init_pattern_buttons();
    init_color_buttons(&app_state->color1_grid, app_state->color1_buttons,
        COLOR1_GRID_X, COLOR1_GRID_Y, on_color1_button_press);
    init_color_buttons(&app_state->color2_grid, app_state->color2_buttons,
        COLOR2_GRID_X, COLOR2_GRID_Y, on_color2_button_press);

    app_patterns.main_window = &app_state->window;

    return E_OK;
}

global app_st app_patterns = {
    .icon = &icon_patterns,
    .init = init_app,
};
