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
    PS2_CMD_READ_OUTPUT         = 0xD0,
    PS2_CMD_WRITE_OUTPUT        = 0xD1,
    PS2_CMD_SEND_MOUSE          = 0xD4,

    PS2_OUTPUT_SYSTEM_RESET     = (1 << 0),
    PS2_OUTPUT_A20              = (1 << 1),
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

global uint16_t
krn_ps2_read_data_with_timeout(size_t timeout)
{
    uint32_t start = krn_timer_get_msecs();
    uint16_t ret = 0;

    do {
        if (krn_ps2_has_data()) {
            ret = inb(PS2_PORT_DATA);
            ret = (ret << 8) | 1;
            break;
        }
    } while (krn_timer_get_msecs() - start < timeout);

    return ret;
}

global uint16_t
krn_ps2_read_data(void)
{
    uint16_t ret = 0;

    for (volatile int i = 0; i < 1000000; ++i) {
        if (krn_ps2_has_data()) {
            ret = inb(PS2_PORT_DATA);
            ret = (ret << 8) | 1;
            break;
        }
    }

    return ret;
}

static void
krn_ps2_skip_data(size_t timeout)
{
    (void)krn_ps2_read_data_with_timeout(timeout);
}

static void
krn_ps2_flush_data(void)
{
    while (krn_ps2_has_data()) {
        krn_ps2_skip_data(0);
    }
}

static uint8_t
krn_ps2_read_config(void)
{
    krn_ps2_flush_data();
    krn_ps2_outb(PS2_CMD_READ_CONFIG, PS2_PORT_CMD);
    return krn_ps2_read_data_with_timeout(100) >> 8;
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

global void
krn_ps2_enable_a20(void)
{
    uint8_t val;

    krn_ps2_outb(PS2_CMD_DISABLE_KBD, PS2_PORT_CMD);

    krn_ps2_outb(PS2_CMD_READ_OUTPUT, PS2_PORT_CMD);
    val = krn_ps2_read_data() >> 8;

    if (val & PS2_OUTPUT_SYSTEM_RESET) {
        krn_ps2_outb(PS2_CMD_WRITE_OUTPUT, PS2_PORT_CMD);
        krn_ps2_outb(val | PS2_OUTPUT_A20, PS2_PORT_DATA);
    }

    krn_ps2_outb(PS2_CMD_ENABLE_KBD, PS2_PORT_CMD);
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
    uint8_t data = krn_ps2_read_data_with_timeout(0) >> 8;

    if (krn_vmware_handle_mouse_intr()) {
        return;
    }

    krn_mouse_handle_ps2_data(data);
}

global void
krn_ps2_init(void)
{
    uint8_t config, mouse_detected;

    krn_debug_printf("Initializing PS2... ");

    krn_ps2_outb(PS2_CMD_DISABLE_KBD, PS2_PORT_CMD);
    krn_ps2_outb(PS2_CMD_ENABLE_MOUSE, PS2_PORT_CMD);
    krn_ps2_flush_data();

    krn_ps2_send_mouse(PS2_CMD_RESET);
    mouse_detected = krn_ps2_read_data_with_timeout(100) >> 8 == 0xfa; /* ACK */

    if (mouse_detected) {
        krn_ps2_skip_data(750); /* self test */
        krn_ps2_skip_data(100); /* device id */

        krn_ps2_send_mouse(PS2_CMD_SET_DEFAULT);
        krn_ps2_skip_data(100); /* ACK */
    }

    config = krn_ps2_read_config();
    config |= PS2_CFG_ENABLE_KBD_IRQ;
    config |= PS2_CFG_ENABLE_MOUSE_IRQ;
    config |= PS2_CFG_TRANSLATION;
    config &= ~PS2_CFG_DISABLE_KBD;
    config &= ~PS2_CFG_DISABLE_MOUSE;
    krn_ps2_write_config(config);

    krn_intr_set_handler(0x2c, krn_ps2_handle_intr);

    krn_ps2_outb(PS2_CMD_ENABLE_KBD, PS2_PORT_CMD);
    krn_ps2_send_mouse(PS2_CMD_ENABLE_REPORTING);

    krn_debug_printf("ok (mouse %s)\n", mouse_detected ? "detected" : "not detected");
}
