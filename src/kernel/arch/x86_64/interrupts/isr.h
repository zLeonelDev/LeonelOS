#ifndef LEONELOS_ISR_H
#define LEONELOS_ISR_H

#include <types.h>

#ifdef __cplusplus
extern "C" {
#endif

void isr_handler(u64 interrupt_no, u64 error_code, InterruptFrame* frame);

#ifdef __cplusplus
}
#endif

#endif