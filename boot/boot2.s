;
; Copyright (c) 2026 luke8086
; Distributed under the terms of GPL-2 License
;
; File: boot2.s - Stage 2 bootloader
;

[cpu 386]

%ifndef KERNEL_SECTORS
%define KERNEL_SECTORS 256
%endif

BOOT2_SEGMENT           equ 0x8000
BOOT2_ADDR              equ (BOOT2_SEGMENT << 4)

KERNEL_DEST             equ 0x00010000
KERNEL_START_LBA        equ 6

[bits 16]
[org 0x00]

[section .text]

;
; Entry point
;

start_:
    jmp main


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
; Wait until the 8042 input buffer is empty for writing
;

kbd_wait_write:
    push ecx
    mov ecx, 0x100000

.loop:
    in al, 0x64
    test al, 0x02
    jz .done
    dec ecx
    jnz .loop

.done:
    pop ecx
    ret


;
; Wait until the 8042 output buffer is ready for reading
;

kbd_wait_read:
    push ecx
    mov ecx, 0x100000

.loop:
    in al, 0x64
    test al, 0x01
    jnz .done
    dec ecx
    jnz .loop

.done:
    pop ecx
    ret


;
; Enable the A20 line using 8042 keyboard controller
;

kbd_enable_a20:
    pushad

    ; Disable keyboard
    call kbd_wait_write
    mov al, 0xad
    out 0x64, al

    ; Send command: read controller output port
    call kbd_wait_write
    mov al, 0xd0
    out 0x64, al

    ; Get value of controller output port
    call kbd_wait_read
    in al, 0x60
    mov bl, al

    ; Verify the value by checking the system reset bit
    test al, 0x01
    jz .skip

    ; Send command: write controller output port
    call kbd_wait_write
    mov al, 0xd1
    out 0x64, al

    ; Write new value of controller output port, with A20 bit enabled
    call kbd_wait_write
    mov al, bl
    or al, 0x02
    out 0x60, al

.skip:
    ; Re-enable keyboard
    call kbd_wait_write
    mov al, 0xae
    out 0x64, al

    ; Drain the buffer before returning
    call kbd_wait_write

    popad
    ret


;
; Check if the A20 line is enabled
;

check_a20:
    push es
    push fs
    pushad

    ; ES:SI = 0000:0500 (0x500 linear)
    xor ax, ax
    mov es, ax
    mov si, 0x0500

    ; FS:DI = FFFF:0510 (0x100500 linear)
    not ax
    mov fs, ax
    mov di, 0x0510

    ; Preserve the original bytes
    mov dl, [es:si]
    mov dh, [fs:di]

    ; Check 1
    mov byte [es:si], 0xff
    mov byte [fs:di], 0x00
    cmp byte [es:si], 0x00
    je .disabled

    ; Check 2
    mov byte [es:si], 0x00
    mov byte [fs:di], 0xff
    cmp byte [es:si], 0xff
    je .disabled

    ; A20 enabled
    mov byte [a20_enabled], 1
    jmp .restore

.disabled:
    ; A20 not enabled
    mov byte [a20_enabled], 0

.restore:
    ; Restore the original bytes
    mov [es:si], dl
    mov [fs:di], dh

    popad
    pop fs
    pop es
    ret


;
; Enable the A20 line
;

enable_a20:
    pushad

    ; Check
    call check_a20
    cmp byte [a20_enabled], 0
    jne .done

    ; Try BIOS
    mov ax, 0x2401
    int 0x15

    ; Check
    call check_a20
    cmp byte [a20_enabled], 0
    jne .done

    ; Try Fast A20 gate
    in al, 0x92
    test al, 0x02
    jnz .skip_fast_a20_gate
    or al, 0x02
    and al, 0xFE
    out 0x92, al
.skip_fast_a20_gate:

    ; Check
    call check_a20
    cmp byte [a20_enabled], 0
    jne .done

    ; Try keyboard controller
    call kbd_enable_a20

    ; Check
    call check_a20
    cmp byte [a20_enabled], 0
    jne .done

    ; Handle error
    mov byte [current_char], 'A'
    call putc
    jmp halt

.done:
    popad
    ret


;
; Main function
;

main:
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
    call enable_a20

    cli

    lgdt [gdt_pointer]

    mov eax, cr0
    or eax, 0x01
    mov cr0, eax

    jmp dword 0x08:(BOOT2_ADDR + main_32)

[bits 32]

main_32:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, BOOT2_ADDR + 0xf000;

    mov ebx, BOOT2_ADDR + mboot_info

    jmp 0x08:KERNEL_DEST

[bits 16]

;
; Multiboot info
;

mboot_info:
    dd 0x205                            ; flags (mem | cmdline | bootloader)
    dd 0                                ; mem_lower
    dd 0                                ; mem_upper
    dd 0                                ; boot_device (unused)
    dd BOOT2_ADDR + mboot_cmdline       ; cmdline
    times 11 dd 0                       ; unused
    dd BOOT2_ADDR + mboot_loader_name   ; boot_loader_name

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
    dd BOOT2_ADDR + gdt     ; base (pointer to the GDT)


;
; Global variables
;

current_char        db '.'

a20_enabled         db 0

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
; Padding to 2048 bytes (4 sectors)
;

times 2048 - ($ - $$) db 0
