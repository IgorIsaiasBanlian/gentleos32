// --------------------------------------------------------------------------------------
// Copyright (c) 2026 luke8086
// Distributed under the terms of GPL-2 License
// --------------------------------------------------------------------------------------
// File: uart.c - Driver for UART 8250
// --------------------------------------------------------------------------------------

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

static void
krn_uart_handle_intr(isr_stack_st *isr_stack _unsd)
{
    uint8_t data;

    while (krn_uart_has_data()) {
        data = krn_uart_read_data();
        krn_uart_handle_data(data);
    }
}

global void
krn_uart_init(void)
{
    // Disable interrupts and set handler
    krn_uart_outb(0x00, UART_IER);
    krn_interrupt_set_handler(0x24, krn_uart_handle_intr);

    if (UART_MODE == UART_MODE_MOUSE) {
        // Set baud rate divisor to 96 (1200 baud)
        krn_uart_outb(0x80, UART_LCR);
        krn_uart_outb(96, UART_DLL);
        krn_uart_outb(0, UART_DLM);

        // Set 7 data bits, no parity, 1 stop bit, clear DLAB
        krn_uart_outb(0x02, UART_LCR);

        // Disable FIFO
        krn_uart_outb(0x00, UART_FCR);

        // Set DTR + RTS + OUT2 (power the mouse, route IRQs)
        krn_uart_outb(0x0B, UART_MCR);
    }

    // Flush any remaining data and enable interrupts
    krn_uart_flush_data();
    krn_uart_outb(0x01, UART_IER);
}
