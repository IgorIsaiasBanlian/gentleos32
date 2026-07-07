/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: heap.c - Heap allocator
 */

#include <kernel.h>

typedef struct krn_heap_block {
    uint32_t magic;
    uint32_t used;
    size_t units;
    struct krn_heap_block *next;
    char desc[16];
} krn_heap_block_st;

enum {
    MEM_UPPER_START = 0x100000,
    UNIT = sizeof(krn_heap_block_st),
    HEAP_MAGIC = 0x48454150, /* "HEAP" */
};

static krn_heap_block_st krn_heap_base;
static size_t krn_heap_total_bytes;

static const char *
krn_heap_format_units(size_t units)
{
    static char buf[10];
    size_t bytes = units * UNIT;
    int show_kb = (bytes >> 10) > 4;

    snprintf(buf, sizeof(buf), "%6u %s",
        show_kb ? (bytes >> 10) : bytes,
        show_kb ? "KB" : " B"
    );

    return buf;
}

static uintptr_t
krn_heap_align_up(uintptr_t addr)
{
    ASSERT(addr <= 0xffffffe0);

    return (addr + (UNIT - 1)) & ~(uintptr_t)(UNIT - 1);
}

static size_t
krn_heap_align_down(uintptr_t addr)
{
    return addr & ~(uintptr_t)(UNIT - 1);
}

static void
krn_heap_validate(void)
{
    krn_heap_block_st *b;

    for (b = krn_heap_base.next; b; b = b->next) {
        ASSERT(b->magic == HEAP_MAGIC);
        ASSERT(!b->next || (b->next >= (b + b->units) && b->next->magic == HEAP_MAGIC));
    }
}

global void
krn_heap_dump(void)
{
    krn_heap_block_st *b;

    krn_debug_printf("Heap blocks:\n");

    for (b = krn_heap_base.next; b; b = b->next) {
        krn_debug_printf("- %08x  %s  %s  %s\n",
            (uint32_t)b,
            krn_heap_format_units(b->units),
            b->used ? "USED" : "FREE",
            b->used ? b->desc : ""
        );
    }

    krn_heap_validate();
}

global size_t
krn_heap_get_avail_mem(void)
{
    krn_heap_block_st *b;
    size_t ret = 0;

    for (b = krn_heap_base.next; b; b = b->next) {
        ret += b->used ? 0 : b->units * UNIT;
    }

    return ret;
}

global size_t
krn_heap_get_used_mem(void)
{
    return krn_heap_total_bytes - krn_heap_get_avail_mem();
}

static void
krn_heap_init_block(krn_heap_block_st *b, int used, size_t units, krn_heap_block_st *next)
{
    b->magic = HEAP_MAGIC;
    b->used = used;
    b->units = units;
    b->next = next;
    b->desc[0] = '\0';
}

static void
krn_heap_insert_block(krn_heap_block_st *newb)
{
    krn_heap_block_st *b = &krn_heap_base;

    while (b->next != 0 && b->next < newb) {
        b = b->next;
    }

    newb->next = b->next;
    b->next = newb;

    krn_heap_validate();
}

static void
krn_heap_merge_free_blocks(void)
{
    krn_heap_block_st *b;

    for (b = krn_heap_base.next; b; b = b->next) {
        while (b->next && !b->used && !b->next->used && b + b->units == b->next) {
            b->units += b->next->units;
            b->next = b->next->next;
        }
    }

    krn_heap_validate();
}

static void
krn_heap_add_region(uintptr_t start, uintptr_t end)
{
    krn_heap_block_st *b;

    start = krn_heap_align_up(start);
    end = krn_heap_align_down(end);

    if (end <= start) {
        return;
    }

    b = (krn_heap_block_st *)start;
    krn_heap_init_block(b, 0, (end - start) / UNIT, 0);
    krn_heap_insert_block(b);

    krn_heap_total_bytes += end - start;
}

global void *
krn_heap_alloc(size_t size, const char *desc, int assert)
{
    krn_heap_block_st *b, *newb;
    size_t units;
    void *ret = 0;

    ASSERT(size > 0);

    units = (size + UNIT - 1) / UNIT + 1;

    krn_debug_printf("Alloc %s for %s... ", krn_heap_format_units(units), desc);

    for (b = krn_heap_base.next; b; b = b->next) {
        if (b->used) {
            continue;
        }

        if (b->units == units) {
            b->used = 1;
            ret = (void *)(b + 1);
            break;
        }

        if (b->units > units) {
            newb = b + units;
            krn_heap_init_block(newb, 0, b->units - units, b->next);
            b->next = newb;
            b->units = units;
            b->used = 1;
            ret = (void *)(b + 1);
            break;
        }
    }

    if (ret) {
        strncpy(b->desc, desc, sizeof(b->desc));
        b->desc[sizeof(b->desc) - 1] = '\0';
        memset(ret, 0, size);
    }

    krn_debug_printf("%08x\n", (uint32_t)ret);

    krn_heap_validate();

    ASSERT(ret || !assert);

    return ret;
}

global void
krn_heap_free(void *ptr)
{
    krn_heap_block_st *b;

    if (!ptr) {
        return;
    }

    b = (krn_heap_block_st *)ptr - 1;
    ASSERT(b->magic == HEAP_MAGIC && b->used);

    krn_debug_printf("Free  %s for %s... ", krn_heap_format_units(b->units), b->desc);

    b->used = 0;
    b->desc[0] = '\0';
    krn_heap_merge_free_blocks();

    krn_debug_printf("ok\n");
}

global void
krn_heap_init(void)
{
    system_info_st *si = &krn_system_info;
    uintptr_t krn_start = (uintptr_t)&krn_link_start;
    uintptr_t krn_end = (uintptr_t)&krn_link_end;
    uintptr_t initrd_end = si->initrd_start + si->initrd_size;
    uintptr_t low_start = 0x10000;
    uintptr_t low_end = MIN(si->mem_lower << 10, (uintptr_t)0xA0000);
    uintptr_t high_start = MEM_UPPER_START;
    uintptr_t high_end = MEM_UPPER_START + (si->mem_upper << 10);

    ASSERT(si->mem_fields_valid);
    ASSERT(UNIT == 32);

    krn_debug_printf("Kernel:    %08x - %08x (%u KB)\n",
        krn_start, krn_end, (krn_end - krn_start) >> 10);

    krn_debug_printf("Initrd:    %08x - %08x (%u KB)\n",
        si->initrd_start, initrd_end, si->initrd_size >> 10);

    krn_heap_init_block(&krn_heap_base, 1, 0, 0);
    krn_heap_total_bytes = 0;

    if (krn_start < MEM_UPPER_START) {
        low_start = MAX(low_start, krn_end);
        low_start = MAX(low_start, initrd_end);
    }

    high_start = MAX(high_start, krn_end);
    high_start = MAX(high_start, initrd_end);

    krn_heap_add_region(low_start, low_end);
    krn_heap_add_region(high_start, high_end);

    krn_heap_dump();
}
