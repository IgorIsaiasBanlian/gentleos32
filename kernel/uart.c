/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: uart.c - Driver for UART 8250
 */

#include <kernel.h>

static inline void
krn_uart_outb(uint8_t val, uint8_t reg)
{
    outb(val, UART_BASE + reg);
}

static inline uint8_t
krn_uart_inb(uint8_t reg)
{
    return inb(UART_BASE + reg);
}

static int
krn_uart_has_data(void)
{
    return krn_uart_inb(UART_LSR) & 0x01;
}

static uint8_t
krn_uart_read_data(void)
{
    return krn_uart_inb(UART_RBR);
}

static void
krn_uart_flush_data(void)
{
    while (krn_uart_has_data()) {
        (void)krn_uart_inb(UART_RBR);
    }
}

static void
krn_uart_handle_data(uint8_t data)
{
    if (UART_MODE == UART_MODE_MOUSE) {
        krn_mouse_handle_uart_data(data);
    }
}

static uint8_t
krn_uart_can_write_data(void)
{
    return krn_uart_inb(UART_LSR) & 0x20;
}

global void
krn_uart_write_data(uint8_t data)
{
    for (volatile int i = 0; i < 1000000; ++i) {
        if (krn_uart_can_write_data()) {
            krn_uart_outb(data, UART_THR);
            break;
        }
    }
}

static void
krn_uart_handle_intr(isr_stack_st *isr_stack _unsd)
{
    uint8_t data;

    while (krn_uart_has_data()) {
        data = krn_uart_read_data();
        krn_uart_handle_data(data);
    }
}

static void
krn_uart_set_baud_rate(uint32_t rate)
{
    uint16_t div = 115200 / rate;
    uint8_t lcr = krn_uart_inb(UART_LCR);

    krn_uart_outb(lcr | 0x80, UART_LCR);
    krn_uart_outb(div & 0xFF, UART_DLL);
    krn_uart_outb((div >> 8) & 0xFF, UART_DLM);
    krn_uart_outb(lcr & 0x7F, UART_LCR);
}

global void
krn_uart_init(void)
{
    krn_uart_outb(0x00, UART_IER);
    krn_intr_set_handler(0x24, krn_uart_handle_intr);

    krn_uart_outb(0x00, UART_FCR);

    if (UART_MODE == UART_MODE_MOUSE) {
        krn_uart_set_baud_rate(1200);
        krn_uart_outb(0x02, UART_LCR); /* 7N1 */
        krn_uart_outb(0x0B, UART_MCR); /* DTR + RTS + OUT2 */
    } else if (UART_MODE == UART_MODE_DEBUG) {
        krn_uart_set_baud_rate(9600);
        krn_uart_outb(0x03, UART_LCR); /* 8N1 */
        krn_uart_outb(0x08, UART_MCR); /* OUT2 */
    }

    krn_uart_flush_data();
    krn_uart_outb(0x01, UART_IER);
}
