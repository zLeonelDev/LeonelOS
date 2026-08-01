#ifndef LEONELOS_PHYSICAL_MEMORY_H
#define LEONELOS_PHYSICAL_MEMORY_H

#include <types.h>
#include <arch/x86_64/bootinfo.h>

void init_physical_memory(MemoryMap* memory_map);
phys_addr_t allocate_physical(u64 pages);
void free_physical(phys_addr_t addr, u64 pages);
phys_addr_t allocate_contiguous(u64 pages);
void free_contiguous(phys_addr_t addr, u64 pages);
u64 get_free_memory();
u64 get_total_memory();

#endif