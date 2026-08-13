/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: file.c - Operations on files
 */

#include <kernel.h>

extern file_st builtin_files[];
extern size_t builtin_files_count;

global const char *file_type_names[FILE_TYPE_COUNT] = {
    "unknown",
    "bitmap",
    "song",
};

global size_t
file_count(void)
{
    system_info_st *si = &krn_system_info;

    return builtin_files_count + si->initrd_files_count;
}

global file_st *
file_get(size_t index)
{
    system_info_st *si = &krn_system_info;

    if (index < builtin_files_count) {
        return &builtin_files[index];
    }

    index -= builtin_files_count;

    if (index < si->initrd_files_count) {
        return &si->initrd_files[index];
    }

    return NULL;
}

global file_st *
file_lookup(const char *name)
{
    size_t i;
    file_st *file;

    for (i = 0; i < file_count(); ++i) {
        file = file_get(i);

        if (strncmp(name, file->name, sizeof(file->name)) == 0) {
            return file;
        }
    }

    return NULL;
}

static void
file_dump(file_st *file)
{
    static char size_buf[10];
    int show_kb = (file->size >> 10) > 4;

    snprintf(size_buf, sizeof(size_buf), "%u %s",
        show_kb ? (file->size >> 10) : file->size,
        show_kb ? "KB" : "B"
    );

    krn_debug_printf(" - %s: %08x (%s, %s)\n",
        file->name,
        (uint32_t)file->addr,
        size_buf,
        file_type_names[file->type]
    );
}

static void
file_init(file_st *file)
{
    if (file->type == FILE_TYPE_SONG) {
        song_init_notes(file);
    }
}

global void
file_init_all(void)
{
    system_info_st *si = &krn_system_info;
    size_t i;

    krn_debug_printf("Built-in files:%s", builtin_files_count ? "\n" : " none\n");

    for (i = 0; i < builtin_files_count; ++i) {
        file_dump(&builtin_files[i]);
        file_init(&builtin_files[i]);
    }

    krn_debug_printf("Initrd files:%s", si->initrd_files_count ? "\n" : " none\n");

    for (i = 0; i < si->initrd_files_count; ++i) {
        file_dump(&si->initrd_files[i]);
        file_init(&si->initrd_files[i]);
    }
}
