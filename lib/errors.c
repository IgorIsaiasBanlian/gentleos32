/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: errors.c - Error messages
 */

#include <lib.h>

static const char *error_messages[E_CODES_COUNT] = {
    NULL,
    "Not enough memory",
    "Too many windows",
};

global const char *error_message_for(int code) {
    return code >= 0 && code < E_CODES_COUNT ? error_messages[code] : NULL;
}
