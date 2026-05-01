// --------------------------------------------------------------------------------------
// Copyright (c) 2014-2026 luke8086
// Distributed under the terms of GPL-2 License
// --------------------------------------------------------------------------------------
// File: lib.h - Basic standard library
// --------------------------------------------------------------------------------------

#ifndef _LIB_H_
#define _LIB_H_

#include <stdarg.h>
#include <stdint.h>

#include <config.h>

#define NULL ((void *)0)

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define TICK_FREQUENCY 100

#define _unsd __attribute__((unused))

#define RETURN_IF_ALREADY_CALLED        \
    static int _already_called = 0;     \
    if (_already_called) {              \
        return;                         \
    }                                   \
    _already_called = 1;

typedef uint32_t size_t;
typedef int32_t ssize_t;

enum {
    KEY_UP = 0x48,
    KEY_DOWN = 0x50,
    KEY_LEFT = 0x4b,
    KEY_RIGHT = 0x4d,

    KEY_ENTER = 0x1c,
    KEY_SPACE = 0x39,
    KEY_ESC = 0x01,
    KEY_TAB = 0x0f,
    KEY_BKSP = 0x0e,
    KEY_CAPS = 0x3a,

    KEY_INS = 0x52,
    KEY_HOME = 0x47,
    KEY_END = 0x4f,
    KEY_PGUP = 0x49,
    KEY_PGDN = 0x51,
    KEY_DEL = 0x53,

    KEY_LSHIFT = 0x2a,
    KEY_RSHIFT = 0x36,
    KEY_CTRL = 0x1d,
    KEY_ALT = 0x38,
    KEY_RCTRL = 0xe01d,
    KEY_RALT = 0xe038,

    KEY_F1 = 0x3b,
    KEY_F2 = 0x3c,
    KEY_F3 = 0x3d,
    KEY_F4 = 0x3e,
    KEY_F5 = 0x3f,
    KEY_F6 = 0x40,
    KEY_F7 = 0x41,
    KEY_F8 = 0x42,
    KEY_F9 = 0x43,
    KEY_F10 = 0x44,
    KEY_F11 = 0x57,
    KEY_F12 = 0x58,

    KEY_1 = 0x02,
    KEY_2 = 0x03,
    KEY_3 = 0x04,
    KEY_4 = 0x05,
    KEY_5 = 0x06,
    KEY_6 = 0x07,
    KEY_7 = 0x08,
    KEY_8 = 0x09,
    KEY_9 = 0x0a,
    KEY_0 = 0x0b,

    KEY_A = 0x1e,
    KEY_B = 0x30,
    KEY_C = 0x2e,
    KEY_D = 0x20,
    KEY_E = 0x12,
    KEY_F = 0x21,
    KEY_G = 0x22,
    KEY_H = 0x23,
    KEY_I = 0x17,
    KEY_J = 0x24,
    KEY_K = 0x25,
    KEY_L = 0x26,
    KEY_M = 0x32,
    KEY_N = 0x31,
    KEY_O = 0x18,
    KEY_P = 0x19,
    KEY_Q = 0x10,
    KEY_R = 0x13,
    KEY_S = 0x1f,
    KEY_T = 0x14,
    KEY_U = 0x16,
    KEY_V = 0x2f,
    KEY_W = 0x11,
    KEY_X = 0x2d,
    KEY_Y = 0x15,
    KEY_Z = 0x2c,

    KEY_BKTICK = 0x29,
    KEY_MINUS = 0x0c,
    KEY_EQUAL = 0x0d,
    KEY_LBRCKT = 0x1a,
    KEY_RBRCKT = 0x1b,
    KEY_BKSLASH = 0x2b,
    KEY_SEMICOL = 0x27,
    KEY_QUOTE = 0x28,
    KEY_COMMA = 0x33,
    KEY_PERIOD = 0x34,
    KEY_SLASH = 0x35,
};

enum {
    KEY_MOD_ESC     = 1 << 0,
    KEY_MOD_SHIFT   = 1 << 1,
    KEY_MOD_CTRL    = 1 << 2,
    KEY_MOD_ALT     = 1 << 3,
};

// lib/cpu.s
uint32_t cpu_get_eflags(void);
void cpu_set_eflags(uint32_t eflags);
void cpu_cli(void);
void cpu_hlt(void);
uint8_t inb(uint16_t port);
void outb(uint8_t value, uint16_t port);
void outw(uint16_t value, uint16_t port);
int cpu_has_cpuid(void);
void cpu_cpuid(uint32_t eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx);

#include "proto_lib.h"

#endif // _LIB_H_
