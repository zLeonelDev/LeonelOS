#include <heap.h>
#include <types.h>
#include <debug.h>
#include <physical.h>

#define ALIGNMENT 16
#define INITIAL_PAGES 64

typedef struct Header {
    usize size;
    bool used;
    struct Header* next;
    struct Header* prev;
} Header;

#define HEADER_SIZE sizeof(Header)

static u8* g_brk = NULL;
static u8* g_brk_end = NULL;

static u8* heap_alloc_pages(usize pages) {
    phys_addr_t phys = allocate_physical(pages);
    if (phys == PHYS_ALLOC_FAILED) return NULL;
    return (u8*)(phys_addr_t)phys;
}

void init_heap() {
    g_brk = NULL;
    g_brk_end = NULL;

    debug_log("Heap initialized (physical allocator backed)\n");
}

void* kmalloc(usize size) {
    if (size == 0) return NULL;

    usize aligned_size = ALIGN_UP(size, ALIGNMENT);
    usize total_size = aligned_size + HEADER_SIZE;

    if (g_brk == NULL || g_brk + total_size > g_brk_end) {
        usize pages_needed = ALIGN_UP(total_size, PAGE_SIZE) / PAGE_SIZE;
        if (pages_needed < INITIAL_PAGES) pages_needed = INITIAL_PAGES;

        u8* new_pages = heap_alloc_pages(pages_needed);
        if (!new_pages) return NULL;

        if (g_brk == NULL) {
            g_brk = new_pages;
            g_brk_end = new_pages + pages_needed * PAGE_SIZE;
        } else {
            g_brk_end = new_pages;
        }
    }

    Header* block = (Header*)g_brk;
    block->size = aligned_size;
    block->used = true;
    block->next = NULL;
    block->prev = NULL;

    g_brk += total_size;
    return (void*)((usize)block + HEADER_SIZE);
}

void* kmalloc_aligned(usize size, usize alignment) {
    UNUSED(alignment);
    return kmalloc(size);
}

void kfree(void* ptr) {
    if (!ptr) return;

    Header* block = (Header*)((usize)ptr - HEADER_SIZE);
    block->used = false;

    if (block->next && !block->next->used) {
        block->size += HEADER_SIZE + block->next->size;
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
    }

    if (block->prev && !block->prev->used) {
        block->prev->size += HEADER_SIZE + block->size;
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }
    }
}

void* krealloc(void* ptr, usize new_size) {
    if (!ptr) return kmalloc(new_size);
    if (new_size == 0) { kfree(ptr); return NULL; }

    Header* block = (Header*)((usize)ptr - HEADER_SIZE);
    if (block->size >= new_size) return ptr;

    void* new_ptr = kmalloc(new_size);
    if (!new_ptr) return NULL;

    usize copy_size = block->size < new_size ? block->size : new_size;
    for (usize i = 0; i < copy_size; i++) {
        ((u8*)new_ptr)[i] = ((u8*)ptr)[i];
    }

    kfree(ptr);
    return new_ptr;
}

void* kcalloc(usize count, usize size) {
    usize total = count * size;
    void* ptr = kmalloc(total);
    if (ptr) {
        for (usize i = 0; i < total; i++) {
            ((u8*)ptr)[i] = 0;
        }
    }
    return ptr;
}

usize kmalloc_usable_size(void* ptr) {
    if (!ptr) return 0;
    Header* block = (Header*)((usize)ptr - HEADER_SIZE);
    return block->size;
}