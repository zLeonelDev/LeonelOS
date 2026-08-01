#ifndef LEONELOS_CPU_H
#define LEONELOS_CPU_H

#include <types.h>

struct CPUInfo {
    u32 vendor[4];
    u32 family;
    u32 model;
    u32 stepping;
    u32 microcode;
    u32 max_physical_address;
    u32 max_logical_address;
    bool has_sse;
    bool has_sse2;
    bool has_sse3;
    bool has_sse4_1;
    bool has_sse4_2;
    bool has_avx;
    bool has_avx2;
    bool has_osxsave;
    u64 xcr0;
};

extern CPUInfo cpu_info;

void cpu_init();
void cpu_get_info(CPUInfo* info);
void cpu_set_gdt(u64 gdt_base, u16 limit);
void cpu_lidt(u64 idt_base, u16 limit);
void cpu_halt();
u64 cpu_read_cr0();
u64 cpu_read_cr2();
u64 cpu_read_cr3();
u64 cpu_read_cr4();
void cpu_write_cr0(u64 val);
void cpu_write_cr2(u64 val);
void cpu_write_cr3(u64 val);
void cpu_write_cr4(u64 val);
u64 cpu_read_msr(u32 msr);
void cpu_write_msr(u32 msr, u64 val);
u64 cpu_read_tsc();

#endif