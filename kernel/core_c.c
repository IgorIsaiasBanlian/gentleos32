/*
 * Copyright (c) 2014-2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: core_c.c - Main entry point in C
 */

#include <kernel.h>

enum {
#if VGA_MODE_12H
    MBOOT_FLAGS = 0x03, /* page align & mem info */
#else
    MBOOT_FLAGS = 0x07, /* page align & mem info & video mode */
#endif
    MBOOT_MAGIC = 0x1BADB002,
    MBOOT_CKSUM = -(MBOOT_MAGIC + MBOOT_FLAGS),
};

__attribute__((section(".multiboot"))) __attribute__((aligned(4)))
global uint32_t krn_core_mboot_header[] = {
    MBOOT_MAGIC,
    MBOOT_FLAGS,
    MBOOT_CKSUM,
    0, 0, 0, 0, 0,
    0,
    VESA_WIDTH,
    VESA_HEIGHT,
    8,
};

global void
krn_core_c_main(void)
{
    krn_main();

    while (1);
}

global __attribute__((force_align_arg_pointer)) void
krn_core_c_isr_handle(isr_stack_st *isr_stack)
{
    krn_interrupt_handle(isr_stack);
}

