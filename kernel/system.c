// --------------------------------------------------------------------------------------
// Copyright (c) 2026 luke8086
// Distributed under the terms of GPL-2 License
// --------------------------------------------------------------------------------------
// File: system.c - System information
// --------------------------------------------------------------------------------------

#include <kernel.h>

global const char *
krn_system_get_cpu_vendor(void)
{
    static char buf[13];

    if (!cpu_has_cpuid()) {
        return "Unknown";
    }

    uint32_t ebx, ecx, edx;
    cpu_cpuid(0, &ebx, &ecx, &edx);
    *(uint32_t *)(buf + 0) = ebx;
    *(uint32_t *)(buf + 4) = edx;
    *(uint32_t *)(buf + 8) = ecx;
    buf[12] = '\0';

    return buf;
}

global uint32_t
krn_system_get_total_mem(void)
{
    mboot_info_st *m = krn_core_mboot_info;

    if (!(m->flags & 0x01)) {
        return 0;
    }

    return (m->mem_lower + m->mem_upper) << 10;
}

global uint32_t
krn_system_get_used_mem(void)
{
    return (uint32_t)&krn_link_end - (uint32_t)&krn_link_start;
}

global uint32_t
krn_system_get_avail_mem(void)
{
    mboot_info_st *m = krn_core_mboot_info;
    uint32_t kernel_size = krn_system_get_used_mem();

    if (!(m->flags & 0x01)) {
        return 0;
    }

    return (m->mem_upper << 10) - kernel_size;
}
