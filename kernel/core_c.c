/*
 * Copyright (c) 2014-2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: core_c.c - Main entry point in C
 */

#include <kernel.h>

global void
krn_core_c_main(void)
{
    krn_main();

    while (1);
}

global __attribute__((force_align_arg_pointer)) void
krn_core_c_isr_handle(isr_stack_st *isr_stack)
{
    krn_intr_handle(isr_stack);
}

