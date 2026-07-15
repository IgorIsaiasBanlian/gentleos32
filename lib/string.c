/*
 * Copyright (c) 2014-2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: string.c - String handling routines
 */

#include <lib.h>

global void *
memcpy(void *dest, const void *src, size_t n)
{
    uint8_t *srcb, *destb;
    uint32_t *srcd, *destd;
    size_t nd;

    srcb = (uint8_t *)src;
    destb = (uint8_t *)dest;

    for (; n > 0 && ((uintptr_t)destb % 4) != 0; --n) {
        *(destb++) = *(srcb++);
    }

    srcd = (uint32_t *)srcb;
    destd = (uint32_t *)destb;
    nd = n / sizeof(*destd);

    cpu_rep_movsd(destd, srcd, nd);
    srcd += nd;
    destd += nd;
    n -= nd * sizeof(*destd);

    srcb = (uint8_t *)srcd;
    destb = (uint8_t *)destd;

    for (; n > 0; --n) {
        *(destb++) = *(srcb++);
    }

    return dest;
}

global void *
memset(void *dest, int c, size_t n)
{
    uint8_t cb;
    uint8_t *destb;
    uint32_t cd;
    uint32_t *destd;
    size_t nd;

    destb = (uint8_t *)dest;
    cb = (unsigned char)c;

    for (; n > 0 && ((uintptr_t)destb % 4) != 0; --n) {
        *(destb++) = cb;
    }

    destd = (uint32_t *)destb;
    cd = cb | (cb << 8) | (cb << 16) | (cb << 24);
    nd = n / sizeof(*destd);

    cpu_rep_stosd(destd, cd, nd);
    destd += nd;
    n -= nd * sizeof(*destd);

    destb = (uint8_t *)destd;
    for (; n > 0; --n) {
        *(destb++) = cb;
    }

    return dest;
}

global int32_t
strcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2)) {
        ++s1;
        ++s2;
    }

    return (*s1 - *s2);
}

global int32_t
strncmp(const char *s1, const char *s2, size_t n)
{
    while (n > 0 && *s1 && (*s1 == *s2)) {
        ++s1;
        ++s2;
        --n;
    }

    return (n == 0) ? 0 : (*s1 - *s2);
}

global size_t
strlen(const char *s1)
{
    size_t ret = 0;

    while (*s1++) {
        ++ret;
    }

    return ret;
}

global char *
strncpy(char *dest, const char *src, size_t n)
{
    size_t i;

    for (i = 0; i < n && src[i] != '\0'; ++i) {
        dest[i] = src[i];
    }

    while (i < n) {
        dest[i++] = '\0';
    }

    return dest;
}
