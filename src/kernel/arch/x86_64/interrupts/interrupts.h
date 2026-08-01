#ifndef LEONELOS_INTERRUPTS_H
#define LEONELOS_INTERRUPTS_H

static inline void interrupts_enable(void) {
    asm volatile("sti");
}

static inline void interrupts_disable(void) {
    asm volatile("cli");
}

void interrupts_init(void);

#endif
