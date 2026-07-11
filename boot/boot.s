;
; Copyright (c) 2019-2026 luke8086.
; Distributed under the terms of GPL-2 License.
;
; File: boot.s - Combined 2-stage bootloader
;

[org 0x7c00]
[cpu 386]

%ifndef KERNEL_SECTORS
%define KERNEL_SECTORS 256
%endif

STAGE2_START_SECTOR equ 3
STAGE2_SECTORS      equ 4
LOAD_RETRY_COUNT    equ 3
KERNEL_DEST         equ 0x00010000
KERNEL_START_LBA    equ 6


;;
;; Stage 1
;;


    jmp 0x0000:.ensure_cs
.ensure_cs:

    ; Setup segments and stack
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0xfff0
    sti

    ; Preserve disk number
    mov [cs:drive_index], dl

    ; Load stage 2 loader
    mov di, LOAD_RETRY_COUNT
.load_stage2:

    ; Reset disk
    xor ah, ah
    int 0x13

    ; Load from disk
    mov ah, 0x02
    mov al, STAGE2_SECTORS
    mov bx, 0x7e00
    mov dl, [cs:drive_index]
    xor dh, dh                  ; Head 0
    xor ch, ch                  ; Cylinder 0
    mov cl, STAGE2_START_SECTOR
    int 0x13

    ; Load succeeded
    jnc .load_success

    ; Retry LOAD_RETRY_COUNT times
    dec di
    jnz .load_stage2
    jmp .load_error

    ; Jump to stage 2
.load_success:
    mov ah, 0x0e
    mov al, '.'
    xor bx, bx
    int 0x10

    mov al, [cs:drive_index]
    push ax

    jmp stage2_main

    ; All retries failed
.load_error:
    mov ah, 0x0e
    mov al, 'E'
    xor bx, bx
    int 0x10

.halt:
    cli
    hlt
    jmp .halt

drive_index: db 0

;
; MBR partition table with a single bootable partition
;

times 0x1be - ($ - $$) db 0
db 0x80, 0x00, 0x02, 0x00
db 0x01, 0x00, 0x3f, 0x00
dd 0x01, 0x7f

;
; Boot-loader designator
;

times 0x1fe - ($ - $$) db 0
dw 0b10101010_01010101


;;
;; Stage 2
;;

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
    ret


;
; Print current character
;

putc:
    pushad

    mov ah, 0x0e
    mov al, [current_char]
    xor bx, bx
    int 0x10

    popad
    ret


;
; Load the amount of lower memory in KB (int 0x12)
;

load_lower_mem:
    pushad

    int 0x12
    movzx eax, ax
    mov [mboot_info + 4], eax

    popad
    ret


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
    ret


;
; Reset the boot drive
;

reset_drive:
    pushad

    xor ah, ah
    mov dl, [boot_drive_index]
    int 0x13

    popad
    ret


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
    ret


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
    ret


;
; Call load_sectors up to 3 times and report error on failure
;

safe_load_sectors:
    push ebx

    mov ebx, 3

.retry:
    call load_sectors

    test eax, eax
    jnz .success

    call reset_drive
    dec ebx
    jnz .retry

    mov byte [current_char], 'E'
    call putc

    jmp halt

.success:
    mov byte [current_char], '.'
    call putc

    pop ebx
    ret


;
; Limit current sector count to not exceed:
; - remaining sectors in the current tracks
; - remaining sectors to the 64KB boundary
; - the BIOS limit of 127
;

limit_current_count:
    pushad

    mov ax, [remaining_sectors]

    ; AX = min(AX, SPT - (sector - 1))
    movzx bx, byte [boot_drive_spt]
    movzx dx, byte [current_sector]
    sub bx, dx
    inc bx
    call get_min_word

    ; AX = min(AX, (0x1000 - (dest_segment & 0xfff)) / 32)
    ; 64KB = 0x1000 paragraphs, one sector = 32 paragraphs
    mov dx, [current_dest_seg]
    and dx, 0x0fff
    mov bx, 0x1000
    sub bx, dx
    shr bx, 5
    call get_min_word

    ; AX = min(AX, 127)
    mov bx, 127
    call get_min_word

    mov [current_count], al

    popad
    ret


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
    ret


;
; Load the kernel from disk
;

load_kernel:
    pushad

.loop:
    cmp word [remaining_sectors], 0
    je .done

    call update_current_chs
    call limit_current_count
    call safe_load_sectors

    mov cx, ax
    sub [remaining_sectors], cx     ; remaining_sectors -= count
    add [current_lba], cx           ; current_lba += count
    shl cx, 5
    add [current_dest_seg], cx      ; current_dest_seg += count * 32

    jmp .loop

.done:
    popad
    ret


;
; Main function
;

stage2_main:
    mov byte [current_char], '.'
    call putc

    ; Retrieve disk number from stage 1
    pop ax
    mov [boot_drive_index], al

    call load_lower_mem
    call load_upper_mem

    call reset_drive
    call load_drive_geometry
    call load_kernel

    ; Try enabling A20 using BIOS
    mov ax, 0x2401
    int 0x15

    cli

    lgdt [gdt_pointer]

    mov eax, cr0
    or eax, 0x01
    mov cr0, eax

    jmp dword 0x08:main_32

[bits 32]

main_32:
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


;
; Global variables
;

current_char        db '.'

boot_drive_index    db 0
boot_drive_spt      db 9
boot_drive_heads    db 2

remaining_sectors   dw KERNEL_SECTORS

current_lba         dw KERNEL_START_LBA
current_dest_seg    dw KERNEL_DEST >> 4
current_cylinder    dw 0
current_head        db 0
current_sector      db 0
current_count       db 0


;
; Padding to 512 + 2048 bytes (5 sectors)
;

times (512 + 2048) - ($ - $$) db 0
