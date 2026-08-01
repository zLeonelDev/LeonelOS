; LeonelOS - x86_64 interrupt stubs.
; Valid 64-bit ISR/IRQ wrappers. No 32-bit instructions (pusha/popa are illegal
; in long mode). Each stub pushes the error code (or 0), then the vector number,
; then jumps to a common handler that saves all GPRs, calls the C handler with
; (vector, error_code, InterruptFrame*), restores state and iretq's back.

[BITS 64]

extern isr_handler
extern irq_handler_entry

%macro ISR_NOERR 1
global isr%1
isr%1:
    cli
    push qword 0
    push qword %1
    jmp isr_common_stub
%endmacro

%macro ISR_ERR 1
global isr%1
isr%1:
    cli
    push qword %1
    jmp isr_common_stub
%endmacro

ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR   8
ISR_NOERR 9
ISR_ERR   10
ISR_ERR   11
ISR_ERR   12
ISR_ERR   13
ISR_ERR   14
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR   17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31

%macro IRQ 1
global irq%1
irq%1:
    cli
    push qword 0
    push qword %1
    jmp irq_common_stub
%endmacro

IRQ 0
IRQ 1
IRQ 2
IRQ 3
IRQ 4
IRQ 5
IRQ 6
IRQ 7
IRQ 8
IRQ 9
IRQ 10
IRQ 11
IRQ 12
IRQ 13
IRQ 14
IRQ 15

section .text

; Stack layout on entry to isr_common_stub (top of stack first):
;   [vector][error_code][rip][cs][rflags][rsp][ss]
; After we push 15 GPRs, offsets become:
;   [rsp+15*8] = vector
;   [rsp+16*8] = error_code
;   [rsp+17*8] = saved rip  (InterruptFrame*)
isr_common_stub:
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rbp
    push rdi
    push rsi
    push rdx
    push rcx
    push rbx
    push rax

    mov rdi, [rsp + 15*8]       ; interrupt_no
    mov rsi, [rsp + 16*8]       ; error_code
    lea rdx, [rsp + 17*8]       ; InterruptFrame* (points at saved rip)

    call isr_handler

    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop rbp
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15

    add rsp, 16
    iretq

irq_common_stub:
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rbp
    push rdi
    push rsi
    push rdx
    push rcx
    push rbx
    push rax

    mov rdi, [rsp + 15*8]       ; irq number (0-15)
    mov rsi, [rsp + 16*8]       ; error_code (always 0)
    lea rdx, [rsp + 17*8]       ; InterruptFrame*

    call irq_handler_entry

    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop rbp
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15

    add rsp, 16
    iretq
