#include "irq.h"
#include <debug.h>
#include <pic.h>
#include <types.h>

#define NUM_IRQS 16

static IRQHandler irq_handlers[NUM_IRQS];

void irq_register_handler(u8 irq, IRQHandler handler) {
    if (irq < NUM_IRQS) {
        irq_handlers[irq] = handler;
    }
}

void irq_handler_entry(u64 irq, u64 error_code, void* frame) {
    UNUSED(error_code);
    UNUSED(frame);
    if (irq < NUM_IRQS && irq_handlers[irq]) {
        irq_handlers[irq]();
    }
    pic_eoi((u8)(irq + IRQ_BASE));
}
