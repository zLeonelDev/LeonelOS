#include <arch.h>
#include "cpu.h"
#include "gdt.h"

static GDTEntry gdt_entries[GDT_NUM_ENTRIES];
static GDTR gdtr;

static void gdt_set_gate(int index, u64 base, u64 limit, u8 access, u8 gran) {
    gdt_entries[index].limit_low = (limit & 0xFFFF);
    gdt_entries[index].base_low = (base & 0xFFFF);
    gdt_entries[index].base_mid = (base >> 16) & 0xFF;
    gdt_entries[index].access = access;
    gdt_entries[index].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt_entries[index].base_high = (base >> 24) & 0xFF;
}

void gdt_init() {
    gdtr.limit = (sizeof(GDTEntry) * GDT_NUM_ENTRIES) - 1;
    gdtr.base  = (u64)&gdt_entries;
    
    gdt_set_gate(0, 0, 0, 0, 0);
    
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xAF);
    
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);
    
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xAF);
    
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);
    
    gdt_set_gate(5, 0, 0xFFFFFFFF, 0x89, 0x00);
    
    asm volatile("lgdt %0" : : "m"(gdtr));
    
    asm volatile(
        "movw $0x10, %%ax\n"
        "movw %%ax, %%ds\n"
        "movw %%ax, %%es\n"
        "movw %%ax, %%fs\n"
        "movw %%ax, %%gs\n"
        "movw %%ax, %%ss\n"
        "leaq 1f(%%rip), %%rax\n"
        "pushq $0x08\n"
        "pushq %%rax\n"
        "lretq\n"
        "1:\n"
        :
        :
        : "rax", "memory"
    );
}