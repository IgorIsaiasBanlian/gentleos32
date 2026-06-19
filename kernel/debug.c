/*
 * Copyright (c) 2014-2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: debug.c - Debug routines
 */

#include <kernel.h>

global void (*krn_debug_status_cb)(const char *, ...) = (void (*)(const char *, ...))NULL;

global void
krn_debug_putc(char c)
{
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
    static char buf[4096];

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
        krn_speaker_play(hz);
        sleep(msecs);
        krn_speaker_stop();
        sleep(msecs);
    }
}
