// --------------------------------------------------------------------------------------
// Copyright (c) 2014-2026 luke8086
// Distributed under the terms of GPL-2 License
// --------------------------------------------------------------------------------------
// File: kernel.h - Kernel API
// --------------------------------------------------------------------------------------

#ifndef _KERNEL_H_
#define _KERNEL_H_

#include <lib.h>

typedef struct {
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;

    uint32_t int_no;
    uint32_t error;
} __attribute__((packed)) isr_stack_st;

typedef void (*isr_handler_fn)(isr_stack_st *isr_stack);

typedef struct {
    uint32_t size;
    uint64_t addr;
    uint64_t len;
    uint32_t type;
} __attribute__((packed)) mboot_mmap_entry_st;

typedef struct {
    uint32_t flags;

    uint32_t unused_1[10];

    uint32_t mmap_length;
    mboot_mmap_entry_st *mmap_addr;

    uint32_t unused_2[3];

    const char *boot_loader_name;

    uint32_t unused_3[5];

    uint8_t *fb_addr;
    uint32_t unused_4;
    uint32_t fb_pitch;
    uint32_t fb_width;
    uint32_t fb_height;
    uint8_t fb_bpp;
} __attribute__ ((packed)) mboot_info_st;

typedef struct {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint16_t year;
} time_st;

enum {
    EVENT_UNKNOWN = 0,
    EVENT_POINTER_MOVE = 1,
    EVENT_POINTER_DOWN = 2,
    EVENT_POINTER_UP = 3,
    EVENT_POINTER_ALT = 4,
    EVENT_KEY_DOWN = 5,
    EVENT_KEY_UP = 6,
    EVENT_TIMER_TICK = 7,
};

typedef struct {
    uint8_t type;
    union {
        struct {
            uint16_t pointer_x;
            uint16_t pointer_y;
        };
        struct {
            uint8_t key_code;
            uint8_t key_mods;
        };
        struct {
            uint32_t timer_msecs;
        };
    };
} event_st;

extern void *krn_link_start;
extern void *krn_link_end;

static inline void
krn_vga_set_write_planes(uint8_t plane_mask)
{
    outw((plane_mask << 8) | 0x02, 0x3C4);
}

static inline void
krn_vga_set_bit_mask(uint8_t mask)
{
    outw((mask << 8) | 0x08, 0x3CE);
}

static inline void
krn_vga_set_write_mode(uint8_t mode)
{
    outw((mode << 8) | 0x05, 0x3CE);
}

static inline void
krn_vga_set_logic_op(uint8_t op)
{
    outw((op << 8) | 0x03, 0x3CE);
}

static inline void
krn_vga_latch_write(volatile uint8_t *addr, uint8_t val)
{
    (void)*addr;
    *addr = val;
}

#include "p_kernel.h"

#endif // _KERNEL_H_
