/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: heap.c - Heap allocator
 */

#include <kernel.h>

typedef struct heap_block {
    uint32_t magic;
    uint32_t used;
    size_t units;
    struct heap_block *next;
    char desc[16];
} heap_block_st;

enum {
    UNIT = sizeof(heap_block_st),
    HEAP_MAGIC = 0x48454150, /* "HEAP" */
};

static heap_block_st heap_base = {
    .magic = HEAP_MAGIC,
    .used = 1,
    .units = 0,
    .next = 0,
    .desc = "",
};

static size_t heap_total_bytes = 0;

static const char *
heap_format_units(size_t units)
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
heap_align_up(uintptr_t addr)
{
    ASSERT(addr <= 0xffffffe0);

    return (addr + (UNIT - 1)) & ~(uintptr_t)(UNIT - 1);
}

static size_t
heap_align_down(uintptr_t addr)
{
    return addr & ~(uintptr_t)(UNIT - 1);
}

static void
heap_validate(void)
{
    heap_block_st *b;

    ASSERT(UNIT == 32);

    for (b = heap_base.next; b; b = b->next) {
        ASSERT(b->magic == HEAP_MAGIC);
        ASSERT(!b->next || (b->next >= (b + b->units) && b->next->magic == HEAP_MAGIC));
    }
}

global void
heap_dump(void)
{
    heap_block_st *b;

    krn_debug_printf("Heap blocks:\n");

    for (b = heap_base.next; b; b = b->next) {
        krn_debug_printf("- %08x  %s  %s  %s\n",
            (uint32_t)b,
            heap_format_units(b->units),
            b->used ? "USED" : "FREE",
            b->used ? b->desc : ""
        );
    }

    heap_validate();
}

global size_t
heap_get_avail_mem(void)
{
    heap_block_st *b;
    size_t ret = 0;

    for (b = heap_base.next; b; b = b->next) {
        ret += b->used ? 0 : b->units * UNIT;
    }

    return ret;
}

global size_t
heap_get_used_mem(void)
{
    return heap_total_bytes - heap_get_avail_mem();
}

static void
heap_init_block(heap_block_st *b, int used, size_t units, heap_block_st *next)
{
    b->magic = HEAP_MAGIC;
    b->used = used;
    b->units = units;
    b->next = next;
    b->desc[0] = '\0';
}

static void
heap_insert_block(heap_block_st *newb)
{
    heap_block_st *b = &heap_base;

    while (b->next != 0 && b->next < newb) {
        b = b->next;
    }

    newb->next = b->next;
    b->next = newb;

    heap_validate();
}

static void
heap_merge_free_blocks(void)
{
    heap_block_st *b;

    for (b = heap_base.next; b; b = b->next) {
        while (b->next && !b->used && !b->next->used && b + b->units == b->next) {
            b->units += b->next->units;
            b->next = b->next->next;
        }
    }

    heap_validate();
}

global void
heap_add_region(uintptr_t start, uintptr_t end)
{
    heap_block_st *b;

    start = heap_align_up(start);
    end = heap_align_down(end);

    if (end <= start) {
        return;
    }

    b = (heap_block_st *)start;
    heap_init_block(b, 0, (end - start) / UNIT, 0);
    heap_insert_block(b);

    heap_total_bytes += end - start;
}

global void *
heap_alloc(size_t size, const char *desc, int assert)
{
    heap_block_st *b, *newb;
    size_t units;
    void *ret = 0;

    ASSERT(size > 0);

    units = (size + UNIT - 1) / UNIT + 1;

    krn_debug_printf("Alloc %s for %s... ", heap_format_units(units), desc);

    for (b = heap_base.next; b; b = b->next) {
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
            heap_init_block(newb, 0, b->units - units, b->next);
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

    heap_validate();

    ASSERT(ret || !assert);

    return ret;
}

global void
heap_free(void *ptr)
{
    heap_block_st *b;

    if (!ptr) {
        return;
    }

    b = (heap_block_st *)ptr - 1;
    ASSERT(b->magic == HEAP_MAGIC && b->used);

    krn_debug_printf("Free  %s for %s... ", heap_format_units(b->units), b->desc);

    b->used = 0;
    b->desc[0] = '\0';
    heap_merge_free_blocks();

    krn_debug_printf("ok\n");
}

