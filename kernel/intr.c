/*
 * Copyright (c) 2014-2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: intr.c - Interrupt handling
 */

#include <kernel.h>

enum {
    INTR_COUNT = 64, /* 32 exceptions, 16 hw interrupts, 16 sys interrupts */
};

typedef struct {
    uint16_t offset_low;
    uint16_t code_sel;
    uint8_t zero;
    uint8_t flags;
    uint16_t offset_high;
} __attribute__((packed)) idt_gate_st;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_pointer_st;

extern idt_gate_st krn_core_idt[INTR_COUNT];
extern idt_pointer_st krn_core_idt_pointer;

extern uint32_t krn_core_isr_pointers[];
static isr_handler_fn krn_intr_handlers[INTR_COUNT] = { NULL };

global void
krn_intr_handle(isr_stack_st *isr_stack)
{
    if (krn_intr_handlers[isr_stack->int_no]) {
        krn_intr_handlers[isr_stack->int_no](isr_stack);
    }
}

global void
krn_intr_set_handler(uint8_t int_no, isr_handler_fn handler)
{
    krn_intr_handlers[int_no] = handler;
}

global void
krn_intr_init(void)
{
    size_t i;
    uint32_t isr;

    /* Initialize interrupt descriptor table */
    for (i = 0; i < INTR_COUNT; i++) {
        isr = krn_core_isr_pointers[i];

        krn_core_idt[i].offset_low = (uint16_t)(isr & 0xFFFF);
        krn_core_idt[i].code_sel = 0x08;
        krn_core_idt[i].zero = 0;
        krn_core_idt[i].flags = 0x8E;
        krn_core_idt[i].offset_high = (uint16_t)((isr >> 16) & 0xFFFF);
    }

    cpu_lidt(&krn_core_idt_pointer);
}
