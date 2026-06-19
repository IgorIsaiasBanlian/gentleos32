/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: mboot.c - Routines for parsing multiboot info
 */

#include <kernel.h>

enum {
    MBOOT_FLAG_MEM         = 1 << 0,
    MBOOT_FLAG_CMDLINE     = 1 << 2,
    MBOOT_FLAG_BOOTLOADER  = 1 << 9,
    MBOOT_FLAG_FRAMEBUFFER = 1 << 12,
};

/* Only access early on boot, later it gets overwritten */
global mboot_info_st *krn_core_mboot_info;

global void
krn_mboot_init(void)
{
    mboot_info_st *m = krn_core_mboot_info;

    if (m->flags & MBOOT_FLAG_BOOTLOADER) {
        krn_debug_printf("Bootloader: %s\n", m->boot_loader_name);
    }

    if (m->flags & MBOOT_FLAG_CMDLINE) {
        krn_debug_printf("Cmdline: %s\n", m->cmdline);
    }

    if (m->flags & MBOOT_FLAG_MEM) {
        krn_system_info.mem_fields_valid = 1;
        krn_system_info.mem_lower = m->mem_lower;
        krn_system_info.mem_upper = m->mem_upper;

        krn_debug_printf("Mem: lower %u KB, upper %u KB\n", m->mem_lower, m->mem_upper);
    }

    if (m->flags & MBOOT_FLAG_FRAMEBUFFER) {
        krn_system_info.fb_addr = m->fb_addr;
        krn_system_info.fb_width = m->fb_width;
        krn_system_info.fb_height = m->fb_height;
        krn_system_info.fb_pitch = m->fb_pitch;
        krn_system_info.fb_bpp = m->fb_bpp;
        krn_system_info.fb_planar = m->fb_bpp != 8;
        krn_system_info.fb_fields_valid  = 1;

        krn_debug_printf(
            "Video: %08x %dx%dx%d\n",
            (uint32_t)m->fb_addr,
            m->fb_width, m->fb_height, m->fb_bpp
        );
    }
}
