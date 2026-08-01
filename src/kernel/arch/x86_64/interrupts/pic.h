#ifndef LEONELOS_PIC_H
#define LEONELOS_PIC_H

#include <types.h>

void pic_remap(u8 offset_master, u8 offset_slave);
void pic_init();
void pic_eoi(u8 irq);
void irq_enable(u8 irq);
void irq_disable(u8 irq);

#endif