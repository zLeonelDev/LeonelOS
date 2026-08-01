#include <virtual.h>
#include <types.h>
#include <debug.h>
#include <physical.h>

#define PAGING_LAYER_COUNT 4
#define PML4_INDEX(vaddr)  (((vaddr) >> 39) & 0x1FF)
#define PDP_INDEX(vaddr)   (((vaddr) >> 30) & 0x1FF)
#define PD_INDEX(vaddr)    (((vaddr) >> 21) & 0x1FF)
#define PT_INDEX(vaddr)    (((vaddr) >> 12) & 0x1FF)

#define PAGE_PRESENT  (1ULL << 0)
#define PAGE_WRITE    (1ULL << 1)
#define PAGE_USER     (1ULL << 2)
#define PAGE_WRITE_THROUGH  (1ULL << 3)
#define PAGE_CACHE_DISABLE  (1ULL << 4)
#define PAGE_ACCESSED (1ULL << 5)
#define PAGE_DIRTY    (1ULL << 6)
#define PAGE_HUGE     (1ULL << 7)
#define PAGE_GLOBAL   (1ULL << 8)
#define PAGE_EXECUTE_DISABLE (1ULL << 63)

#define KERNEL_PAGE_FLAGS (PAGE_PRESENT | PAGE_WRITE | PAGE_GLOBAL)

static u64* g_pml4 = NULL;
static u64* g_pdpt = NULL;

static u64* alloc_page_table() {
    u64* table = (u64*)allocate_physical(1);
    if (table) {
        for (int i = 0; i < 512; i++) {
            table[i] = 0;
        }
    }
    return table;
}

void init_virtual_memory() {
    g_pml4 = (u64*)allocate_physical(1);
    if (!g_pml4) {
        debug_log("ERROR: Failed to allocate PML4\n");
        return;
    }
    
    for (int i = 0; i < 512; i++) {
        g_pml4[i] = 0;
    }
    
    g_pdpt = alloc_page_table();
    if (!g_pdpt) {
        debug_log("ERROR: Failed to allocate PDPT\n");
        return;
    }
    
    g_pml4[PML4_INDEX(KERNEL_BASE)] = (u64)g_pdpt | KERNEL_PAGE_FLAGS;
    
    for (int i = 0; i < 512; i++) {
        u64* pd = alloc_page_table();
        if (!pd) continue;
        
        g_pdpt[PDP_INDEX(KERNEL_BASE + i * 0x40000000)] = (u64)pd | KERNEL_PAGE_FLAGS;
        
        for (int j = 0; j < 512; j++) {
            phys_addr_t phys = (phys_addr_t)((KERNEL_BASE + i * 0x40000000 + j * 0x200000) & ~0x1FFFFF);
            pd[PD_INDEX(phys)] = phys | KERNEL_PAGE_FLAGS;
        }
    }
    
    u64 cr3 = (u64)g_pml4;
    asm volatile("mov %0, %%cr3" : : "r"(cr3));
    
    u64 cr4;
    asm volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1ULL << 5);
    asm volatile("mov %0, %%cr4" : : "r"(cr4));
    
    u64 efer;
    u32 efer_lo, efer_hi;
    asm volatile("rdmsr" : "=a"(efer_lo), "=d"(efer_hi) : "c"(0xC0000080));
    efer = ((u64)efer_hi << 32) | efer_lo;
    efer |= (1ULL << 8);
    asm volatile("wrmsr" : : "a"(efer & 0xFFFFFFFF), "d"(efer >> 32), "c"(0xC0000080));
    
    u64 cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= (1ULL << 31);
    asm volatile("mov %0, %%cr0" : : "r"(cr0));
    
    debug_log("Virtual memory initialized\n");
}

void init_virtual_memory_kernel() {
    init_virtual_memory();
}

virt_addr_t allocate_virtual(u64 pages) {
    UNUSED(pages);
    return 0;
}

void free_virtual(virt_addr_t addr, u64 pages) {
    UNUSED(addr);
    UNUSED(pages);
}

virt_addr_t map_physical(phys_addr_t phys, u64 pages, u32 flags) {
    UNUSED(phys);
    UNUSED(pages);
    UNUSED(flags);
    return 0;
}

void unmap_physical(virt_addr_t addr, u64 pages) {
    UNUSED(addr);
    UNUSED(pages);
}

virt_addr_t virtual_to_physical(virt_addr_t virt) {
    u64 pml4_idx = PML4_INDEX(virt);
    u64 pdpt_idx = PDP_INDEX(virt);
    u64 pd_idx   = PD_INDEX(virt);
    u64 pt_idx   = PT_INDEX(virt);
    
    if (!(g_pml4[pml4_idx] & PAGE_PRESENT)) return 0;
    
    u64* pdpt = (u64*)(g_pml4[pml4_idx] & ~0xFFF);
    if (!(pdpt[pdpt_idx] & PAGE_PRESENT)) return 0;
    
    u64* pd = (u64*)(pdpt[pdpt_idx] & ~0xFFF);
    if (!(pd[pd_idx] & PAGE_PRESENT)) return 0;
    
    if (pd[pd_idx] & PAGE_HUGE) {
        return (pd[pd_idx] & ~0x1FFFFF) | (virt & 0x1FFFFF);
    }
    
    u64* pt = (u64*)(pd[pd_idx] & ~0xFFF);
    if (!(pt[pt_idx] & PAGE_PRESENT)) return 0;
    
    return (pt[pt_idx] & ~0xFFF) | (virt & 0xFFF);
}