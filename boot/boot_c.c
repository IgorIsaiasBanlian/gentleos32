/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: boot_c.c - Combined 2-stage bootloader, C parts
 */

#include <stdint.h>

extern int has_key(void);
extern int get_key(void);
extern void print_char(char);
extern void print_str(const char *);
extern void print_ushort(uint16_t);
extern void load_kernel(void);

void
stage2_cmain(void)
{
    print_str("\r\nLoading GentleOS");
    load_kernel();
}
