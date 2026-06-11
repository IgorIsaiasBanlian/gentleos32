/*
 * Copyright (c) 2014-2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: ps2.c - Driver for the PS/2 (i8042) controller
 */

#include <kernel.h>

enum {
    PS2_PORT_DATA   = 0x60,
    PS2_PORT_STATUS = 0x64,
    PS2_PORT_CMD    = 0x64,

    PS2_CMD_READ_CONFIG         = 0x20,
    PS2_CMD_WRITE_CONFIG        = 0x60,
    PS2_CMD_ENABLE_MOUSE        = 0xA8,
    PS2_CMD_DISABLE_KBD         = 0xAD,
    PS2_CMD_ENABLE_KBD          = 0xAE,
    PS2_CMD_SEND_MOUSE          = 0xD4,
    PS2_CMD_RESET               = 0xFF,
    PS2_CMD_SET_DEFAULT         = 0xF6,
    PS2_CMD_ENABLE_REPORTING    = 0xF4,

    PS2_CFG_ENABLE_KBD_IRQ      = (1 << 0),
    PS2_CFG_ENABLE_MOUSE_IRQ    = (1 << 1),
    PS2_CFG_DISABLE_KBD         = (1 << 4),
    PS2_CFG_DISABLE_MOUSE       = (1 << 5),
    PS2_CFG_TRANSLATION         = (1 << 6),
};

static void
krn_ps2_outb(uint8_t val, uint16_t port)
{
    /* Status bit 1 clear means input buffer is empty */
    for (volatile int i = 0; i < 1000000; ++i) {
        if ((inb(PS2_PORT_STATUS) & 2) == 0) {
            break;
        }
    }

    outb(val, port);
}

static int
krn_ps2_has_data(void)
{
    return (inb(PS2_PORT_STATUS) & 1) != 0;
}

static void
krn_ps2_wait_for_data(void)
{
    for (volatile int i = 0; i < 1000000; ++i) {
        if (krn_ps2_has_data()) {
            break;
        }
    }
}

global uint8_t
krn_ps2_read_data(int wait)
{
    if (wait) {
        krn_ps2_wait_for_data();
    }

    return inb(PS2_PORT_DATA);
}

static void
krn_ps2_skip_data(size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        (void)krn_ps2_read_data(1);
    }
}

static void
krn_ps2_flush_data(void)
{
    while (krn_ps2_has_data()) {
        (void)krn_ps2_read_data(0);
    }
}

static uint8_t
krn_ps2_read_config(void)
{
    krn_ps2_flush_data();
    krn_ps2_outb(PS2_CMD_READ_CONFIG, PS2_PORT_CMD);
    return krn_ps2_read_data(1);
}

static void
krn_ps2_write_config(uint8_t cfg)
{
    krn_ps2_outb(PS2_CMD_WRITE_CONFIG, PS2_PORT_CMD);
    krn_ps2_outb(cfg, PS2_PORT_DATA);
}

global void
krn_ps2_reboot(void)
{
    outb(0xFE, PS2_PORT_CMD);
}

static void
krn_ps2_send_mouse(uint8_t cmd)
{
    krn_ps2_outb(PS2_CMD_SEND_MOUSE, PS2_PORT_CMD);
    krn_ps2_outb(cmd, PS2_PORT_DATA);
}

static void
krn_ps2_handle_intr(isr_stack_st *isr_stack _unsd)
{
    uint8_t data = krn_ps2_read_data(0);

    krn_mouse_handle_ps2_data(data);
}

global void
krn_ps2_init(void)
{
    uint8_t config;

    krn_debug_printf("Initializing PS2... ");

    krn_ps2_outb(PS2_CMD_DISABLE_KBD, PS2_PORT_CMD);
    krn_ps2_outb(PS2_CMD_ENABLE_MOUSE, PS2_PORT_CMD);
    krn_ps2_flush_data();

    krn_ps2_send_mouse(PS2_CMD_RESET);
    krn_ps2_skip_data(3);

    krn_ps2_send_mouse(PS2_CMD_SET_DEFAULT);
    krn_ps2_skip_data(1);

    config = krn_ps2_read_config();
    config |= PS2_CFG_ENABLE_KBD_IRQ;
    config |= PS2_CFG_ENABLE_MOUSE_IRQ;
    config |= PS2_CFG_TRANSLATION;
    config &= ~PS2_CFG_DISABLE_KBD;
    config &= ~PS2_CFG_DISABLE_MOUSE;
    krn_ps2_write_config(config);

    krn_interrupt_set_handler(0x2c, krn_ps2_handle_intr);

    krn_ps2_outb(PS2_CMD_ENABLE_KBD, PS2_PORT_CMD);
    krn_ps2_send_mouse(PS2_CMD_ENABLE_REPORTING);

    krn_debug_printf("ok\n");
}
