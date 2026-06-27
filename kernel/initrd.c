/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: initrd.c - Support for a simple initial RAM disk
 */

#include <kernel.h>

typedef struct {
    char magic[4];
    uint32_t count;
} __attribute__((packed)) initrd_header_st;

static initrd_header_st *initrd_header = 0;
static initrd_entry_st *initrd_entries = 0;

global initrd_entry_st *
krn_initrd_lookup(const char *name)
{
    size_t i;
    initrd_entry_st *e;

    if (!initrd_header || !initrd_entries) {
        return NULL;
    }

    for (i = 0; i < initrd_header->count; ++i) {
        e = &initrd_entries[i];

        if (strncmp(name, e->name, sizeof(e->name)) == 0) {
            return e;
        }
    }

    return NULL;
}

global void
krn_initrd_init(void)
{
    system_info_st *si = &krn_system_info;
    initrd_header_st *header;
    initrd_entry_st *entries;
    size_t i;

    krn_debug_printf("Initializing initrd... ");

    if (si->initrd_start == 0 || si->initrd_size < sizeof(initrd_header_st)) {
        krn_debug_printf("not found\n");
        return;
    }

    header = (initrd_header_st *)si->initrd_start;
    entries = (initrd_entry_st *)((uint32_t)header + sizeof(initrd_header_st));

    if (strncmp(header->magic, "IRD1", 4) != 0) {
        krn_debug_printf("invalid format\n");
        return;
    }

    krn_debug_printf("found %u files\n", header->count);

    for (i = 0; i < header->count; ++i) {
        entries[i].addr = (void *)((uint32_t)entries[i].addr + (uint32_t)header);
        entries[i].name[sizeof(entries[i].name) - 1] = 0; /* Just in case */

        krn_debug_printf(" - %s: %08x (%u B)\n", entries[i].name,
            (uint32_t)entries[i].addr, entries[i].size);
    }

    initrd_header = header;
    initrd_entries = entries;
}
