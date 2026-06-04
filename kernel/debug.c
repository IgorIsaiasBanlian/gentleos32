// --------------------------------------------------------------------------------------
// Copyright (c) 2014-2026 luke8086
// Distributed under the terms of GPL-2 License
// --------------------------------------------------------------------------------------
// File: debug.c - Debug routines
// --------------------------------------------------------------------------------------

#include <kernel.h>

global void (*krn_debug_status_cb)(const char *, ...) = (void (*)(const char *, ...))NULL;

global void
krn_debug_putc(char c)
{
    if (UART_MODE == UART_MODE_DEBUG) {
        krn_uart_write_data(c);
    } else {
        outb(c, 0xe9); // QEMU debug port
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

global void
krn_debug_dump_multiboot_info(void)
{
    mboot_info_st *m = krn_core_mboot_info;

    if (m->flags & 0x04) {
        krn_debug_printf("Bootloader: %s\n", m->boot_loader_name);
    }

    if (m->flags & 0x800 && !VGA_MODE_12H) {
        krn_debug_printf(
            "Video: %08x %dx%dx%d\n",
            (uint32_t)m->fb_addr,
            m->fb_width, m->fb_height, m->fb_bpp
        );
    }

    if (m->flags & 0x01) {
        krn_debug_printf("Mem: lower %u KB, upper %u KB\n", m->mem_lower, m->mem_upper);
    }
}

global void
krn_debug_dump_kernel_location(void)
{
    uint32_t start = (uint32_t) &krn_link_start;
    uint32_t end = (uint32_t) &krn_link_end;
    uint32_t size = (end - start) >> 10;

    krn_debug_printf("Kernel location: %08x - %08x (%dKB)\n", start, end, size);
}
