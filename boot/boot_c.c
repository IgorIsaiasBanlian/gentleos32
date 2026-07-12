/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: boot_c.c - Combined 2-stage bootloader, C parts
 */

extern void putc(char);

void
cmain(void)
{
    putc('C');
}
