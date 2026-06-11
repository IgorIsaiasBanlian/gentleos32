/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: key.c - Helper functions for keyboard keys
 */

#include <lib.h>

enum {
    KEY_CHAR_MAP_SIZE = 90,
};

static const char key_char_map_default[KEY_CHAR_MAP_SIZE] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '-', 0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

static const char key_char_map_shifted[KEY_CHAR_MAP_SIZE] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|',
    'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '-', 0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

global char
key_char_for_code(uint8_t code, uint8_t mods)
{
    int shifted = mods & KEY_MOD_SHIFT;
    const char *map = shifted ? key_char_map_shifted : key_char_map_default;

    return code < KEY_CHAR_MAP_SIZE ? map[code] : 0;
}

global int
key_number_for_code(uint8_t code)
{
    if (code == KEY_0) {
        return 0;
    }

    if (code < KEY_1 || code > KEY_9) {
        return -1;
    }

    return code - KEY_1 + 1;
}
