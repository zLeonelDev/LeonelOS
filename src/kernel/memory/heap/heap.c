#include <heap.h>
#include <types.h>
#include <debug.h>
#include <physical.h>

#define HEAP_START KERNEL_HEAP_START
#define HEAP_END   (KERNEL_HEAP_START + KERNEL_HEAP_SIZE)

typedef struct Header {
    usize size;
    bool used;
    struct Header* next;
    struct Header* prev;
} Header;

#define HEADER_SIZE sizeof(Header)
#define ALIGNMENT 16

static Header* g_heap_start = NULL;
static usize g_heap_size = 0;

static Header* g_head = NULL;

static void* heap_break = NULL;
static void* heap_start = NULL;

static Header* find_free_block(usize size) {
    Header* current = g_head;
    while (current) {
        if (!current->used && current->size >= size) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

static Header* request_space(Header* last, usize size) {
    usize total_size = HEADER_SIZE + size;
    total_size = ALIGN_UP(total_size, ALIGNMENT);
    
    if (!heap_break) {
        heap_start = (void*)HEAP_START;
        heap_break = heap_start;
    }
    
    void* new_break = (void*)((usize)heap_break + total_size);
    if ((usize)new_break > HEAP_END) {
        return NULL;
    }
    
    Header* block = (Header*)heap_break;
    block->size = size;
    block->used = true;
    block->next = NULL;
    block->prev = last;
    
    if (last) {
        last->next = block;
    } else {
        g_head = block;
    }
    
    heap_break = new_break;
    
    return block;
}

void init_heap() {
    g_head = NULL;
    heap_start = NULL;
    heap_break = NULL;
    g_heap_start = (Header*)HEAP_START;
    g_heap_size = KERNEL_HEAP_SIZE;
    
    debug_log("Heap initialized at 0x%llx (size: %llu MB)\n",
              HEAP_START, KERNEL_HEAP_SIZE / MEGABYTE);
}

void* kmalloc(usize size) {
    if (size == 0) return NULL;
    
    usize aligned_size = ALIGN_UP(size, ALIGNMENT);
    
    Header* block = find_free_block(aligned_size);
    if (block) {
        block->used = true;
        return (void*)((usize)block + HEADER_SIZE);
    }
    
    Header* last = NULL;
    Header* current = g_head;
    while (current) {
        last = current;
        current = current->next;
    }
    
    block = request_space(last, aligned_size);
    if (!block) return NULL;
    
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