// --------------------------------------------------------------------------------------
// Copyright (c) 2014-2026 luke8086
// Distributed under the terms of GPL-2 License
// --------------------------------------------------------------------------------------
// File: ps2.c - Driver for the PS/2 (i8042) controller
// --------------------------------------------------------------------------------------

#include <kernel.h>

enum {
    PS2_PORT_DATA   = 0x60,
    PS2_PORT_STATUS = 0x64,
    PS2_PORT_CMD    = 0x64,

    PS2_CMD_READ_CONFIG         = 0x20,
    PS2_CMD_WRITE_CONFIG        = 0x60,
    PS2_CMD_ENABLE_MOUSE        = 0xA8,
    PS2_CMD_SEND_MOUSE          = 0xD4,
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

static uint8_t
krn_ps2_inb(uint16_t port)
{
    /* Status bit 0 set means output buffer has data */
    for (volatile int i = 0; i < 1000000; ++i) {
        if ((inb(PS2_PORT_STATUS) & 1) != 0) {
            break;
        }
    }

    return inb(port);
}

static uint8_t
krn_ps2_read_config(void)
{
    krn_ps2_outb(PS2_CMD_READ_CONFIG, PS2_PORT_CMD);
    return krn_ps2_inb(PS2_PORT_DATA);
}

static void
krn_ps2_write_config(uint8_t cfg)
{
    krn_ps2_outb(PS2_CMD_WRITE_CONFIG, PS2_PORT_CMD);
    krn_ps2_outb(cfg, PS2_PORT_DATA);
}

global uint8_t
krn_ps2_read_data(void)
{
    return inb(PS2_PORT_DATA);
}

global void
krn_ps2_reboot(void)
{
    outb(0xFE, PS2_PORT_CMD);
}

global void
krn_ps2_init(void)
{
    uint8_t config;

    /* Enable the second PS/2 port (mouse) */
    krn_ps2_outb(PS2_CMD_ENABLE_MOUSE, PS2_PORT_CMD);

    /* Set default configuration for the mouse */
    krn_ps2_outb(PS2_CMD_SEND_MOUSE, PS2_PORT_CMD);
    krn_ps2_outb(PS2_CMD_SET_DEFAULT, PS2_PORT_DATA);
    (void)krn_ps2_read_data();

    /* Enable mouse interrupts */
    krn_ps2_outb(PS2_CMD_SEND_MOUSE, PS2_PORT_CMD);
    krn_ps2_outb(PS2_CMD_ENABLE_REPORTING, PS2_PORT_DATA);
    (void)krn_ps2_read_data();

    /* Enable devices, interrupts, scancode translation */
    config = krn_ps2_read_config();
    config |= PS2_CFG_ENABLE_KBD_IRQ;
    config |= PS2_CFG_ENABLE_MOUSE_IRQ;
    config |= PS2_CFG_TRANSLATION;
    config &= ~PS2_CFG_DISABLE_KBD;
    config &= ~PS2_CFG_DISABLE_MOUSE;
    krn_ps2_write_config(config);
}
