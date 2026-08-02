ENTRY(_start)

SECTIONS
{
    /* Milestone: kernel runs at a low physical address because the bootloader
     * does not set up higher-half paging yet. UEFI firmware page tables keep
     * this range identity mapped. 0x10000000 (256 MiB) sits inside the free
     * conventional region reported by OVMF, well above GOP framebuffers
     * (typically < 32 MiB) and ~24 MiB of loader allocations. Move back to
     * 0xFFFFFFFF80000000 when the paging subsystem is wired in. */
    . = 0x10000000;

    kernel_start = .;

    .text : ALIGN(0x1000)
    {
        *(.text._start)
        *(.text*)
        *(.rodata*)
        *(.data.rel.ro*)
        . = ALIGN(0x1000);
    }

    .data : ALIGN(0x1000)
    {
        *(.data*)
        . = ALIGN(0x1000);
    }

    .bss : ALIGN(0x1000)
    {
        *(.bss*)
        *(COMMON)
        . = ALIGN(0x1000);
    }

    .kernel_stack : ALIGN(0x1000)
    {
        . = ALIGN(0x1000);
        . = . + 0x40000;
        . = ALIGN(0x1000);
    }

    . = ALIGN(0x1000);
    kernel_end = .;
}