/*
 * Copyright (c) 2014-2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: debug.c - Debug routines
 */

#include <kernel.h>

global void (*krn_debug_status_cb)(const char *, ...) = (void (*)(const char *, ...))NULL;

global char krn_debug_buffer[DEBUG_BUFFER_ROWS][DEBUG_BUFFER_COLS];
global uint32_t krn_debug_buffer_gen = 0;
static int krn_debug_buffer_row = 0;
static int krn_debug_buffer_col = 0;

static void
krn_debug_buffer_scroll(void)
{
    int i;

    for (i = 0; i < DEBUG_BUFFER_ROWS - 1; ++i) {
        memcpy(krn_debug_buffer[i], krn_debug_buffer[i + 1], DEBUG_BUFFER_COLS);
    }

    memset(krn_debug_buffer[DEBUG_BUFFER_ROWS - 1], ' ', DEBUG_BUFFER_COLS);
}

static void
krn_debug_buffer_newline(void)
{
    krn_debug_buffer_col = 0;
    ++krn_debug_buffer_row;

    if (krn_debug_buffer_row == DEBUG_BUFFER_ROWS) {
        krn_debug_buffer_scroll();
        krn_debug_buffer_row = DEBUG_BUFFER_ROWS - 1;
    }
}

static void
krn_debug_buffer_putc(char c)
{
    if (c == '\t') {
        c = ' ';
    }

    if (c == '\n') {
        krn_debug_buffer_newline();
        return;
    }

    if (c == '\r') {
        krn_debug_buffer_col = 0;
        return;
    }

    if (c < 0x20) {
        return;
    }

    krn_debug_buffer[krn_debug_buffer_row][krn_debug_buffer_col] = c;
    krn_debug_buffer_col++;

    if (krn_debug_buffer_col == DEBUG_BUFFER_COLS) {
        krn_debug_buffer_newline();
    }
}

global void
krn_debug_putc(char c)
{
    krn_debug_buffer_putc(c);
    ++krn_debug_buffer_gen;

    if (krn_system_info.uart_mode == UART_MODE_DEBUG) {
        krn_uart_write_data(c);
    } else {
        outb(c, 0xe9); /* QEMU debug port */
    }
}

global void
krn_debug_printf(const char *fmt, ...)
{
    int count;
    static char buf[128];

    va_list args;

    va_start(args, fmt);
    count = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    for (int i = 0; i < count; i++) {
        krn_debug_putc(buf[i]);
    }
}

global void
krn_debug_assert(int expr, const char *file, unsigned line)
{
    if (expr) {
        return;
    }

    krn_debug_printf("Fatal: Assertion failed (%s:%u)\n", file, line);

    if (krn_debug_status_cb) {
        krn_debug_status_cb("Assert failed (%s:%u)", file, line);
    }

    halt();
}

global void
krn_debug_beep(unsigned hz, unsigned msecs, unsigned count)
{
    for (unsigned i = 0; i < count; i++) {
        krn_speaker_play_freq(hz, NULL);
        sleep(msecs);
        krn_speaker_stop();
        sleep(msecs);
    }
}
