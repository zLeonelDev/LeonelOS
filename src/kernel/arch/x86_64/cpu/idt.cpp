#include <arch.h>
#include "idt.h"

static IDTEntry idt_entries[IDT_NUM_ENTRIES];
static IDTR idtr;

extern "C" void isr0();
extern "C" void isr1();
extern "C" void isr2();
extern "C" void isr3();
extern "C" void isr4();
extern "C" void isr5();
extern "C" void isr6();
extern "C" void isr7();
extern "C" void isr8();
extern "C" void isr9();
extern "C" void isr10();
extern "C" void isr11();
extern "C" void isr12();
extern "C" void isr13();
extern "C" void isr14();
extern "C" void isr15();
extern "C" void isr16();
extern "C" void isr17();
extern "C" void isr18();
extern "C" void isr19();
extern "C" void isr20();
extern "C" void isr21();
extern "C" void isr22();
extern "C" void isr23();
extern "C" void isr24();
extern "C" void isr25();
extern "C" void isr26();
extern "C" void isr27();
extern "C" void isr28();
extern "C" void isr29();
extern "C" void isr30();
extern "C" void isr31();

extern "C" void irq0();
extern "C" void irq1();
extern "C" void irq2();
extern "C" void irq3();
extern "C" void irq4();
extern "C" void irq5();
extern "C" void irq6();
extern "C" void irq7();
extern "C" void irq8();
extern "C" void irq9();
extern "C" void irq10();
extern "C" void irq11();
extern "C" void irq12();
extern "C" void irq13();
extern "C" void irq14();
extern "C" void irq15();

static void idt_set_gate(u8 vector, u64 handler, u8 selector, u8 type) {
    idt_entries[vector].offset_low   = (handler & 0xFFFF);
    idt_entries[vector].selector    = selector;
    idt_entries[vector].ist         = 0;
    idt_entries[vector].type_attr   = type;
    idt_entries[vector].offset_mid  = (handler >> 16) & 0xFFFF;
    idt_entries[vector].offset_high = (handler >> 32) & 0xFFFFFFFF;
    idt_entries[vector].reserved    = 0;
}

void idt_init() {
    idtr.limit = (sizeof(IDTEntry) * IDT_NUM_ENTRIES) - 1;
    idtr.base  = (u64)&idt_entries;
    
    for (int i = 0; i < IDT_NUM_ENTRIES; i++) {
        idt_set_gate(i, 0, 0x08, 0x8E);
    }
    
    idt_set_gate(0,  (u64)isr0,  0x08, 0x8E);
    idt_set_gate(1,  (u64)isr1,  0x08, 0x8E);
    idt_set_gate(2,  (u64)isr2,  0x08, 0x8E);
    idt_set_gate(3,  (u64)isr3,  0x08, 0x8E);
    idt_set_gate(4,  (u64)isr4,  0x08, 0x8E);
    idt_set_gate(5,  (u64)isr5,  0x08, 0x8E);
    idt_set_gate(6,  (u64)isr6,  0x08, 0x8E);
    idt_set_gate(7,  (u64)isr7,  0x08, 0x8E);
    idt_set_gate(8,  (u64)isr8,  0x08, 0x8E);
    idt_set_gate(9,  (u64)isr9,  0x08, 0x8E);
    idt_set_gate(10, (u64)isr10, 0x08, 0x8E);
    idt_set_gate(11, (u64)isr11, 0x08, 0x8E);
    idt_set_gate(12, (u64)isr12, 0x08, 0x8E);
    idt_set_gate(13, (u64)isr13, 0x08, 0x8E);
    idt_set_gate(14, (u64)isr14, 0x08, 0x8E);
    idt_set_gate(15, (u64)isr15, 0x08, 0x8E);
    idt_set_gate(16, (u64)isr16, 0x08, 0x8E);
    idt_set_gate(17, (u64)isr17, 0x08, 0x8E);
    idt_set_gate(18, (u64)isr18, 0x08, 0x8E);
    idt_set_gate(19, (u64)isr19, 0x08, 0x8E);
    idt_set_gate(20, (u64)isr20, 0x08, 0x8E);
    idt_set_gate(21, (u64)isr21, 0x08, 0x8E);
    idt_set_gate(22, (u64)isr22, 0x08, 0x8E);
    idt_set_gate(23, (u64)isr23, 0x08, 0x8E);
    idt_set_gate(24, (u64)isr24, 0x08, 0x8E);
    idt_set_gate(25, (u64)isr25, 0x08, 0x8E);
    idt_set_gate(26, (u64)isr26, 0x08, 0x8E);
    idt_set_gate(27, (u64)isr27, 0x08, 0x8E);
    idt_set_gate(28, (u64)isr28, 0x08, 0x8E);
    idt_set_gate(29, (u64)isr29, 0x08, 0x8E);
    idt_set_gate(30, (u64)isr30, 0x08, 0x8E);
    idt_set_gate(31, (u64)isr31, 0x08, 0x8E);
    
    idt_set_gate(32, (u64)irq0,  0x08, 0x8E);
    idt_set_gate(33, (u64)irq1,  0x08, 0x8E);
    idt_set_gate(34, (u64)irq2,  0x08, 0x8E);
    idt_set_gate(35, (u64)irq3,  0x08, 0x8E);
    idt_set_gate(36, (u64)irq4,  0x08, 0x8E);
    idt_set_gate(37, (u64)irq5,  0x08, 0x8E);
    idt_set_gate(38, (u64)irq6,  0x08, 0x8E);
    idt_set_gate(39, (u64)irq7,  0x08, 0x8E);
    idt_set_gate(40, (u64)irq8,  0x08, 0x8E);
    idt_set_gate(41, (u64)irq9,  0x08, 0x8E);
    idt_set_gate(42, (u64)irq10, 0x08, 0x8E);
    idt_set_gate(43, (u64)irq11, 0x08, 0x8E);
    idt_set_gate(44, (u64)irq12, 0x08, 0x8E);
    idt_set_gate(45, (u64)irq13, 0x08, 0x8E);
    idt_set_gate(46, (u64)irq14, 0x08, 0x8E);
    idt_set_gate(47, (u64)irq15, 0x08, 0x8E);
    
    asm volatile("lidt %0" : : "m"(idtr));
}