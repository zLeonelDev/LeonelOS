#ifndef LEONELOS_TIMER_H
#define LEONELOS_TIMER_H

#include <types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TIMER_FREQUENCY 100

void timer_init(u32 frequency);
u64 timer_get_ticks(void);
void timer_sleep(u32 ms);

#ifdef __cplusplus
}
#endif

#endif
