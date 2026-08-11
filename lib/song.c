/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: song.c - Operations on songs
 */

#include <kernel.h>

global void
song_init_notes(file_st *song)
{
    note_st *notes = song->addr;
    size_t count = song->size / sizeof(note_st);

    if (count == 0) {
        return;
    }

    notes[count - 1].duration = 0; /* Just in case */

    while (notes->duration != 0) {
        notes->duration = (notes->duration * TICK_FREQUENCY + 999) / 1000;
        ++notes;
    }
}

global uint32_t
song_get_total_ticks(const note_st *notes)
{
    uint32_t ticks = 0;

    while (notes->duration != 0) {
        ticks += notes->duration;
        ++notes;
    }

    return ticks;
}
