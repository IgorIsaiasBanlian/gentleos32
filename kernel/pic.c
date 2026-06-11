/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: pic.c - Driver for PIC 8259
 */

#include <kernel.h>

enum {
    PIC1_CMD  = 0x20,
    PIC1_DATA = 0x21,
    PIC2_CMD  = 0xA0,
    PIC2_DATA = 0xA1,
};

global void
krn_pic_init(void)
{
    /* Send "init & require ICW4" to both PICs */
    outb(0x11, PIC1_CMD);
    outb(0x11, PIC2_CMD);

    /* Set interrupt offsets */
    outb(0x20, PIC1_DATA);
    outb(0x28, PIC2_DATA);

    /* Send "master PIC has slave on irq 2" to PIC1 */
    outb(0x04, PIC1_DATA);

    /* Send "slave PIC id is 2" to PIC2 */
    outb(0x02, PIC2_DATA);

    /* Send "8086 mode" to both PICs */
    outb(0x01, PIC1_DATA);
    outb(0x01, PIC2_DATA);

    /* Unmask all interrupts in both PICs */
    outb(0x00, PIC1_DATA);
    outb(0x00, PIC2_DATA);

    /* Enable interrupts */
    cpu_sti();
}
