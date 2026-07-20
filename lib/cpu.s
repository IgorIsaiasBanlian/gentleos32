;
; Copyright (c) 2014-2026 luke8086
; Distributed under the terms of GPL-2 License
;
; File: cpu.s - CPU-related functions in assembly
;

[cpu 386]

global cpu_lidt:function
cpu_lidt:
    mov eax, [esp + 4]
    lidt [eax]
    ret

global cpu_get_eflags:function
cpu_get_eflags:
    pushfd
    pop eax
    ret

global cpu_set_eflags:function
cpu_set_eflags:
    mov eax, [esp + 4]
    push eax
    popfd
    ret

global cpu_cli:function
cpu_cli:
    cli
    ret

global cpu_sti:function
cpu_sti:
    sti
    ret

global cpu_hlt:function
cpu_hlt:
    hlt
    ret

global inb:function
inb:
    push ebp
    mov ebp, esp

    push edx
    mov dx, [ebp + 8]
    in al, dx
    pop edx

    mov esp, ebp
    pop ebp
    ret

global outb:function
outb:
    push ebp
    mov ebp, esp
    pusha

    mov dx, [ebp + 12]
    mov al, [ebp + 8]
    out dx, al

    popa
    mov esp, ebp
    pop ebp
    ret

global outw:function
outw:
    push ebp
    mov ebp, esp
    pusha

    mov dx, [ebp + 12]
    mov ax, [ebp + 8]
    out dx, ax

    popa
    mov esp, ebp
    pop ebp
    ret

global cpu_rep_movsd:function
cpu_rep_movsd:
    push esi
    push edi

    mov edi, [esp + 12]
    mov esi, [esp + 16]
    mov ecx, [esp + 20]
    cld
    rep movsd

    pop edi
    pop esi
    ret

global cpu_rep_stosd:function
cpu_rep_stosd:
    push edi

    mov edi, [esp + 8]
    mov eax, [esp + 12]
    mov ecx, [esp + 16]
    cld
    rep stosd

    pop edi
    ret

global cpu_vmware_call:function
cpu_vmware_call:
    push ebp
    mov ebp, esp
    push ebx
    push esi

    mov esi, [ebp + 8]
    mov eax, [esi]
    mov ebx, [esi + 4]
    mov ecx, [esi + 8]
    mov edx, [esi + 12]

    in eax, dx

    mov [esi], eax
    mov [esi + 4], ebx
    mov [esi + 8], ecx
    mov [esi + 12], edx

    pop esi
    pop ebx
    mov esp, ebp
    pop ebp
    ret
