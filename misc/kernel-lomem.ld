OUTPUT_FORMAT(elf32-i386)
ENTRY(krn_link_start)

krn_link_start = 0x00010000;

SECTIONS {

    . = krn_link_start;

    krn_link_start = .;

    .text : {
        krn_link_text_start = .;
        KEEP(*(.entry))
        *(.text*)
    }
    krn_link_text_end = .;

    .data : {
        krn_link_data_start = .;
        *(.data*)
        *(.rodata*)
        *(.eh_frame)
    }
    krn_link_data_end = .;

    .empty : {
        *(.rel.*)
        *(.got*)
        *(.igot*)
        *(.plt*)
        *(.iplt*)
    }
    ASSERT(SIZEOF(.empty) == 0, "unexpected non-empty ELF sections")

    .bss : ALIGN(0x1000) {
        krn_link_bss_start = .;
        *(.bss*)
        *(COMMON)
    }
    . = ALIGN(0x10);
    krn_link_bss_end = .;

    krn_link_end = .;

    /DISCARD/ : {
        *(.comment)
        *(.note.GNU-stack)
        *(.note.gnu.property)
    }

}
