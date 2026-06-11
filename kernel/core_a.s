;
; Copyright (c) 2014-2026 luke8086
; Distributed under the terms of GPL-2 License
;
; File: core_a.s - Main entry point, setup of the core structures
;

extern krn_core_c_main
extern krn_core_c_isr_handle
extern krn_core_mboot_info

[bits 32]
[cpu 386]

;;
;; Main entry point for the bootloader
;;

[section .text]

;
; Main entry point
;
global krn_core_entry:function
krn_core_entry:
    ; Initialize stack
    mov  esp, krn_core_stack_end
    mov  ebp, esp

    ; Save pointer to the info structure
    mov [krn_core_mboot_info], ebx

    ; Load GDT
    call krn_core_gdt_load

    ; Jump to C code (stack pointer is already aligned)
    jmp krn_core_c_main

[section .bss]

;
; Empty space for the stack
;
align 16
krn_core_stack:
  resb 0x10000
global krn_core_stack_end:data
krn_core_stack_end:


;;
;; Global Descriptor Table
;;

[section .text]

;
; Load GDT and update segment registers
;
global krn_core_gdt_load:function
krn_core_gdt_load:
    lgdt [krn_core_gdt_pointer]
    mov ax, (0x02 << 3)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x08:.flush_gdt
.flush_gdt:
    ret


[section .rodata]

;
; Contents of the GDT
;
align 16
global krn_core_gdt_descriptors:data
krn_core_gdt_descriptors:
    ; Null segment
    dw 0x00       ; segment limit[15:0]
    dw 0x00       ; base addr[15:0]
    db 0x00       ; base addr[23:16]
    db 00000000b  ; P, DPL, S, type
    db 00000000b  ; G, DB, _, AVL, segment limit[19:16]
    db 0x00       ; base addr[31:24]

    ; Code segment
    dw 0xFFFF     ; segment limit[15:0]
    dw 0x0000     ; base addr[15:0]
    db 0x00       ; base addr[23:16]
    db 10011010b  ; P, DPL, 1, 1, C, R, A
    db 11001111b  ; G, D, L, AVL, segment limit[19:16]
    db 0x00       ; base addr[31:24]

    ; Data segment
    dw 0xFFFF     ; segment limit[15:0]
    dw 0x00       ; base addr[15:0]
    db 0x00       ; base addr[23:16]
    db 10010010b  ; P, DPL, 1, 0, E, W, A
    db 11001111b  ; G, B, _, AVL, segment limit[19:16]
    db 0x00       ; base addr[31:24]

;
; Pointer to the GDT
;
global krn_core_gdt_pointer:data
krn_core_gdt_pointer:
    dw (3 * 8 - 1)      ; limit (3 descriptors * 8 bytes - 1)
    dd krn_core_gdt_descriptors  ; base (pointer to the GDT)


;;
;; Interrupt Descriptor Table
;;

; 32 exceptions, 16 hw interrupts, 16 system interrupts
%define INTR_COUNT 64

[section .bss]

;
; Contents of the IDT
;
align 16
global krn_core_idt:data
krn_core_idt:
    resq INTR_COUNT

[section .rodata]

;
; Pointer to the IDT
;
global krn_core_idt_pointer:data
krn_core_idt_pointer:
  dw (INTR_COUNT * 8 - 1)   ; limit (INTR_COUNT descriptors * 8 bytes - 1)
  dd krn_core_idt           ; base (pointer to the IDT)


;;
;; Interrupt Service Routines
;;

[section .text]

;
; Build an interrupt service routine
;
; Since ISRs don't receive the interrupt number, we define every ISR
; separately with the interrupt number hardcoded, using a macro.
;
%macro build_isr 1
krn_core_isr_%1:
    ; If interrupt doesn't contain error code, push dummy code
    %if !(%1 == 8 || (%1 >= 10 && %1 <= 14) || %1 == 17 || %1 == 30)
        push dword 0
    %endif

    ; Push interrupt number
    push dword %1

    ; Preserve general-purpose registers
    pusha

    ; Call interrupt handler in C
    push esp
    call krn_core_c_isr_handle
    add esp, 4

    ; If interrupt comes from PIC 1, send end-of-interrupt to PIC 1
    %if %1 >= 0x20 && %1 < 0x28
        mov al, 0x20
        out 0x20, al
    %endif

    ; If interrupt comes from PIC 2, send end-of-interrupt to PIC 1 and PIC 2
    %if %1 >= 0x28 && %1 < 0x30
        mov al, 0x20
        out 0x20, al
        out 0xa0, al
    %endif

    ; Restore general-purpose registers
    popa

    ; Pop interrupt number and error code and return
    add esp, 8
    iret
%endmacro

;
; Generate all ISRs
;
%assign i 0
%rep 64
    build_isr i
    %assign i i+1
%endrep


[section .rodata]

;
; Array of pointers to interrupt service routines
;
global krn_core_isr_pointers:data
krn_core_isr_pointers:

%assign i 0
%rep INTR_COUNT
    dd krn_core_isr_%[i]
%assign i i+1
%endrep
