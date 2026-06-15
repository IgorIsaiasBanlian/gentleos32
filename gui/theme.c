/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: theme.c - Color themes
 */

#include <gui.h>

global gui_theme_st gui_theme;

global void
gui_theme_set(int theme)
{
    switch (theme) {
    case GUI_THEME_MONO:
        gui_theme.widget_bg = 0x0f;
        gui_theme.widget_fg = 0x00;
        gui_theme.widget_sel_bg = gui_theme.widget_fg;
        gui_theme.widget_sel_fg = gui_theme.widget_bg;
        gui_theme.title_act_bg = gui_theme.widget_fg;
        gui_theme.title_act_fg = gui_theme.widget_bg;
        gui_theme.title_act_inner_border = gui_theme.title_act_fg;
        gui_theme.title_sel_bg = gui_theme.widget_bg;
        gui_theme.title_sel_fg = gui_theme.widget_fg;
        gui_theme.alert_fg = gui_theme.widget_sel_bg;
        gui_theme.border = 0x00;
        gui_theme.desktop = 0x0f;
        gui_theme.desktop_alt = 0x00;
        gui_theme.desktop_pattern = &bitmap_pattern_2;
        gui_theme.card_front_bg = gui_theme.widget_bg;
        break;

    case GUI_THEME_NEON:
        gui_theme.widget_bg = 0x01;
        gui_theme.widget_fg = 0x0b;
        gui_theme.widget_sel_bg = 0x05;
        gui_theme.widget_sel_fg = gui_theme.widget_fg;
        gui_theme.title_act_bg = gui_theme.widget_sel_bg;
        gui_theme.title_act_fg = gui_theme.widget_fg;
        gui_theme.title_act_inner_border = gui_theme.title_act_bg;
        gui_theme.title_sel_bg = 0x00;
        gui_theme.title_sel_fg = gui_theme.title_act_fg;
        gui_theme.alert_fg = 0x0d;
        gui_theme.border = gui_theme.widget_fg;
        gui_theme.desktop = gui_theme.widget_sel_bg;
        gui_theme.desktop_alt = 0x00;
        gui_theme.desktop_pattern = &bitmap_pattern_4;
        gui_theme.card_front_bg = gui_theme.widget_bg;
        break;

    default:
        gui_theme.widget_bg = 0x07;
        gui_theme.widget_fg = 0x00;
        gui_theme.widget_sel_bg = 0x08;
        gui_theme.widget_sel_fg = gui_theme.widget_fg;
        gui_theme.title_act_bg = 0x0e;
        gui_theme.title_act_fg = gui_theme.widget_fg;
        gui_theme.title_act_inner_border = gui_theme.title_act_bg;
        gui_theme.title_sel_bg = gui_theme.widget_sel_bg;
        gui_theme.title_sel_fg = gui_theme.widget_sel_fg;
        gui_theme.alert_fg = 0x04;
        gui_theme.border = gui_theme.widget_fg;
        gui_theme.desktop = 0x03;
        gui_theme.desktop_alt = 0x01;
        gui_theme.desktop_pattern = NULL;
        gui_theme.card_front_bg = 0x0f;
        break;
    }

    gui_theme.card_back_bg_1 = gui_theme.widget_bg;
    gui_theme.card_back_bg_2 = gui_theme.widget_sel_bg;
    gui_theme.card_red_fg = gui_theme.alert_fg;
    gui_theme.card_black_fg = gui_theme.widget_fg;
    gui_theme.card_sel_bg = gui_theme.widget_sel_bg;
    gui_theme.card_sel_fg = gui_theme.widget_sel_fg;

    gui_theme.mj_face_bg = gui_theme.card_front_bg;
    gui_theme.mj_face_fg = gui_theme.card_black_fg;
    gui_theme.mj_face_sel_bg = gui_theme.card_sel_bg;
    gui_theme.mj_face_sel_fg = gui_theme.card_sel_fg;
    gui_theme.mj_edge = gui_theme.border;

    gui_theme.snake_floor = gui_theme.widget_bg;
    gui_theme.snake_wall = gui_theme.border;
    gui_theme.snake_snake = gui_theme.widget_fg;
    gui_theme.snake_fruit = gui_theme.title_act_bg;

    gui_theme.tetris_block = gui_theme.widget_fg;

    gui_theme.piano_key_black = gui_theme.widget_fg;
    gui_theme.piano_key_white = gui_theme.widget_bg;
    gui_theme.piano_key_sel = gui_theme.title_act_bg;
}
