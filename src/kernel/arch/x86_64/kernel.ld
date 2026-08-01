ENTRY(_start)

SECTIONS
{
    . = 0xFFFFFFFF80000000 + 0x100000;

    .text ALIGN(0x1000) :
    {
        *(.text._start)
        *(.text.boot)
        *(.text)
        *(.text.*)
        *(.rodata*)
        *(.eh_frame*)
    }

    .rodata ALIGN(0x1000) :
    {
        *(.rodata*)
    }

    .data ALIGN(0x1000) :
    {
        *(.data)
        *(.data.*)
        *(.bss)
        *(.bss.*)
        *(COMMON)
    }

    .bss ALIGN(0x1000) :
    {
        *(.bss)
        *(.bss.*)
        *(COMMON)
    }

    /DISCARD/ :
    {
        *(.comment)
        *(.eh_frame)
        *(.eh_frame_hdr)
        *(.note.gnu.build-id)
        *(.note.gnu.property)
    }

    .kernel_stack ALIGN(0x1000) :
    {
        . = . + 0x20000;
        kernel_stack_top = .;
    }
}