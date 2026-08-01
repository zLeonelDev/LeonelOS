#ifndef LEONELOS_SCHEDULER_H
#define LEONELOS_SCHEDULER_H

#include <types.h>

void scheduler_init();
void scheduler_add_thread(Thread* thread);
void scheduler_remove_thread(Thread* thread);
Thread* scheduler_next();
void scheduler_tick();

#endif