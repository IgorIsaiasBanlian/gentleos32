/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: boot.h - Declarations shared between bootloader and kernel
 */

#ifndef _BOOT_H_
#define _BOOT_H_

#include <stdint.h>

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
    uint32_t unused_3;
    uint32_t fb_pitch;
    uint32_t fb_width;
    uint32_t fb_height;
    uint8_t fb_bpp;
} __attribute__ ((packed)) mboot_info_st;

_Static_assert(sizeof(mboot_info_st) == 0x6d, "unexpected sizeof");

typedef struct {
    uint32_t mod_start;
    uint32_t mod_end;
    uint32_t cmdline;
    uint32_t reserved;
} __attribute__((packed)) mboot_mod_st;

_Static_assert(sizeof(mboot_mod_st) == 0x10, "unexpected sizeof");

enum {
    MBOOT_FLAG_MEM         = 1 << 0,
    MBOOT_FLAG_CMDLINE     = 1 << 2,
    MBOOT_FLAG_MODS        = 1 << 3,
    MBOOT_FLAG_BOOTLOADER  = 1 << 9,
    MBOOT_FLAG_FRAMEBUFFER = 1 << 12,
};

enum {
    UART_MODE_NONE,
    UART_MODE_MOUSE,
    UART_MODE_DEBUG,
    UART_MODE_COUNT,
};

typedef struct {
    uint16_t offset;
    uint16_t segment;
} vbe_far_ptr_st;

_Static_assert(sizeof(vbe_far_ptr_st) == 0x04, "unexpected sizeof");

typedef struct {
    char signature[4];
    uint16_t version;
    vbe_far_ptr_st oem_string;
    uint16_t capabilities[2];
    vbe_far_ptr_st mode_list;
    uint16_t total_memory;
} __attribute__((packed)) vbe_ctrl_info_st;

_Static_assert(sizeof(vbe_ctrl_info_st) == 0x14, "unexpected sizeof");

typedef struct {
    uint16_t attrs;
    uint8_t win_a_attrs;
    uint8_t win_b_attrs;
    uint16_t win_granularity;
    uint16_t win_size;
    uint16_t win_a_segment;
    uint16_t win_b_segment;
    unsigned int win_func_ptr;
    uint16_t pitch;
    uint16_t width;
    uint16_t height;
    uint8_t char_width;
    uint8_t char_height;
    uint8_t planes;
    uint8_t bpp;
    uint8_t banks;
    uint8_t memory_model;
    uint8_t bank_size;
    uint8_t image_pages;
    uint8_t reserved;
    uint8_t color_fields[9];
    unsigned int fb_addr;
    uint32_t offscreen_ofs;
    uint16_t offscreen_size;
    uint16_t lin_pitch;
} __attribute__((packed)) vbe_mode_info_st;

_Static_assert(sizeof(vbe_mode_info_st) == 0x34, "unexpected sizeof");

enum {
    GUI_MIN_WIDTH = 640,
    GUI_MIN_HEIGHT = 480,
};

#endif /* _BOOT_H_ */
