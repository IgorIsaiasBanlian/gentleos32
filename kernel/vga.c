/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: vga.c - VGA driver
 */

#include <kernel.h>

enum {
    VGA_AC_WRITE    = 0x3C0,
    VGA_AC_INDEX    = 0x3C0,
    VGA_MISC_WRITE  = 0x3C2,
    VGA_SEQ_INDEX   = 0x3C4,
    VGA_SEQ_DATA    = 0x3C5,
    VGA_GC_INDEX    = 0x3CE,
    VGA_GC_DATA     = 0x3CF,
    VGA_CRTC_INDEX  = 0x3D4,
    VGA_CRTC_DATA   = 0x3D5,
    VGA_INSTAT_READ = 0x3DA,

    VGA_NUM_SEQ_REGS  = 5,
    VGA_NUM_CRTC_REGS = 25,
    VGA_NUM_GC_REGS   = 9,
    VGA_NUM_AC_REGS   = 21,
    VGA_NUM_TOTAL_REGS = 1 + VGA_NUM_SEQ_REGS +
        VGA_NUM_CRTC_REGS + VGA_NUM_GC_REGS + VGA_NUM_AC_REGS
};

static const uint8_t krn_vga_regs_12h[VGA_NUM_TOTAL_REGS] = {
    /* MISC */
    0xE3,

    /* SEQ */
    0x03, 0x01, 0x0F, 0x00, 0x06,

    /* CRTC */
    0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0x0B, 0x3E,
    0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xEA, 0x0C, 0xDF, 0x28, 0x00, 0xE7, 0x04, 0xE3,
    0xFF,

    /* GC */
    0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x05, 0x0F,
    0xFF,

    /* AC */
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
    0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
    0x01, 0x00, 0x0F, 0x00, 0x00
};

static const uint8_t krn_vga_dac_indexes[16] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
    0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F
};

/*
 * Disclaimer: the code for setting video mode is based on:
 * - https://files.osdev.org/mirrors/geezer/osd/graphics/modes.c
 * - https://github.com/qemu/seabios/blob/master/vgasrc/stdvgamodes.c
 * At the time of writing, I didnt't really understand it in detail
 */
static void
krn_vga_set_mode(const uint8_t *regs)
{
    size_t i;

    /* Write MISC reg */
    outb(*(regs++), VGA_MISC_WRITE);

    /* Write SEQ regs */
    for(i = 0; i < VGA_NUM_SEQ_REGS; i++) {
        outb(i, VGA_SEQ_INDEX);
        outb(*(regs++), VGA_SEQ_DATA);
    }

    /* Unlock CRTC regs */
    outb(0x03, VGA_CRTC_INDEX);
    outb(inb(VGA_CRTC_DATA) | 0x80, VGA_CRTC_DATA);
    outb(0x11, VGA_CRTC_INDEX);
    outb(inb(VGA_CRTC_DATA) & ~0x80, VGA_CRTC_DATA);

    /* Write CRTC regs */
    for(i = 0; i < VGA_NUM_CRTC_REGS; ++i) {
        outb(i, VGA_CRTC_INDEX);
        outb(*(regs++), VGA_CRTC_DATA);
    }

    /* Write GC regs */
    for(i = 0; i < VGA_NUM_GC_REGS; i++) {
        outb(i, VGA_GC_INDEX);
        outb(*(regs++), VGA_GC_DATA);
    }

    /* Write AC regs */
    for(i = 0; i < VGA_NUM_AC_REGS; i++) {
        (void)inb(VGA_INSTAT_READ);
        outb(i, VGA_AC_INDEX);
        outb(*(regs++), VGA_AC_WRITE);
    }

    /* Re-enable display */
    (void)inb(VGA_INSTAT_READ);
    outb(0x20, VGA_AC_INDEX);
}

static void
krn_vga_clear_screen_12h(void)
{
    uint8_t *pixels = (uint8_t *)0xA0000;
    memset(pixels, 0x00, 640 * 480 / 8);
}

global void
krn_vga_set_color(int index, uint32_t rgb)
{
    uint8_t dac_index = index;

    if (VGA_MODE_12H) {
        dac_index = krn_vga_dac_indexes[index & 0x0F];
    }

    outb(dac_index, 0x3C8);
    outb((rgb >> 18) & 0x3F, 0x3C9);
    outb((rgb >> 10) & 0x3F, 0x3C9);
    outb((rgb >>  2) & 0x3F, 0x3C9);
}

global void
krn_vga_init(void)
{
    system_info_st *si = &krn_system_info;

    krn_debug_printf("Initializing video... ");

    if (VGA_MODE_12H) {
        krn_vga_set_mode(krn_vga_regs_12h);
        krn_vga_set_write_mode(0);
        krn_vga_set_bit_mask(0xFF);
        krn_vga_clear_screen_12h();

        si->fb_addr = (uint8_t *)0xA0000;
        si->fb_width = 640;
        si->fb_height = 480;
        si->fb_pitch = 640 / 8;
        si->fb_bpp = 4;
        si->fb_planar = 1;
        si->fb_fields_valid = 1;
    }

    krn_vga_set_color(0x01, 0x002041);
    krn_vga_set_color(0x05, 0x710071);
    krn_vga_set_color(0x09, 0x3366aa);
    krn_vga_set_color(0x0d, 0xff00ff);
    krn_vga_set_color(0x0e, 0xffcc00);

    krn_debug_printf("ok\n");
}
