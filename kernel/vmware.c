/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: vmware.c - Driver for PS/2 mouse in VMware and QEMU
 */

#include <kernel.h>

enum {
    VMWARE_PORT        = 0x5658,
    VMWARE_MAGIC       = 0x564D5868, /* "VMXh" */
    VMWARE_VERSION_ID  = 0x3442554A, /* "JUB4" */

    VMWARE_CMD_GETVERSION         = 10,
    VMWARE_CMD_ABSPOINTER_DATA    = 39,
    VMWARE_CMD_ABSPOINTER_STATUS  = 40,
    VMWARE_CMD_ABSPOINTER_COMMAND = 41,

    VMWARE_ABS_ENABLE      = 0x45414552, /* QEAE */
    VMWARE_ABS_REQUEST_ABS = 0x53424152, /* RABS */
    VMWARE_ABS_RELATIVE    = 0xF5,

    VMWARE_BTN_LEFT  = 0x20,
    VMWARE_BTN_RIGHT = 0x10,
};

static int krn_vmware_abspointer_active = 0;

static void
krn_vmware_call(vmware_regs_st *regs, uint32_t command, uint32_t param)
{
    regs->eax = VMWARE_MAGIC;
    regs->ebx = param;
    regs->ecx = command;
    regs->edx = VMWARE_PORT;

    cpu_vmware_call(regs);
}

static int
krn_vmware_detect(void)
{
    vmware_regs_st regs;

    krn_vmware_call(&regs, VMWARE_CMD_GETVERSION, (uint32_t)~VMWARE_MAGIC);

    return regs.ebx == VMWARE_MAGIC && regs.eax != 0xFFFFFFFFu;
}

static int
krn_vmware_enable_abspointer(void)
{
    vmware_regs_st regs;

    krn_vmware_call(&regs, VMWARE_CMD_ABSPOINTER_COMMAND, VMWARE_ABS_ENABLE);

    krn_vmware_call(&regs, VMWARE_CMD_ABSPOINTER_STATUS, 0);
    if ((regs.eax & 0xFFFF0000u) == 0xFFFF0000u) {
        return 0;
    }

    krn_vmware_call(&regs, VMWARE_CMD_ABSPOINTER_DATA, 1);
    if (regs.eax != VMWARE_VERSION_ID) {
        return 0;
    }

    krn_vmware_call(&regs, VMWARE_CMD_ABSPOINTER_COMMAND, VMWARE_ABS_REQUEST_ABS);

    return 1;
}

static void
krn_vmware_disable_abspointer(void)
{
    vmware_regs_st regs;

    krn_vmware_call(&regs, VMWARE_CMD_ABSPOINTER_COMMAND, VMWARE_ABS_RELATIVE);
}

global int
krn_vmware_handle_mouse_intr(void)
{
    vmware_regs_st regs;
    uint32_t count;
    system_info_st *si = &krn_system_info;

    if (!krn_vmware_abspointer_active) {
        return 0;
    }

    krn_vmware_call(&regs, VMWARE_CMD_ABSPOINTER_STATUS, 0);

    if ((regs.eax & 0xFFFF0000u) == 0xFFFF0000u) {
        krn_vmware_disable_abspointer();
        krn_vmware_abspointer_active = krn_vmware_enable_abspointer();
        return 1;
    }

    count = regs.eax & 0xFFFF;

    while (count >= 4) {
        krn_vmware_call(&regs, VMWARE_CMD_ABSPOINTER_DATA, 4);

        krn_mouse_handle_abs_packet(
            (int)(regs.ebx * (uint32_t)(si->fb_width - 1) / 0xffff),
            (int)(regs.ecx * (uint32_t)(si->fb_height - 1) / 0xffff),
            (regs.eax & VMWARE_BTN_LEFT) != 0,
            (regs.eax & VMWARE_BTN_RIGHT) != 0);

        count -= 4;
    }

    return 1;
}

global void
krn_vmware_init(void)
{
    krn_lock_t lock;

    krn_debug_printf("Initializing VMware mouse... ");

    lock = krn_lock();

    if (krn_vmware_detect()) {
        krn_vmware_abspointer_active = krn_vmware_enable_abspointer();
    }

    krn_unlock(lock);

    krn_debug_printf("ok (%s)\n", krn_vmware_abspointer_active ? "detected" : "not detected");
}
