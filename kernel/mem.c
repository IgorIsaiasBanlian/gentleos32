/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: mem.c - Memory setup
 */

#include <kernel.h>

/* FIXME: Check from time to time if the ignore is still needed */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"

global int
krn_mem_check_a20(void)
{
    volatile uint8_t *lo, *hi;
    uint8_t saved_lo, saved_hi;
    int enabled;

    lo = (volatile uint8_t *)0x00000500;
    hi = (volatile uint8_t *)0x00100500;

    saved_lo = *lo;
    saved_hi = *hi;

    *lo = 0xff;
    *hi = 0x00;
    enabled = (*lo != 0x00);

    if (enabled) {
        *lo = 0x00;
        *hi = 0xff;
        enabled = (*lo != 0xff);
    }

    *lo = saved_lo;
    *hi = saved_hi;

    return enabled;
}

#pragma GCC diagnostic pop

static void
krn_mem_enable_a20(void)
{
    uint8_t val;

    krn_debug_printf("Checking A20 line... ");

    if (krn_mem_check_a20()) {
        krn_debug_printf("already enabled\n");
        return;
    }

    /* Try Fast A20 gate */
    val = inb(0x92);
    if ((val & 0x02) == 0) {
        outb((val | 0x02) & 0xfe, 0x92);
    }

    if (krn_mem_check_a20()) {
        krn_debug_printf("enabled with fast gate\n");
        return;
    }

    /* Try keyboard controller */
    krn_ps2_enable_a20();

    if (krn_mem_check_a20()) {
        krn_debug_printf("enabled with kbd ctrl\n");
        return;
    }

    krn_debug_printf("failed to enable\n");
}

global void
krn_mem_init(void)
{
    krn_mem_enable_a20();
}
