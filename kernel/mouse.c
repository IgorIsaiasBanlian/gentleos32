// --------------------------------------------------------------------------------------
// Copyright (c) 2014-2026 luke8086
// Distributed under the terms of GPL-2 License
// --------------------------------------------------------------------------------------
// File: mouse.c - Driver for PS/2 and MS serial mouse
// --------------------------------------------------------------------------------------

#include <kernel.h>

static struct {
    int16_t x;
    int16_t y;
    uint16_t btn_left;
    uint16_t btn_right;
} mouse_state;

static uint16_t mouse_uart_base;

static void
krn_mouse_handle_packet(int dx, int dy, int btn_left, int btn_right)
{
    int32_t current_x = mouse_state.x + dx;
    int32_t current_y = mouse_state.y - dy;

    current_x = MIN(current_x, GUI_WIDTH);
    current_x = MAX(current_x, 0);

    current_y = MIN(current_y, GUI_HEIGHT);
    current_y = MAX(current_y, 0);

    event_st event = {
        .type = EVENT_POINTER_MOVE,
        .pointer_x = current_x,
        .pointer_y = current_y,
    };

    if (btn_left && !mouse_state.btn_left) {
        event.type = EVENT_POINTER_DOWN;
    } else if (!btn_left && mouse_state.btn_left) {
        event.type = EVENT_POINTER_UP;
    } else if (btn_right && !mouse_state.btn_right) {
        event.type = EVENT_POINTER_ALT;
    }

    if (event.type == EVENT_POINTER_MOVE &&
        event.pointer_x == mouse_state.x &&
        event.pointer_y == mouse_state.y) {

        return;
    }

    mouse_state.x = current_x;
    mouse_state.y = current_y;
    mouse_state.btn_left = btn_left;
    mouse_state.btn_right = btn_right;

    (void)krn_event_ipush(event);
}

static void
krn_mouse_handle_ms_data(uint8_t data)
{
    static uint8_t cycle = 0;
    static uint8_t p[3];

    // Synchronize incoming data
    if (data & 0x40) {
        cycle = 0;
    } else if (cycle == 0) {
        return;
    }

    p[cycle++] = data;

    if (cycle < 3) {
        return;
    }

    cycle = 0;

    int32_t dx = (int8_t)(((p[0] & 0x03) << 6) | (p[1] & 0x3F));
    int32_t dy = -(int8_t)(((p[0] & 0x0C) << 4) | (p[2] & 0x3F));
    uint8_t btn_left = p[0] & 0x20 ? 1 : 0;
    uint8_t btn_right = p[0] & 0x10 ? 1 : 0;

    krn_mouse_handle_packet(dx, dy, btn_left, btn_right);
}

static void
krn_mouse_handle_ms_intr(isr_stack_st *isr_stack _unsd)
{
    while (inb(mouse_uart_base + UART_LSR) & 0x01) {
        uint8_t data = inb(mouse_uart_base + UART_RBR);
        krn_mouse_handle_ms_data(data);
    }
}

static void
krn_mouse_handle_ps2_intr(isr_stack_st *isr_stack _unsd)
{
    static uint8_t cycle = 0;
    static uint8_t p[3];
    int dx, dy, btn_left, btn_right;

    uint8_t mouse_data = krn_ps2_read_data();

    // Synchronize incoming data
    if (cycle == 0 && (mouse_data & 8) == 0) {
        return;
    }

    p[cycle++] = mouse_data;

    if (cycle < 3) {
        return;
    }

    cycle = 0;

    // Ignore packet if overflow bits are set
    if (p[0] & 0xc0) {
        return;
    }

    dx = (int32_t)p[1] - ((p[0] & 0x10) ? 256 : 0);
    dy = (int32_t)p[2] - ((p[0] & 0x20) ? 256 : 0);
    btn_left = p[0] & 0x01 ? 1 : 0;
    btn_right = p[0] & 0x02 ? 1 : 0;

    krn_mouse_handle_packet(dx, dy, btn_left, btn_right);
}

static void
krn_mouse_ms_init(uint16_t uart_base)
{
    mouse_uart_base = uart_base;

    // Disable interrupts
    outb(0x00, mouse_uart_base + UART_IER);

    // Set baud rate divisor to 96 (1200 baud)
    outb(0x80, mouse_uart_base + UART_LCR);
    outb(96, mouse_uart_base + UART_DLL);
    outb(0, mouse_uart_base + UART_DLM);

    // Set 7 data bits, no parity, 1 stop bit, clear DLAB
    outb(0x02, mouse_uart_base + UART_LCR);

    // Disable FIFO
    outb(0x00, mouse_uart_base + UART_FCR);

    // Set DTR + RTS + OUT2 (power the mouse, route IRQs)
    outb(0x0B, mouse_uart_base + UART_MCR);

    // Drain any pending data
    while (inb(mouse_uart_base + UART_LSR) & 0x01) {
        (void)inb(mouse_uart_base + UART_RBR);
    }

    // Enable interrupts
    outb(0x01, mouse_uart_base + UART_IER);

    krn_interrupt_set_handler(0x24, krn_mouse_handle_ms_intr);
}

global void
krn_mouse_init(void)
{
    mouse_state.x = GUI_WIDTH / 2;
    mouse_state.y = GUI_HEIGHT / 2;

    krn_mouse_ms_init(UART_COM1);

    krn_interrupt_set_handler(0x2c, krn_mouse_handle_ps2_intr);
}
