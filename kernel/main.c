/*
 * Copyright (c) 2014-2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: main.c - Kernel main function
 */

#include <kernel.h>
#include <gui.h>

global system_info_st krn_system_info = { 0 };

global void
krn_main(void)
{
    krn_uart_init();
    krn_debug_printf("Starting GentleOS/32\n");

    krn_mboot_init();
    krn_vga_init();
    krn_timer_init();
    krn_keyboard_init();
    krn_ps2_init();

    krn_debug_dump_kernel_location();

    rand_init();

    gui_main();

    /* NOTREACHED */
    halt();
}
