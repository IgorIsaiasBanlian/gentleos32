;
; Copyright (c) 2019-2026 luke8086.
; Distributed under the terms of GPL-2 License.
;
; File: boot_a.s - Combined 2-stage bootloader, assembly parts
;

[cpu 386]
[bits 16]

STAGE2_DEST         equ 0x7e00
STAGE2_START_LBA    equ 2
STAGE2_SECTORS      equ 4

KERNEL_DEST         equ 0x10000
KERNEL_START_LBA    equ 6
%ifndef KERNEL_SECTORS
%define KERNEL_SECTORS 256
%endif

extern cmain

[section .asm]

;;
;; Stage 1
;;

;
; Entry point
;

    jmp 0x0000:stage1_main


;
; Global variables
;

boot_drive_index    db 0
boot_drive_spt      db 9
boot_drive_heads    db 2

remaining_sectors   dw STAGE2_SECTORS

current_lba         dw STAGE2_START_LBA
current_dest_seg    dw STAGE2_DEST >> 4
current_cylinder    dw 0
current_head        db 0
current_sector      db 0
current_count       db 0


;
; Stage 1 main function
;

stage1_main:
    ; Setup segments and stack
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, 0xfff0
    sti

    ; Clear direction flag just in case
    cld

    ; Initialize boot drive
    mov [boot_drive_index], dl
    o32 call dword reset_drive
    o32 call dword load_drive_geometry

    ; Load stage 2 loader
    o32 call dword safe_load_remaining_sectors
    jmp stage2_main


;
; Halt the program
;

halt:
    cli
    hlt
    jmp halt


;
; Return the smaller of AX and BX in AX
;

get_min_word:
    cmp ax, bx
    jbe .done
    mov ax, bx
.done:
    o32 ret


;
; Print a single character
;

global print_char:function
print_char:
    push ebp
    mov ebp, esp
    pushad

    mov ah, 0x0e
    mov al, [ebp + 8]
    xor bx, bx
    int 0x10

    popad
    pop ebp
    o32 ret


;
; Reset the boot drive
;

reset_drive:
    pushad

    xor ah, ah
    mov dl, [boot_drive_index]
    int 0x13

    popad
    o32 ret


;
; Load the boot drive geometry
;

load_drive_geometry:
    pushad
    push es

    ; ES:DI = 0 to work around buggy BIOSes
    xor di, di
    mov es, di

    mov ah, 0x08
    mov dl, [boot_drive_index]
    int 0x13

    ; On error keep the default geometry
    jc .done

    ; SPT = CL & 0x3f
    mov al, cl
    and al, 0x3f

    ; HEADS = DH + 1
    mov ah, dh
    inc ah

    ; Ignore invalid geometry
    test al, al
    jz .done
    test ah, ah
    jz .done

    mov [boot_drive_spt], al
    mov [boot_drive_heads], ah

.done:
    pop es
    popad
    o32 ret


;
; Update current CHS using current LBA
;

update_current_chs:
    pushad

    movzx cx, byte [boot_drive_spt]
    mov ax, [current_lba]
    xor dx, dx
    div cx                              ; AX = LBA / SPT, DX = LBA % SPT
    inc dl
    mov [current_sector], dl            ; sector = LBA % SPT + 1

    movzx cx, byte [boot_drive_heads]
    xor dx, dx
    div cx                              ; AX = AX / heads, DX = AX % heads
    mov [current_cylinder], ax          ; cylinder = (LBA / SPT) / heads
    mov [current_head], dl              ; head = (LBA / SPT) % heads

    popad
    o32 ret


;
; Load sectors from the boot drive using current count, CHS and dest
;

load_sectors:
    push ebx
    push es

    ; CL = ((cylinder >> 2) & 0xc0) | (sector & 0x3f)
    mov dx, [current_cylinder]
    shr dx, 2
    and dl, 0xc0
    mov cl, [current_sector]
    and cl, 0x3f
    or cl, dl

    mov ah, 0x02
    mov al, [current_count]
    mov ch, [current_cylinder]
    mov dh, [current_head]
    mov dl, [boot_drive_index]
    mov es, [current_dest_seg]
    xor bx, bx

    int 0x13

    jc .error

    movzx eax, al
    jmp .done

.error:
    xor eax, eax

.done:
    pop es
    pop ebx
    o32 ret


;
; Call load_sectors up to 3 times and report error on failure
;

safe_load_sectors:
    push ebx

    mov ebx, 3

.retry:
    o32 call dword load_sectors

    test eax, eax
    jnz .success

    o32 call dword reset_drive
    dec ebx
    jnz .retry

    push dword 'E'
    o32 call dword print_char
    add esp, 4

    jmp halt

.success:
    push dword '.'
    o32 call dword print_char
    add esp, 4

    pop ebx
    o32 ret


;
; Load current_count with remaining_sectors and limit it to not exceed:
; - remaining sectors in the current track
; - remaining sectors to the 64KB boundary
; - the BIOS limit of 127
;

update_current_count:
    pushad

    mov ax, [remaining_sectors]

    ; AX = min(AX, SPT - (sector - 1))
    movzx bx, byte [boot_drive_spt]
    movzx dx, byte [current_sector]
    sub bx, dx
    inc bx
    o32 call dword get_min_word

    ; AX = min(AX, (0x1000 - (dest_segment & 0xfff)) / 32)
    ; 64KB = 0x1000 paragraphs, one sector = 32 paragraphs
    mov dx, [current_dest_seg]
    and dx, 0x0fff
    mov bx, 0x1000
    sub bx, dx
    shr bx, 5
    o32 call dword get_min_word

    ; AX = min(AX, 127)
    mov bx, 127
    o32 call dword get_min_word

    mov [current_count], al

    popad
    o32 ret


;
; Keep loading sectors until remaining_sectors is 0
;

safe_load_remaining_sectors:
    pushad

.loop:
    cmp word [remaining_sectors], 0
    je .done

    o32 call dword update_current_chs
    o32 call dword update_current_count
    o32 call dword safe_load_sectors

    mov cx, ax
    sub [remaining_sectors], cx     ; remaining_sectors -= count
    add [current_lba], cx           ; current_lba += count
    shl cx, 5
    add [current_dest_seg], cx      ; current_dest_seg += count * 32

    jmp .loop

.done:
    popad
    o32 ret


;
; MBR partition table with a single bootable partition
;

global mbr_partition_table:data
mbr_partition_table:
    times 0x1be - ($ - $$) db 0
    db 0x80, 0x00, 0x02, 0x00
    db 0x01, 0x00, 0x3f, 0x00
    dd 0x01, 0x7f


;
; Boot-loader designator
;

global mbr_designator:data
mbr_designator:
    times 0x1fe - ($ - $$) db 0
    dw 0b10101010_01010101


;;
;; Stage 2
;;


;
; Stage 2 main function
;

stage2_main:
    push dword '.'
    o32 call dword print_char
    add esp, 4

    ; Load memory stats
    o32 call dword load_lower_mem
    o32 call dword load_upper_mem

    ; Load kernel
    mov word [current_dest_seg], KERNEL_DEST >> 4
    mov word [current_lba], KERNEL_START_LBA
    mov word [remaining_sectors], KERNEL_SECTORS
    o32 call dword safe_load_remaining_sectors

    ; Try enabling A20 using BIOS
    mov ax, 0x2401
    int 0x15

    ; Call C code
    o32 call dword cmain

    ; Go to protected mode
    jmp start_pmode


;
; Start 32-bit protected mode and jump to the kernel
;
start_pmode:
    cli

    lgdt [gdt_pointer]

    mov eax, cr0
    or eax, 0x01
    mov cr0, eax

    jmp dword 0x08:stage2_main_32

[bits 32]

stage2_main_32:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0xfff0;

    mov ebx, mboot_info

    jmp 0x08:KERNEL_DEST

[bits 16]

;
; Check if a keystroke is waiting
;

global has_key:function
has_key:
    mov ah, 0x01
    int 0x16
    setnz al
    movzx eax, al
    o32 ret


;
; Wait for a keystroke and return ASCII code
;

global get_key:function
get_key:
    xor ah, ah
    int 0x16
    movzx eax, al
    o32 ret


;
; Print a null-terminated string
;

global print_str:function
print_str:
    push ebp
    mov ebp, esp
    pushad

    mov esi, [ebp + 8]

.loop:
    lodsb
    test al, al
    jz .done

    mov ah, 0x0e
    xor bx, bx
    int 0x10

    jmp .loop

.done:
    popad
    pop ebp
    o32 ret


;
; Print an unsigned 16-bit integer in decimal
;

global print_ushort:function
print_ushort:
    push ebp
    mov ebp, esp
    pushad

    ; Push digits in reverse order, save count in CX
    mov eax, [ebp + 8]
    mov bx, 10
    xor cx, cx
.divide:
    xor dx, dx
    div bx
    push dx
    inc cx
    test ax, ax
    jnz .divide

    ; Pop digits and print
.print:
    pop ax
    add al, '0'
    mov ah, 0x0e
    xor bx, bx
    int 0x10
    loop .print

    popad
    pop ebp
    o32 ret


;
; Load the amount of lower memory in KB (int 0x12)
;

load_lower_mem:
    pushad

    int 0x12
    movzx eax, ax
    mov [mboot_info + 4], eax

    popad
    o32 ret


;
; Load the amount of upper memory in KB (int 0x15, ah=0x88)
;

load_upper_mem:
    pushad

    mov ah, 0x88
    int 0x15
    jc .done
    movzx eax, ax
    mov [mboot_info + 8], eax

.done:
    popad
    o32 ret


;
; Multiboot info
;

mboot_info:
    dd 0x205                ; flags (mem | cmdline | bootloader)
    dd 0                    ; mem_lower
    dd 0                    ; mem_upper
    dd 0                    ; boot_device (unused)
    dd mboot_cmdline        ; cmdline
    times 11 dd 0           ; unused
    dd mboot_loader_name    ; boot_loader_name

mboot_cmdline:      db `\0`
mboot_loader_name:  db `GentleBoot\0`


;
; Temporary GDT, same as in kernel
;

align 16
gdt:
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

gdt_pointer:
    dw (3 * 8 - 1)          ; limit (3 descriptors * 8 bytes - 1)
    dd gdt     ; base (pointer to the GDT)

