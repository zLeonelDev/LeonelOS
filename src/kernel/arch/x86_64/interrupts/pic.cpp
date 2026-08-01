#include "pic.h"
#include <io.h>

void pic_remap(u8 offset_master, u8 offset_slave) {
    u8 a1 = port_byte_in(0x21);
    u8 a2 = port_byte_in(0xA1);
    
    port_byte_out(0x20, 0x11);
    port_byte_out(0xA0, 0x11);
    port_byte_out(0x21, offset_master);
    port_byte_out(0xA1, offset_slave);
    port_byte_out(0x21, 4);
    port_byte_out(0xA1, 2);
    port_byte_out(0x21, 1);
    port_byte_out(0xA1, 1);
    port_byte_out(0x21, a1);
    port_byte_out(0xA1, a2);
}

void pic_init() {
    pic_remap(0x20, 0x28);
}

void pic_eoi(u8 irq) {
    if (irq >= 8) {
        port_byte_out(0xA0, 0x20);
    }
    port_byte_out(0x20, 0x20);
}

void irq_enable(u8 irq) {
    if (irq < 8) {
        u8 mask = port_byte_in(0x21);
        mask &= ~(1 << irq);
        port_byte_out(0x21, mask);
    } else {
        u8 mask = port_byte_in(0xA1);
        mask &= ~(1 << (irq - 8));
        port_byte_out(0xA1, mask);
    }
}

void irq_disable(u8 irq) {
    if (irq < 8) {
        u8 mask = port_byte_in(0x21);
        mask |= (1 << irq);
        port_byte_out(0x21, mask);
    } else {
        u8 mask = port_byte_in(0xA1);
        mask |= (1 << (irq - 8));
        port_byte_out(0xA1, mask);
    }
}