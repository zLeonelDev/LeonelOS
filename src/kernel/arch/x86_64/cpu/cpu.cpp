#include "cpu.h"
#include <debug.h>

CPUInfo cpu_info;

void cpu_init() {
    u32 eax, ebx, ecx, edx;
    
    asm volatile("cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(0)
    );
    
    cpu_info.vendor[0] = ebx;
    cpu_info.vendor[1] = edx;
    cpu_info.vendor[2] = ecx;
    cpu_info.vendor[3] = 0;
    
    asm volatile("cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(1)
    );
    
    cpu_info.family = (eax >> 8) & 0xF;
    cpu_info.model = (eax >> 4) & 0xF;
    cpu_info.stepping = eax & 0xF;
    cpu_info.microcode = ebx;
    cpu_info.max_physical_address = (eax >> 26) & 0x3F;
    cpu_info.max_logical_address = (ebx >> 16) & 0xFF;
    
    cpu_info.has_sse = (edx >> 25) & 1;
    cpu_info.has_sse2 = (edx >> 26) & 1;
    cpu_info.has_sse3 = ecx & 1;
    cpu_info.has_sse4_1 = (ecx >> 19) & 1;
    cpu_info.has_sse4_2 = (ecx >> 20) & 1;
    cpu_info.has_avx = (ecx >> 28) & 1;
    
    if (cpu_info.has_avx) {
        u32 xcr0_lo, xcr0_hi;
        asm volatile("xgetbv" : "=a"(xcr0_lo), "=d"(xcr0_hi) : "c"(0));
        cpu_info.xcr0 = ((u64)xcr0_hi << 32) | xcr0_lo;
        cpu_info.has_osxsave = (cpu_info.xcr0 & 0x6) == 0x6;
    }
    
    debug_log("CPU: Vendor=%s Family=%u Model=%u Stepping=%u\n",
              (char*)cpu_info.vendor, cpu_info.family, cpu_info.model, cpu_info.stepping);
}

void cpu_get_info(CPUInfo* info) {
    *info = cpu_info;
}

u64 cpu_read_cr0() { u64 val; asm volatile("mov %%cr0, %0" : "=r"(val)); return val; }
u64 cpu_read_cr2() { u64 val; asm volatile("mov %%cr2, %0" : "=r"(val)); return val; }
u64 cpu_read_cr3() { u64 val; asm volatile("mov %%cr3, %0" : "=r"(val)); return val; }
u64 cpu_read_cr4() { u64 val; asm volatile("mov %%cr4, %0" : "=r"(val)); return val; }
void cpu_write_cr0(u64 val) { asm volatile("mov %0, %%cr0" : : "r"(val)); }
void cpu_write_cr2(u64 val) { asm volatile("mov %0, %%cr2" : : "r"(val)); }
void cpu_write_cr3(u64 val) { asm volatile("mov %0, %%cr3" : : "r"(val)); }
void cpu_write_cr4(u64 val) { asm volatile("mov %0, %%cr4" : : "r"(val)); }
u64 cpu_read_msr(u32 msr) {
    u32 lo, hi;
    asm volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    return ((u64)hi << 32) | lo;
}
void cpu_write_msr(u32 msr, u64 val) {
    asm volatile("wrmsr" : : "a"(val & 0xFFFFFFFF), "d"(val >> 32), "c"(msr));
}
u64 cpu_read_tsc() {
    u32 lo, hi;
    asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((u64)hi << 32) | lo;
}
void cpu_halt() { asm volatile("hlt"); }
void cpu_set_gdt(u64 gdt_base, u16 limit) {
    DescriptorTablePtr ptr = {gdt_base, limit};
    asm volatile("lgdt %0" : : "m"(ptr));
}
void cpu_lidt(u64 idt_base, u16 limit) {
    DescriptorTablePtr ptr = {idt_base, limit};
    asm volatile("lidt %0" : : "m"(ptr));
}