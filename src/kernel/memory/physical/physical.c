#include <physical.h>
#include <types.h>
#include <debug.h>
#include <arch/x86_64/bootinfo.h>

#define MAX_PHYSICAL_MEMORY (1024 * MEGABYTE)
#define BITMAP_SIZE (MAX_PHYSICAL_MEMORY / PAGE_SIZE / 8)

static u8* g_bitmap = NULL;
static u64 g_total_pages = 0;
static u64 g_free_pages = 0;
static u64 g_allocated_pages = 0;

void init_physical_memory(MemoryMap* memory_map) {
    if (!memory_map) return;
    
    u64 bitmap_pages = BITMAP_SIZE / PAGE_SIZE + 1;
    g_bitmap = (u8*)KERNEL_HEAP_START;
    for (u64 i = 0; i < BITMAP_SIZE; i++) {
        g_bitmap[i] = 0xFF;
    }
    
    g_total_pages = 0;
    g_free_pages = 0;
    
    for (u64 i = 0; i < memory_map->count; i++) {
        MemoryMapEntry* entry = &memory_map->entries[i];
        if (entry->type == MEMORY_CONVENTIONAL ||
            entry->type == MEMORY_LOADER_DATA) {
            
            u64 base = entry->base_addr;
            u64 size = entry->length;
            
            for (u64 page = base; page < base + size; page += PAGE_SIZE) {
                u64 page_idx = page / PAGE_SIZE;
                if (page_idx < BITMAP_SIZE * 8) {
                    g_bitmap[page_idx / 8] &= ~(1 << (page_idx % 8));
                    g_free_pages++;
                }
                g_total_pages++;
            }
        }
    }
    
    for (u64 i = 0; i < bitmap_pages; i++) {
        u64 page_idx = ((u64)g_bitmap / PAGE_SIZE) + i;
        if (page_idx < BITMAP_SIZE * 8) {
            g_bitmap[page_idx / 8] |= (1 << (page_idx % 8));
            g_free_pages--;
        }
    }
    
    g_allocated_pages = 0;
    
    debug_log("Physical memory: %u pages total, %u pages free\n",
              (unsigned int)g_total_pages, (unsigned int)g_free_pages);
}

phys_addr_t allocate_physical(u64 pages) {
    if (pages == 0) return 0;
    
    phys_addr_t addr = 0;
    u64 found = 0;
    
    for (u64 i = 0; i < BITMAP_SIZE * 8; i++) {
        if (!(g_bitmap[i / 8] & (1 << (i % 8)))) {
            if (found == 0) {
                addr = i * PAGE_SIZE;
            }
            found++;
            
            if (found == pages) {
                u64 base_idx = addr / PAGE_SIZE;
                for (u64 p = 0; p < pages; p++) {
                    u64 idx = base_idx + p;
                    g_bitmap[idx / 8] |= (1 << (idx % 8));
                }
                g_free_pages -= pages;
                g_allocated_pages += pages;
                return addr;
            }
        } else {
            found = 0;
            addr = 0;
        }
    }
    
    return 0;
}

void free_physical(phys_addr_t addr, u64 pages) {
    if (addr == 0 || pages == 0) return;
    
    u64 base_idx = addr / PAGE_SIZE;
    for (u64 p = 0; p < pages; p++) {
        u64 idx = base_idx + p;
        g_bitmap[idx / 8] &= ~(1 << (idx % 8));
    }
    
    g_free_pages += pages;
    g_allocated_pages -= pages;
}

phys_addr_t allocate_contiguous(u64 pages) {
    return allocate_physical(pages);
}

void free_contiguous(phys_addr_t addr, u64 pages) {
    free_physical(addr, pages);
}

u64 get_free_memory() {
    return g_free_pages * PAGE_SIZE;
}

u64 get_total_memory() {
    return g_total_pages * PAGE_SIZE;
}