OUTPUT_FORMAT(elf32-i386)

SECTIONS {

    . = 0x7c00;

    .all : {
        KEEP(*(.asm))

        *(.text*)
        *(.data*)
        *(.rodata*)
        *(.bss*)
        *(COMMON)
    }

    .empty : {
        *(.rel.*)
        *(.got*)
        *(.igot*)
        *(.plt*)
        *(.iplt*)
    }
    ASSERT(SIZEOF(.empty) == 0, "unexpected non-empty ELF sections")

    /DISCARD/ : {
        *(.eh_frame)
        *(.comment)
        *(.note.GNU-stack)
        *(.note.gnu.property)
    }

}
