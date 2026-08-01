#ifndef LEONELOS_SYSCALL_H
#define LEONELOS_SYSCALL_H

#include <types.h>

void syscall_init();
void syscall_handler();
void syscall_iret();

#endif