ENTRY(efi_main)

SECTIONS
{
    . = 0x0;

    .text : {
        *(.text)
        *(.text.*)
        *(.rodata*)
    }

    .reloc : {
        *(.reloc)
    }

    .data : {
        *(.data)
        *(.data.*)
    }

    .bss : {
        *(.bss)
        *(.bss.*)
        *(COMMON)
    }

    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
    }
}