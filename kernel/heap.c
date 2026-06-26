/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: heap.c - Heap allocator
 */

#include <kernel.h>

static uint32_t mem_lower_start;
static uint32_t mem_lower_ptr;
static uint32_t mem_lower_end;
static uint32_t mem_upper_start;
static uint32_t mem_upper_ptr;
static uint32_t mem_upper_end;
static int mem_use_upper;

static uint32_t
krn_heap_align_addr(uint32_t addr)
{
    return (addr + 0xf) & ~(uint32_t)(0xf);
}

global void *
krn_heap_alloc(size_t size, const char *desc, int assert)
{
    uint32_t p;
    void *ret = 0;

    krn_debug_printf("Allocating %x for %s... ", size, desc);

    if (!mem_use_upper) {
        p = krn_heap_align_addr(mem_lower_ptr);

        if (p + size <= mem_lower_end) {
            mem_lower_ptr = p + size;
            ret = (void *)p;
        }
    }

    if (!ret) {
        mem_use_upper = 1;
    }

    if (mem_use_upper) {
        p = krn_heap_align_addr(mem_upper_ptr);

        if (p + size <= mem_upper_end) {
            mem_upper_ptr = p + size;
            ret = (void *)p;
        }
    }

    if (ret) {
        memset(ret, 0, size);
    }

    krn_debug_printf("%x\n", (uint32_t)ret);

    ASSERT(ret || !assert);

    return ret;
}

global uint32_t
krn_heap_get_used_mem(void)
{
    return (mem_lower_ptr - mem_lower_start) + (mem_upper_ptr - mem_upper_start);
}

global uint32_t
krn_heap_get_avail_mem(void)
{
    return (mem_lower_end - mem_lower_ptr) + (mem_upper_end - mem_upper_ptr);
}

global void
krn_heap_init(void)
{
    system_info_st *si = &krn_system_info;
    uint32_t krn_start = (uint32_t)&krn_link_start;
    uint32_t krn_end = (uint32_t)&krn_link_end;
    uint32_t initrd_end = si->initrd_start + si->initrd_size;

    ASSERT(si->mem_fields_valid);

    mem_lower_start = 0x10000;
    mem_lower_ptr = mem_lower_start;
    mem_lower_end = MIN(si->mem_lower << 10, (uint32_t)0xA0000);
    ASSERT(mem_lower_ptr < mem_lower_end);

    mem_upper_start = krn_heap_align_addr(krn_end);
    if (initrd_end > mem_upper_start) {
        mem_upper_start = krn_heap_align_addr(initrd_end);
    }

    mem_upper_ptr = mem_upper_start;
    mem_upper_end = 0x100000 + (si->mem_upper << 10);
    ASSERT(mem_upper_ptr < mem_upper_end);

    mem_use_upper = 0;

    krn_debug_printf("Kernel:    %08x - %08x (%d KB)\n",
        krn_start, krn_end, (krn_end - krn_start) >> 10);

    krn_debug_printf("Heap low:  %08x - %08x (%d KB)\n",
        mem_lower_start, mem_lower_end, (mem_lower_end - mem_lower_start) >> 10);

    krn_debug_printf("Heap high: %08x - %08x (%d KB)\n",
        mem_upper_start, mem_upper_end, (mem_upper_end - mem_upper_start) >> 10);
}
