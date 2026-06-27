/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: mboot.c - Routines for parsing multiboot info
 */

#include <kernel.h>

typedef struct {
    uint32_t flags;

    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    const char *cmdline;

    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t unused_1[9];

    const char *boot_loader_name;

    uint32_t unused_2[5];

    uint8_t *fb_addr;
    uint32_t unused_4;
    uint32_t fb_pitch;
    uint32_t fb_width;
    uint32_t fb_height;
    uint8_t fb_bpp;
} __attribute__ ((packed)) mboot_info_st;

typedef struct {
    uint32_t mod_start;
    uint32_t mod_end;
    uint32_t cmdline;
    uint32_t reserved;
} __attribute__((packed)) mboot_mod_st;

enum {
    MBOOT_FLAG_MEM         = 1 << 0,
    MBOOT_FLAG_CMDLINE     = 1 << 2,
    MBOOT_FLAG_MODS        = 1 << 3,
    MBOOT_FLAG_BOOTLOADER  = 1 << 9,
    MBOOT_FLAG_FRAMEBUFFER = 1 << 12,
};

/* Only access early on boot, later it gets overwritten */
mboot_info_st *krn_core_mboot_info;

static int
cmdline_has_flag(const char *cmdline, const char *flag)
{
    const char *tok;
    const char *p = cmdline;
    size_t tok_len;
    size_t flag_len = strlen(flag);

    while (*p) {
        while (*p == ' ') {
            ++p;
        }

        tok = p;
        while (*p && *p != ' ') {
            ++p;
        }

        tok_len = (size_t)(p - tok);

        if (tok_len == flag_len && strncmp(tok, flag, flag_len) == 0) {
            return 1;
        }
    }

    return 0;
}

global void
krn_mboot_dump(void)
{
    mboot_info_st *m = krn_core_mboot_info;

    if (m->flags & MBOOT_FLAG_BOOTLOADER) {
        krn_debug_printf("Bootloader: %s\n", m->boot_loader_name);
    }

    if (m->flags & MBOOT_FLAG_CMDLINE) {
        krn_debug_printf("Cmdline: %s\n", m->cmdline);
    }

    if (m->flags & MBOOT_FLAG_FRAMEBUFFER) {
        krn_debug_printf("Video: %08x %dx%dx%d\n", (uint32_t)m->fb_addr,
            m->fb_width, m->fb_height, m->fb_bpp);
    }

    if (m->flags & MBOOT_FLAG_MEM) {
        krn_debug_printf("Mem low:   %u KB\n", m->mem_lower);
        krn_debug_printf("Mem high:  %u KB\n", m->mem_upper);
    }
}

global void
krn_mboot_init(void)
{
    mboot_info_st *m = krn_core_mboot_info;
    system_info_st *si = &krn_system_info;

    if (m->flags & MBOOT_FLAG_CMDLINE) {
        if (cmdline_has_flag(m->cmdline, "theme=mono")) {
            si->initial_theme = GUI_THEME_MONO;
        } else if (cmdline_has_flag(m->cmdline, "theme=neon")) {
            si->initial_theme = GUI_THEME_NEON;
        }

        if (cmdline_has_flag(m->cmdline, "uart=mouse")) {
            si->uart_mode = UART_MODE_MOUSE;
        } else if (cmdline_has_flag(m->cmdline, "uart=debug")) {
            si->uart_mode = UART_MODE_DEBUG;
        }

        krn_system_info.debug_keyboard = cmdline_has_flag(m->cmdline, "debug_keyboard");
    }

    if (m->flags & MBOOT_FLAG_MEM) {
        si->mem_fields_valid = 1;
        si->mem_lower = m->mem_lower;
        si->mem_upper = m->mem_upper;
    }

    if (m->flags & MBOOT_FLAG_FRAMEBUFFER) {
        si->fb_addr = m->fb_addr;
        si->fb_width = m->fb_width;
        si->fb_height = m->fb_height;
        si->fb_pitch = m->fb_pitch;
        si->fb_bpp = m->fb_bpp;
        si->fb_planar = m->fb_bpp != 8;
        si->fb_fields_valid  = 1;
    }

    if ((m->flags & MBOOT_FLAG_MODS) && m->mods_count >= 1) {
        mboot_mod_st *mod = (mboot_mod_st *)m->mods_addr;
        si->initrd_start = mod->mod_start;
        si->initrd_size = mod->mod_end - mod->mod_start;
    }
}
