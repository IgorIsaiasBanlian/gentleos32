// --------------------------------------------------------------------------------------
// Copyright (c) 2014-2026 luke8086
// Distributed under the terms of GPL-2 License
// --------------------------------------------------------------------------------------
// File: mouse.c - Driver for PS/2 mouse
// --------------------------------------------------------------------------------------

#include <kernel.h>

static struct {
    int16_t x;
    int16_t y;
    uint16_t btn_left;
    uint16_t btn_right;
} mouse_state;

static void
krn_mouse_handle_packet(uint8_t a, uint8_t b, uint8_t c)
{
    // Ignore packet if overflow bits are set
    if (a & 0xc0) {
        return;
    }

    int32_t dx = (int32_t)b - ((a & 0x10) ? 256 : 0);
    int32_t dy = (int32_t)c - ((a & 0x20) ? 256 : 0);

    int32_t current_x = mouse_state.x + dx;
    int32_t current_y = mouse_state.y - dy;

    current_x = MIN(current_x, GUI_WIDTH);
    current_x = MAX(current_x, 0);

    current_y = MIN(current_y, GUI_HEIGHT);
    current_y = MAX(current_y, 0);

    uint8_t btn_left = a & 0x01 ? 1 : 0;
    uint8_t btn_right = a & 0x02 ? 1 : 0;

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
krn_mouse_handle_intr(isr_stack_st *isr_stack _unsd)
{
    static uint8_t mouse_cycle = 0;
    static uint8_t mouse_byte[3];

    uint8_t mouse_data = krn_ps2_read_data();

    // Synchronize incoming data
    if (mouse_cycle == 0 && (mouse_data & 8) == 0) {
        return;
    }

    mouse_byte[mouse_cycle++] = mouse_data;

    if (mouse_cycle == 3) {
        mouse_cycle = 0;
        krn_mouse_handle_packet(mouse_byte[0], mouse_byte[1], mouse_byte[2]);
    }
}

global void
krn_mouse_init(void)
{
    mouse_state.x = GUI_WIDTH / 2;
    mouse_state.y = GUI_HEIGHT / 2;

    krn_interrupt_set_handler(0x2c, krn_mouse_handle_intr);
}
