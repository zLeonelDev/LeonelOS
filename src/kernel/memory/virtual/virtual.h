#ifndef LEONELOS_VIRTUAL_MEMORY_H
#define LEONELOS_VIRTUAL_MEMORY_H

#include <types.h>

void init_virtual_memory();
void init_virtual_memory_kernel();
virt_addr_t allocate_virtual(u64 pages);
void free_virtual(virt_addr_t addr, u64 pages);
virt_addr_t map_physical(phys_addr_t phys, u64 pages, u32 flags);
void unmap_physical(virt_addr_t addr, u64 pages);
virt_addr_t virtual_to_physical(virt_addr_t virt);

#endif