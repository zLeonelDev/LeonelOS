[BITS 64]

global _start
extern kernel_main

section .text
_start:
    cli

    ; Arguments from the bootloader follow the Microsoft x64 calling
    ; convention used by the UEFI PE application (RCX/RDX/R8), not System V.
    ; Reload into System V registers before calling kernel_main.
    mov [saved_framebuffer], rcx   ; rcx = FramebufferInfo*
    mov [saved_assets],      rdx   ; rdx = BootAssets*
    mov [saved_memory_map],  r8    ; r8  = MemoryMap*

    ; Switch to the kernel stack.
    mov rsp, kernel_stack_top

    ; Clear frame pointer for a clean backtrace.
    xor rbp, rbp

    ; Reload arguments.
    mov rdi, [saved_framebuffer]
    mov rsi, [saved_assets]
    mov rdx, [saved_memory_map]

    call kernel_main

.hang:
    hlt
    jmp .hang

section .bss
align 16
kernel_stack:
    resb 0x40000
kernel_stack_top:

section .data
saved_framebuffer: dq 0
saved_assets:      dq 0
saved_memory_map:  dq 0
