#ifndef ASSETS_H
#define ASSETS_H

#include <types.h>

typedef struct {
    void* data;
    u64 size;
    u32 width;
    u32 height;
} Asset;

typedef struct {
    Asset leonelos;
    Asset icon;
    Asset spinner;
} BootAssets;

EFI_STATUS assets_load(EFI_SYSTEM_TABLE* st, EFI_HANDLE image, BootAssets* assets);
EFI_STATUS asset_read_file(EFI_SYSTEM_TABLE* st, EFI_HANDLE image, CHAR16* path, void** data, u64* size);
u32 asset_get_width(void* rgba_data, u64 size);
u32 asset_get_height(void* rgba_data, u64 size);

#define ASSET_LEONELOS_PATH L"\\boot\\assets\\leonelos.rgba"
#define ASSET_ICON_PATH L"\\boot\\assets\\icon.rgba"
#define ASSET_SPINNER_PATH L"\\boot\\assets\\spinner.rgba"

#define LEONELOS_WIDTH 800
#define LEONELOS_HEIGHT 400
#define ICON_SIZE 512
#define SPINNER_SIZE 512

#endif