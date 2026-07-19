/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: mem.c - Memory setup
 */

#include <kernel.h>

enum {
    MEM_UPPER_START = 0x100000,
};

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

static void
krn_mem_dump_system_regions(void)
{
    system_info_st *si = &krn_system_info;
    uintptr_t krn_start = (uintptr_t)&krn_link_start;
    uintptr_t krn_end = (uintptr_t)&krn_link_end;
    uintptr_t initrd_end = si->initrd_start + si->initrd_size;

    krn_debug_printf("System regions:\n");

    krn_debug_printf("- Kernel:  %08x - %08x (%u KB)\n",
        krn_start, krn_end, (krn_end - krn_start) >> 10);

    krn_debug_printf("- Initrd:  %08x - %08x (%u KB)\n",
        si->initrd_start, initrd_end, si->initrd_size >> 10);

}

static void
krn_mem_init_heap(void)
{
    system_info_st *si = &krn_system_info;
    uintptr_t krn_start = (uintptr_t)&krn_link_start;
    uintptr_t krn_end = (uintptr_t)&krn_link_end;
    uintptr_t initrd_end = si->initrd_start + si->initrd_size;
    uintptr_t low_start = 0x10000;
    uintptr_t low_end = MIN(si->mem_lower << 10, (uintptr_t)0xA0000);
    uintptr_t high_start = MEM_UPPER_START;
    uintptr_t high_end = MEM_UPPER_START + (si->mem_upper << 10);

    krn_debug_printf("Initializing heap... ");

    ASSERT(si->mem_fields_valid);

    if (krn_start < MEM_UPPER_START) {
        low_start = MAX(low_start, krn_end);
        low_start = MAX(low_start, initrd_end);
    }

    high_start = MAX(high_start, krn_end);
    high_start = MAX(high_start, initrd_end);

    heap_add_region(low_start, low_end);
    heap_add_region(high_start, high_end);

    krn_debug_printf("ok\n");

    heap_dump();
}

global void
krn_mem_init(void)
{
    system_info_st *si = &krn_system_info;

    /* TODO: Add support for proper detection if we don't get this info from multiboot */
    if (!si->mem_fields_valid) {
        si->mem_lower = 639;
        si->mem_upper = 4096;
        si->mem_fields_valid = 1;
    }

    krn_mem_enable_a20();
    krn_mem_dump_system_regions();
    krn_mem_init_heap();
}
