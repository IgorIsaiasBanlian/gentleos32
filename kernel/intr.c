/*
 * Copyright (c) 2014-2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: intr.c - intr handling
 */

#include <kernel.h>

static isr_handler_fn krn_intr_handlers[64] = { NULL };

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
