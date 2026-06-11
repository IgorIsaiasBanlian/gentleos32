/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: system.c - System information
 */

#include <kernel.h>

global uint32_t
krn_system_get_total_mem(void)
{
    system_info_st *si = &krn_system_info;

    if (!si->mem_fields_valid) {
        return 0;
    }

    return (si->mem_lower + si->mem_upper) << 10;
}

global uint32_t
krn_system_get_used_mem(void)
{
    return (uint32_t)&krn_link_end - (uint32_t)&krn_link_start;
}

global uint32_t
krn_system_get_avail_mem(void)
{
    system_info_st *si = &krn_system_info;
    uint32_t kernel_size = krn_system_get_used_mem();

    if (!si->mem_fields_valid) {
        return 0;
    }

    return (si->mem_upper << 10) - kernel_size;
}
