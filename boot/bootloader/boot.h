#ifndef BOOT_H
#define BOOT_H

#include <types.h>
#include <efi.h>
#include <efilib.h>

/*
 * Boot protocol ABI.
 * These structures must match src/kernel/arch/x86_64/bootinfo.h exactly:
 * the bootloader fills them and passes pointers to kernel_main.
 */

typedef struct {
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

typedef struct {
    void* data;
    u64 size;
    u32 width;
    u32 height;
} BootAsset;

typedef struct {
    BootAsset leonelos;
    BootAsset icon;
    BootAsset spinner;
} BootAssets;

typedef struct {
    u64 base_addr;
    u64 length;
    u32 type;
    u32 attr;
} MemoryMapEntry;

typedef struct {
    MemoryMapEntry* entries;
    u64 count;
    u64 map_key;
    u64 total_memory;
    u64 usable_memory;
} MemoryMap;

/* Raw headerless RGBA asset dimensions (must match boot/assets/.rgba files). */
#define LEONELOS_WIDTH  638
#define LEONELOS_HEIGHT 126
#define ICON_WIDTH      408
#define ICON_HEIGHT     440
#define SPINNER_SIZE    512

/* Boot screen layout (mirrors src/kernel/graphics/bootui/bootui.c). */
#define LOGO_TOP_OFFSET    330
#define ICON_TOP_OFFSET    180
#define SPINNER_TOP_OFFSET 20
#define ICON_SCALE         0.4f

#define KERNEL_ELF_PATH L"\\kernel.elf"

EFI_STATUS gop_init(EFI_SYSTEM_TABLE* st, FramebufferInfo* fb);
EFI_STATUS asset_read_file(EFI_SYSTEM_TABLE* st, EFI_HANDLE image, CHAR16* path, void** data, u64* size);
EFI_STATUS assets_load(EFI_SYSTEM_TABLE* st, EFI_HANDLE image, BootAssets* assets);
EFI_STATUS memory_map_get(EFI_SYSTEM_TABLE* st, MemoryMap* mm);
EFI_STATUS kernel_load(EFI_SYSTEM_TABLE* st, EFI_HANDLE image, VOID** entry);
VOID bootui_draw(FramebufferInfo* fb, BootAssets* assets);

#endif
