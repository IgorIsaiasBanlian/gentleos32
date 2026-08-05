/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: file.c - Operations on files
 */

#include <kernel.h>

global const char *file_type_names[FILE_TYPE_COUNT] = {
    "unknown",
    "bitmap",
};

global file_st *
file_lookup(const char *name)
{
    size_t i;
    file_st *file;
    system_info_st *si = &krn_system_info;

    for (i = 0; i < si->initrd_files_count; ++i) {
        file = &si->initrd_files[i];

        if (strncmp(name, file->name, sizeof(file->name)) == 0) {
            return file;
        }
    }

    return NULL;
}

