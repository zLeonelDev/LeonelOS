#ifndef LEONELOS_IRQ_H
#define LEONELOS_IRQ_H

#include <types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*IRQHandler)(void);

void irq_register_handler(u8 irq, IRQHandler handler);
void irq_handler_entry(u64 irq, u64 error_code, void* frame);

#ifdef __cplusplus
}
#endif

#endif
