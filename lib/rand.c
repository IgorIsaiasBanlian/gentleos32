/*
 * Copyright (c) 2014-2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: rand.c - Random number generator
 */

#include <kernel.h>

static uint32_t rand_seed = 1;

global void
rand_add_entropy(uint32_t seed)
{
    rand_seed ^= seed;

    if (!rand_seed) {
        ++rand_seed;
    }
}

global uint32_t
rand(void)
{
    /* See https://en.wikipedia.org/wiki/Xorshift */
    uint32_t x = rand_seed;

    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;

    rand_seed = x;

    return x;
}
