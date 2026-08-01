#include "isr.h"
#include <debug.h>

static const char* interrupt_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Flags",
    "Invalid Opcode",
    "No Coprocessor",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",
    "x87 FPU Floating-Point Error",
    "Alignment Check",
    "Machine-Check Exception",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

void isr_handler(u64 interrupt_no, u64 error_code, InterruptFrame* frame) {
    if (interrupt_no < 32) {
        debug_log("EXCEPTION: %s (err=0x%llx) RIP=0x%llx CS=0x%llx RFLAGS=0x%llx\n",
                  interrupt_messages[interrupt_no], error_code,
                  frame->rip, frame->cs, frame->rflags);
    } else {
        debug_log("INTERRUPT: vector=%llu (err=0x%llx)\n", interrupt_no, error_code);
    }

    if (interrupt_no == 14) {
        u64 cr2;
        asm volatile("mov %%cr2, %0" : "=r"(cr2));
        debug_log("  Page fault CR2: 0x%llx\n", cr2);
    }

    for (;;) {
        asm volatile("hlt");
    }
}
