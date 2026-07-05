;
; Copyright (c) 2019-2026 luke8086.
; Distributed under the terms of GPL-2 License.
;
; File: boot1.s - Stage 1 bootloader
;

[org 0x7c00]
[cpu 8086]

BOOT2_SEGMENT       equ 0x8000
BOOT2_LBA           equ 3
BOOT2_SECTORS       equ 4
LOAD_RETRY_COUNT    equ 3

    jmp 0x0000:.ensure_cs
.ensure_cs:

    ; Setup segments and stack
    cli
    mov ax, BOOT2_SEGMENT
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0xf000
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
    mov al, BOOT2_SECTORS
    mov bx, 0
    mov dl, [cs:drive_index]
    xor dh, dh                  ; Head 0
    xor ch, ch                  ; Cylinder 0
    mov cl, BOOT2_LBA           ; Start sector (1-indexed)
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

    jmp BOOT2_SEGMENT:0x0

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

; MBR partition table with a single bootable partition
times 0x1be - ($ - $$) db 0
db 0x80, 0x00, 0x02, 0x00
db 0x01, 0x00, 0x3f, 0x00
dd 0x01, 0x7f

; Boot-loader designator
times 0x1fe - ($ - $$) db 0
dw 0b10101010_01010101
