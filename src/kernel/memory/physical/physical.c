#include <physical.h>
#include <types.h>
#include <debug.h>
#include <arch/x86_64/bootinfo.h>

/*
 * Physical memory manager.
 *
 * Tracks which 4 KiB pages are free using a single bitmap, one bit per page.
 * The bitmap itself lives in a static BSS buffer so we don't need any
 * dynamic memory to bootstrap it; that also means the bitmap's address is
 * simply its link-time virtual (= physical, since we still run identity-
 * mapped) address.
 *
 * Capacity: 16 KiB of bitmap covers 16 KiB * 8 bits/page * 4 KiB/page
 * = 512 MiB of RAM. Plenty for typical early-boot heaps; raise this if
 * we ever need to track >512 MiB.
 */
#define MAX_PAGES    (16 * 1024 * 8)
#define BITMAP_BYTES (MAX_PAGES / 8)

static u8 g_bitmap[BITMAP_BYTES];
static u8 g_bitmap_ready = 0;
static u64 g_total_pages = 0;
static u64 g_free_pages = 0;
static u64 g_allocated_pages = 0;

static inline u64 page_index(phys_addr_t addr) {
    return addr / PAGE_SIZE;
}

static inline void set_bit(u64 idx) {
    g_bitmap[idx / 8] |= (u8)(1u << (idx % 8));
}

static inline void clear_bit(u64 idx) {
    g_bitmap[idx / 8] &= (u8)~(1u << (idx % 8));
}

static inline u8 test_bit(u64 idx) {
    return (u8)((g_bitmap[idx / 8] >> (idx % 8)) & 1u);
}

void init_physical_memory(MemoryMap* memory_map) {
    if (!memory_map) {
        debug_log("PMM: no memory map\n");
        return;
    }

    /* Mark all pages used; we flip conventional ranges to free below.
     * Bitmap semantic: 1 = used, 0 = free. */
    for (u64 i = 0; i < BITMAP_BYTES; i++) g_bitmap[i] = 0xFF;

    g_total_pages = 0;
    g_free_pages = 0;
    g_allocated_pages = 0;

    u64 usable_bytes = 0;
    u64 outside = 0;
    for (u64 i = 0; i < memory_map->count; i++) {
        MemoryMapEntry* e = &memory_map->entries[i];
        if (e->type != MEMORY_CONVENTIONAL) continue;
        if (e->base_addr + e->length <= e->base_addr) continue; /* overflow guard */
        if (e->length == 0) continue;

        u64 base = e->base_addr;
        u64 end  = base + e->length;
        for (u64 page = base; page < end; page += PAGE_SIZE) {
            u64 idx = page_index(page);
            if (idx >= MAX_PAGES) {
                outside++;
                continue;
            }
            clear_bit(idx);
            g_free_pages++;
            g_total_pages++;
        }
        usable_bytes += e->length;
    }

    /* Reserve page 0: the real-mode IVT/BDA live there and a null-pointer
     * dereference must never silently succeed. OVMF reports this region as
     * Conventional, so we have to take it back. */
    if (!test_bit(0)) {
        set_bit(0);
        g_free_pages--;
    }

    g_bitmap_ready = 1;

    /* Reserve the area where the kernel image lives. The bootloader told us
     * the entry point and the loader regions are not part of the memory
     * map's "Conventional" classification, so without explicitly booking
     * these pages we would hand them out and the kernel would corrupt
     * itself. We compute it from the kernel's known link range. */
    extern char kernel_start[];
    extern char kernel_end[];
    phys_addr_t kr_start = (phys_addr_t)kernel_start;
    phys_addr_t kr_end   = (phys_addr_t)kernel_end;
    u64 kr_start_idx = kr_start / PAGE_SIZE;
    u64 kr_end_idx   = (kr_end + PAGE_SIZE - 1) / PAGE_SIZE;
    for (u64 i = kr_start_idx; i < kr_end_idx && i < MAX_PAGES; i++) {
        if (!test_bit(i)) {
            g_free_pages--;
        }
        set_bit(i);
    }

    debug_log("PMM: %u pages total, %u pages free, %u MB usable\n",
              (unsigned int)g_total_pages, (unsigned int)g_free_pages,
              (unsigned int)(usable_bytes / MEGABYTE));
    if (outside > 0) {
        debug_log("PMM: %u pages outside bitmap range (>%u MiB)\n",
                  (unsigned int)outside,
                  (unsigned int)((u64)MAX_PAGES * PAGE_SIZE / MEGABYTE));
    }

    /* Diagnostic: dump the first 4 free regions so we can sanity-check that
     * the Conventional ranges the bootloader gave us line up with the
     * bitmap layout. */
    u64 run_start = 0;
    u8 in_run = 0;
    u64 dash = 0;
    u64 max_dump = 4;
    debug_log("PMM: free regions (up to %lu):\n", max_dump);
    for (u64 i = 0; i < MAX_PAGES && dash < max_dump; i++) {
        if (!test_bit(i)) {
            if (!in_run) {
                run_start = i;
                in_run = 1;
            }
        } else if (in_run) {
            debug_log("PMM:   [%lx..%lx) %lu pages\n",
                      (unsigned long)run_start * PAGE_SIZE,
                      (unsigned long)i * PAGE_SIZE,
                      (unsigned long)(i - run_start));
            in_run = 0;
            dash++;
        }
    }
    if (in_run && dash < max_dump) {
        debug_log("PMM:   [%lx..%lx) %lu pages\n",
                  (unsigned long)run_start * PAGE_SIZE,
                  (unsigned long)MAX_PAGES * PAGE_SIZE,
                  (unsigned long)(MAX_PAGES - run_start));
    }
}

phys_addr_t allocate_physical(u64 pages) {
    if (pages == 0 || !g_bitmap_ready) return PHYS_ALLOC_FAILED;

    /* First-fit: walk the bitmap until we find `pages` consecutive free
     * bits, then claim them. */
    u64 run_start = 0;
    u64 run_len = 0;
    for (u64 i = 0; i < MAX_PAGES; i++) {
        if (test_bit(i)) {
            /* busy: reset */
            run_len = 0;
            continue;
        }
        if (run_len == 0) run_start = i;
        run_len++;
        if (run_len == pages) {
            for (u64 k = 0; k < pages; k++) set_bit(run_start + k);
            g_free_pages -= pages;
            g_allocated_pages += pages;
            return (phys_addr_t)run_start * PAGE_SIZE;
        }
    }
    return PHYS_ALLOC_FAILED;
}

void free_physical(phys_addr_t addr, u64 pages) {
    if (addr == 0 || pages == 0 || !g_bitmap_ready) return;
    u64 base_idx = page_index(addr);
    if (base_idx >= MAX_PAGES) return;
    if (base_idx + pages > MAX_PAGES) pages = MAX_PAGES - base_idx;
    for (u64 i = 0; i < pages; i++) clear_bit(base_idx + i);
    g_free_pages += pages;
    if (g_allocated_pages >= pages) g_allocated_pages -= pages;
}

phys_addr_t allocate_contiguous(u64 pages) {
    return allocate_physical(pages);
}

void free_contiguous(phys_addr_t addr, u64 pages) {
    free_physical(addr, pages);
}

u64 get_free_memory(void) { return g_free_pages * PAGE_SIZE; }
u64 get_total_memory(void) { return g_total_pages * PAGE_SIZE; }
u64 get_used_memory(void) { return g_allocated_pages * PAGE_SIZE; }
