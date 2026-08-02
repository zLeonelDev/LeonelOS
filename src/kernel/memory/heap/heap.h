#ifndef LEONELOS_HEAP_H
#define LEONELOS_HEAP_H

#include <types.h>

#ifdef __cplusplus
extern "C" {
#endif

void init_heap();
void* kmalloc(usize size);
void* kmalloc_aligned(usize size, usize alignment);
void kfree(void* ptr);
void* krealloc(void* ptr, usize new_size);
void* kcalloc(usize count, usize size);
usize kmalloc_usable_size(void* ptr);

#define KMALLOC_MAX SIZE_MAX

#ifdef __cplusplus
}
#endif

#endif