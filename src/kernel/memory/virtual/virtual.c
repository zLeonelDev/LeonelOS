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

static void split_huge_page(u64* pd, u64 pd_idx) {
    u64 huge_entry = pd[pd_idx];
    if (!(huge_entry & PAGE_HUGE)) return;
    phys_addr_t huge_phys = huge_entry & ~0x1FFFFF;
    u64 huge_flags = huge_entry & 0xFFF;

    u64* pt = alloc_page_table();
    if (!pt) return;

    for (int k = 0; k < 512; k++) {
        pt[k] = (huge_phys + (u64)k * PAGE_SIZE) | huge_flags;
    }

    pd[pd_idx] = (u64)pt | (huge_flags & ~(PAGE_HUGE));
}

void init_virtual_memory() {
    debug_log("VM: init start\n");
    g_pml4 = (u64*)allocate_physical(1);
    if (!g_pml4) { debug_log("VM: FAIL PML4\n"); return; }
    debug_log("VM: PML4 ok\n");

    for (int i = 0; i < 512; i++) g_pml4[i] = 0;

    g_pdpt = alloc_page_table();
    if (!g_pdpt) { debug_log("VM: FAIL PDPT\n"); return; }
    debug_log("VM: PDPT ok\n");

    g_pml4[PML4_INDEX(KERNEL_BASE)] = (u64)g_pdpt | KERNEL_PAGE_FLAGS;

    extern char kernel_start[];
    phys_addr_t kernel_phys_base = (phys_addr_t)kernel_start;
    debug_log("VM: kernel_phys_base=0x%llx\n", (unsigned long long)kernel_phys_base);

    for (int i = 0; i < 512; i++) {
        u64* pd = alloc_page_table();
        if (!pd) continue;
        g_pdpt[PDP_INDEX(KERNEL_BASE + i * 0x40000000)] = (u64)pd | KERNEL_PAGE_FLAGS;
        for (int j = 0; j < 512; j++) {
            virt_addr_t virt = (virt_addr_t)(KERNEL_BASE + i * 0x40000000 + j * 0x200000);
            phys_addr_t phys = (phys_addr_t)((u64)i * 0x40000000 + (u64)j * 0x200000) + kernel_phys_base;
            pd[PD_INDEX(virt)] = phys | KERNEL_PAGE_FLAGS | PAGE_HUGE;
        }
    }
    debug_log("VM: tables populated\n");

    u64 cr3 = (u64)g_pml4;
    asm volatile("mov %0, %%cr3" : : "r"(cr3));
    debug_log("VM: CR3 set\n");

    u64 cr4;
    asm volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1ULL << 5);
    asm volatile("mov %0, %%cr4" : : "r"(cr4));
    debug_log("VM: CR4 set\n");

    u64 efer;
    u32 efer_lo, efer_hi;
    asm volatile("rdmsr" : "=a"(efer_lo), "=d"(efer_hi) : "c"(0xC0000080));
    efer = ((u64)efer_hi << 32) | efer_lo;
    efer |= (1ULL << 8);
    asm volatile("wrmsr" : : "a"(efer & 0xFFFFFFFF), "d"(efer >> 32), "c"(0xC0000080));
    debug_log("VM: EFER set\n");

    u64 cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= (1ULL << 31);
    asm volatile("mov %0, %%cr0" : : "r"(cr0));
    debug_log("VM: CR0 set (PG enabled)\n");
}

void init_virtual_memory_kernel() {
    init_virtual_memory();
}

virt_addr_t allocate_virtual(u64 pages) {
    if (!g_pml4 || pages == 0) return 0;

    virt_addr_t start = KERNEL_HEAP_START;
    virt_addr_t end = KERNEL_HEAP_START + KERNEL_HEAP_SIZE;
    u64 found = 0;
    virt_addr_t candidate = 0;

    for (virt_addr_t va = start; va < end; va += PAGE_SIZE) {
        u64 pml4_idx = PML4_INDEX(va);
        u64 pdpt_idx = PDP_INDEX(va);
        u64 pd_idx = PD_INDEX(va);
        u64 pt_idx = PT_INDEX(va);

        if (!(g_pml4[pml4_idx] & PAGE_PRESENT)) {
            found = 0;
            continue;
        }
        u64* pdpt = (u64*)(g_pml4[pml4_idx] & ~0xFFF);
        if (!(pdpt[pdpt_idx] & PAGE_PRESENT)) {
            found = 0;
            continue;
        }
        u64* pd = (u64*)(pdpt[pdpt_idx] & ~0xFFF);
        if (!(pd[pd_idx] & PAGE_PRESENT)) {
            found = 0;
            if (found == 0) { candidate = va; found = 1; }
            continue;
        }
        if (pd[pd_idx] & PAGE_HUGE) {
            found = 0;
            continue;
        }
        u64* pt = (u64*)(pd[pd_idx] & ~0xFFF);
        if (pt[pt_idx] & PAGE_PRESENT) {
            found = 0;
            continue;
        }
        if (found == 0) { candidate = va; found = 1; }
        found++;
        if (found == pages) return candidate;
    }
    return 0;
}

void free_virtual(virt_addr_t addr, u64 pages) {
    if (!g_pml4 || addr == 0 || pages == 0) return;

    for (u64 i = 0; i < pages; i++) {
        virt_addr_t va = addr + i * PAGE_SIZE;
        u64 pml4_idx = PML4_INDEX(va);
        u64 pdpt_idx = PDP_INDEX(va);
        u64 pd_idx = PD_INDEX(va);
        u64 pt_idx = PT_INDEX(va);

        if (!(g_pml4[pml4_idx] & PAGE_PRESENT)) continue;
        u64* pdpt = (u64*)(g_pml4[pml4_idx] & ~0xFFF);
        if (!(pdpt[pdpt_idx] & PAGE_PRESENT)) continue;
        u64* pd = (u64*)(pdpt[pdpt_idx] & ~0xFFF);
        if (!(pd[pd_idx] & PAGE_PRESENT)) continue;

        if (pd[pd_idx] & PAGE_HUGE) {
            pd[pd_idx] = 0;
            continue;
        }

        u64* pt = (u64*)(pd[pd_idx] & ~0xFFF);
        pt[pt_idx] = 0;

        int pt_empty = 1;
        for (int k = 0; k < 512; k++) {
            if (pt[k] != 0) { pt_empty = 0; break; }
        }
        if (pt_empty) {
            free_physical((phys_addr_t)pt, 1);
            pd[pd_idx] = 0;
        }
    }
}

virt_addr_t map_physical(phys_addr_t phys, u64 pages, u32 flags) {
    if (!g_pml4 || pages == 0) return 0;

    virt_addr_t vaddr = allocate_virtual(pages);
    if (!vaddr) return 0;

    for (u64 i = 0; i < pages; i++) {
        virt_addr_t va = vaddr + i * PAGE_SIZE;
        u64 pml4_idx = PML4_INDEX(va);
        u64 pdpt_idx = PDP_INDEX(va);
        u64 pd_idx = PD_INDEX(va);
        u64 pt_idx = PT_INDEX(va);

        u64* pdpt = g_pdpt;

        u64 pd_entry = pdpt[pdpt_idx];
        u64* pd;
        if (!(pd_entry & PAGE_PRESENT)) {
            pd = alloc_page_table();
            if (!pd) return 0;
            pdpt[pdpt_idx] = (u64)pd | KERNEL_PAGE_FLAGS;
        } else {
            pd = (u64*)(pd_entry & ~0xFFF);
            if (pd[pd_idx] & PAGE_HUGE) {
                split_huge_page(pd, pd_idx);
            }
        }

        u64 pd_entry2 = pd[pd_idx];
        u64* pt;
        if (!(pd_entry2 & PAGE_PRESENT)) {
            pt = alloc_page_table();
            if (!pt) return 0;
            pd[pd_idx] = (u64)pt | KERNEL_PAGE_FLAGS;
        } else {
            pt = (u64*)(pd_entry2 & ~0xFFF);
        }

        pt[pt_idx] = (phys + i * PAGE_SIZE) | flags | PAGE_PRESENT;
    }

    return vaddr;
}

virt_addr_t map_physical_at(virt_addr_t vaddr, phys_addr_t phys, u64 pages, u32 flags) {
    if (!g_pml4 || pages == 0) return 0;

    for (u64 i = 0; i < pages; i++) {
        virt_addr_t va = vaddr + i * PAGE_SIZE;
        u64 pml4_idx = PML4_INDEX(va);
        u64 pdpt_idx = PDP_INDEX(va);
        u64 pd_idx = PD_INDEX(va);
        u64 pt_idx = PT_INDEX(va);

        u64* pdpt = g_pdpt;

        u64 pd_entry = pdpt[pdpt_idx];
        u64* pd;
        if (!(pd_entry & PAGE_PRESENT)) {
            pd = alloc_page_table();
            if (!pd) return 0;
            pdpt[pdpt_idx] = (u64)pd | KERNEL_PAGE_FLAGS;
        } else {
            pd = (u64*)(pd_entry & ~0xFFF);
            if (pd[pd_idx] & PAGE_HUGE) {
                split_huge_page(pd, pd_idx);
            }
        }

        u64 pd_entry2 = pd[pd_idx];
        u64* pt;
        if (!(pd_entry2 & PAGE_PRESENT)) {
            pt = alloc_page_table();
            if (!pt) return 0;
            pd[pd_idx] = (u64)pt | KERNEL_PAGE_FLAGS;
        } else {
            pt = (u64*)(pd_entry2 & ~0xFFF);
        }

        pt[pt_idx] = (phys + i * PAGE_SIZE) | flags | PAGE_PRESENT;
    }

    return vaddr;
}

void unmap_physical(virt_addr_t addr, u64 pages) {
    free_virtual(addr, pages);
}

virt_addr_t virtual_to_physical(virt_addr_t virt) {
    if (!g_pml4) return 0;

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