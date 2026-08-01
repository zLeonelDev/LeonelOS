#ifndef LEONELOS_BOOTINFO_H
#define LEONELOS_BOOTINFO_H

#include <types.h>

/*
 * Boot protocol ABI shared between the UEFI bootloader and the kernel.
 * The bootloader fills these structures and passes pointers to kernel_main.
 * Field layout must match boot/bootloader/boot.h exactly.
 */

typedef struct FramebufferInfo {
    void* address;
    u32 width;
    u32 height;
    u32 pitch;
    u32 bpp;
    u32 pixel_format;
    u32 red_mask;
    u32 green_mask;
    u32 blue_mask;
    u32 alpha_mask;
} FramebufferInfo;

typedef struct BootAsset {
    void* data;
    u64 size;
    u32 width;
    u32 height;
} BootAsset;

typedef struct BootAssets {
    BootAsset leonelos;
    BootAsset icon;
    BootAsset spinner;
} BootAssets;

typedef enum {
    MEMORY_RESERVED = 0,
    MEMORY_LOADER_CODE = 1,
    MEMORY_LOADER_DATA = 2,
    MEMORY_BOOTLOADER_CODE = 3,
    MEMORY_BOOTLOADER_DATA = 4,
    MEMORY_RUNTIME_CODE = 5,
    MEMORY_RUNTIME_DATA = 6,
    MEMORY_CONVENTIONAL = 7,
    MEMORY_UNUSABLE = 8,
    MEMORY_ACPI_RECLAIM = 9,
    MEMORY_ACPI_NVS = 10,
    MEMORY_MMIO = 11,
    MEMORY_MMIO_PORT = 12,
    MEMORY_PAL_CODE = 13,
    MEMORY_PERSISTENT = 14,
} MemoryType;

typedef struct MemoryMapEntry {
    u64 base_addr;
    u64 length;
    u32 type;
    u32 attr;
} MemoryMapEntry;

typedef struct MemoryMap {
    MemoryMapEntry* entries;
    u64 count;
    u64 map_key;
    u64 total_memory;
    u64 usable_memory;
} MemoryMap;

#ifdef __cplusplus
extern "C" {
#endif
void kernel_main(FramebufferInfo* framebuffer, BootAssets* assets, MemoryMap* memory_map);
#ifdef __cplusplus
}
#endif

#endif
