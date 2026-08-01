#include <efi.h>
#include <efilib.h>
#include <boot.h>
#include <graphics.h>

EFI_STATUS gop_init(EFI_SYSTEM_TABLE* st, FramebufferInfo* fb) {
    EFI_STATUS status;
    EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop = NULL;

    status = st->BootServices->LocateProtocol(&gop_guid, NULL, (VOID**)&gop);
    if (EFI_ERROR(status)) return status;
    if (!gop || !gop->Mode || !gop->Mode->Info) return EFI_UNSUPPORTED;

    /* Pick a mode that fits a typical display rather than blindly maxing out
     * the GOP framebuffer. We prefer an exact match to a target resolution,
     * then the closest match whose area is >= 1024x768, then any mode that is
     * at least 1024x768, finally the largest available mode. */
    const UINTN target_w = 1024;
    const UINTN target_h = 768;
    const UINTN min_area = 1024 * 768;

    UINTN best_mode = 0;
    INTN best_score = -1;
    UINT32 mode;
    for (mode = 0; mode < gop->Mode->MaxMode; mode++) {
        EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* info = NULL;
        UINTN info_size = 0;
        status = gop->QueryMode(gop, mode, &info_size, &info);
        if (EFI_ERROR(status) || !info) continue;
        UINTN w = info->HorizontalResolution;
        UINTN h = info->VerticalResolution;
        UINTN area = w * h;

        INTN score = 0;
        if (w == target_w && h == target_h) {
            score = 4000;  /* exact match wins */
        } else if (area >= min_area) {
            /* close enough to target, prefer smaller diff */
            INTN dw = (INTN)w - (INTN)target_w;
            INTN dh = (INTN)h - (INTN)target_h;
            score = 2000 - (dw * dw + dh * dh);
        } else {
            /* below target — rank by area so we pick the largest usable */
            score = (INTN)area / 100;
        }

        if (score > best_score) {
            best_score = score;
            best_mode = mode;
        }
    }

    status = gop->SetMode(gop, (UINT32)best_mode);
    if (EFI_ERROR(status)) return status;

    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* info = gop->Mode->Info;
    fb->address = (VOID*)(UINTN)gop->Mode->FrameBufferBase;
    fb->width = info->HorizontalResolution;
    fb->height = info->VerticalResolution;
    fb->pitch = info->PixelsPerScanLine * 4;
    fb->bpp = 32;
    fb->pixel_format = info->PixelFormat;

    switch (info->PixelFormat) {
        case PixelRedGreenBlueReserved8BitPerColor:
            fb->red_mask = 0x00FF0000;
            fb->green_mask = 0x0000FF00;
            fb->blue_mask = 0x000000FF;
            fb->alpha_mask = 0xFF000000;
            fb->pixel_format = (u32)PixelRedGreenBlueReserved8BitPerColor;
            break;
        case PixelBlueGreenRedReserved8BitPerColor:
            fb->red_mask = 0x000000FF;
            fb->green_mask = 0x0000FF00;
            fb->blue_mask = 0x00FF0000;
            fb->alpha_mask = 0xFF000000;
            fb->pixel_format = (u32)PixelBlueGreenRedReserved8BitPerColor;
            break;
        default:
            fb->red_mask = info->PixelInformation.RedMask;
            fb->green_mask = info->PixelInformation.GreenMask;
            fb->blue_mask = info->PixelInformation.BlueMask;
            fb->alpha_mask = info->PixelInformation.ReservedMask;
            fb->pixel_format = 0;
            break;
    }

    return EFI_SUCCESS;
}

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE* system_table) {
    efi_lib_init(image_handle, system_table);

    EFI_STATUS status = system_table->BootServices->SetWatchdogTimer(0, 0, 0, NULL);
    if (EFI_ERROR(status)) return status;

    Print(L"LeonelOS Bootloader 0.1.0\r\n");

    FramebufferInfo fb_info;
    status = gop_init(system_table, &fb_info);
    if (EFI_ERROR(status)) {
        Print(L"ERROR: graphics init failed\r\n");
        return status;
    }
    Print(L"Framebuffer: %ux%u pitch=%u bpp=%u fmt=%u\r\n",
          fb_info.width, fb_info.height, fb_info.pitch, fb_info.bpp, fb_info.pixel_format);
    Print(L"  masks: R=%x G=%x B=%x A=%x\r\n",
          fb_info.red_mask, fb_info.green_mask, fb_info.blue_mask, fb_info.alpha_mask);

    VOID* kernel_entry = NULL;
    status = kernel_load(system_table, image_handle, &kernel_entry);
    if (EFI_ERROR(status)) {
        Print(L"ERROR: kernel load failed\r\n");
        return status;
    }

    /* Load assets after the kernel so pool allocations cannot land inside
     * the kernel's reserved address range. */
    BootAssets assets;
    status = assets_load(system_table, image_handle, &assets);
    if (EFI_ERROR(status)) {
        Print(L"ERROR: asset load failed\r\n");
        return status;
    }
    Print(L"Assets: logo=%ux%u icon=%ux%u spinner=%ux%u\r\n",
          assets.leonelos.width, assets.leonelos.height,
          assets.icon.width, assets.icon.height,
          assets.spinner.width, assets.spinner.height);

    MemoryMap mem_map;
    status = memory_map_get(system_table, &mem_map);
    if (EFI_ERROR(status)) {
        Print(L"ERROR: memory map failed\r\n");
        return status;
    }
    Print(L"Memory: %llu entries, %llu MB total, %llu MB usable\r\n",
          mem_map.count, mem_map.total_memory / 1048576ull, mem_map.usable_memory / 1048576ull);

    /* Draw the splash. All Print() output must happen before this: the
     * firmware's text console writes into the same framebuffer. */
    graphics_clear(&fb_info, 0x00000000);
    bootui_draw(&fb_info, &assets);

    system_table->BootServices->Stall(50000);

    status = system_table->BootServices->ExitBootServices(image_handle, (UINTN)mem_map.map_key);
    if (status == EFI_INVALID_PARAMETER) {
        /* The memory map key may have advanced since we captured it; refresh
         * it and retry before giving up. */
        UINTN msz = 0, mkey = 0, dsz = 0;
        UINT32 dver = 0;
        system_table->BootServices->GetMemoryMap(&msz, NULL, &mkey, &dsz, &dver);
        status = system_table->BootServices->ExitBootServices(image_handle, (UINTN)mkey);
    }
    if (EFI_ERROR(status)) {
        Print(L"ERROR: ExitBootServices status=0x%llx\r\n", (u64)status);
        return status;
    }

    void (*kernel_main)(FramebufferInfo*, BootAssets*, MemoryMap*) =
        (void (*)(FramebufferInfo*, BootAssets*, MemoryMap*))kernel_entry;
    kernel_main(&fb_info, &assets, &mem_map);

    return EFI_SUCCESS;
}
