#include <efi.h>
#include <efilib.h>
#include <boot.h>

#define ASSET_LEONELOS_PATH L"\\boot\\assets\\leonelos.rgba"
#define ASSET_ICON_PATH     L"\\boot\\assets\\icon.rgba"
#define ASSET_SPINNER_PATH  L"\\boot\\assets\\spinner.rgba"

EFI_STATUS asset_read_file(EFI_SYSTEM_TABLE* st, EFI_HANDLE image, CHAR16* path,
                           void** buffer, u64* size) {
    EFI_STATUS status;
    EFI_FILE_PROTOCOL* root = NULL;
    EFI_FILE_PROTOCOL* file = NULL;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs = NULL;
    EFI_GUID sfsp_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;

    if (!st || !st->BootServices || !buffer || !size) return EFI_INVALID_PARAMETER;
    *buffer = NULL;
    *size = 0;

    /* The image's device handle is the CD/disk block device, not the FAT
     * volume, so the file system cannot be resolved from the image handle
     * directly. Make sure the device's drivers are connected, then enumerate
     * every handle that provides the simple file system protocol and try to
     * open the requested path on each. */
    EFI_GUID loaded_image_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    EFI_LOADED_IMAGE_PROTOCOL* loaded_image = NULL;
    status = st->BootServices->HandleProtocol(image, &loaded_image_guid, (VOID**)&loaded_image);
    if (EFI_ERROR(status) || !loaded_image) {
        Print(L"  SFSP: loaded image status=0x%llx\r\n", (u64)status);
        return EFI_UNSUPPORTED;
    }
    if (loaded_image->DeviceHandle) {
        st->BootServices->ConnectController(loaded_image->DeviceHandle, NULL, NULL, TRUE);
    }

    UINTN handle_count = 0;
    EFI_HANDLE* handles = NULL;
    status = st->BootServices->LocateHandleBuffer(ByProtocol, &sfsp_guid, NULL, &handle_count, &handles);
    if (EFI_ERROR(status) || handle_count == 0) {
        Print(L"  SFSP: enumerate status=0x%llx count=%llu\r\n", (u64)status, (u64)handle_count);
        return EFI_NOT_FOUND;
    }

    for (UINTN i = 0; i < handle_count; i++) {
        fs = NULL;
        status = st->BootServices->HandleProtocol(handles[i], &sfsp_guid, (VOID**)&fs);
        if (EFI_ERROR(status) || !fs) continue;

        EFI_FILE_PROTOCOL* root_i = NULL;
        status = fs->OpenVolume(fs, &root_i);
        if (EFI_ERROR(status)) continue;

        EFI_FILE_PROTOCOL* file_i = NULL;
        status = root_i->Open(root_i, &file_i, path, EFI_FILE_MODE_READ, 0);
        if (EFI_ERROR(status)) {
            root_i->Close(root_i);
            continue;
        }

        root = root_i;
        file = file_i;
        break;
    }

    st->BootServices->FreePool(handles);

    if (!file) {
        Print(L"  SFSP: open '%s' not found on any volume\r\n", path);
        return EFI_NOT_FOUND;
    }

    EFI_GUID file_info_guid = EFI_FILE_INFO_GUID;
    EFI_FILE_INFO* info = NULL;
    UINTN info_size = 0;

    status = file->GetInfo(file, &file_info_guid, &info_size, NULL);
    if (status == EFI_BUFFER_TOO_SMALL) {
        status = st->BootServices->AllocatePool(EfiLoaderData, info_size, (VOID**)&info);
        if (EFI_ERROR(status)) {
            Print(L"  SFSP: pool status=0x%llx\r\n", (u64)status);
            file->Close(file);
            root->Close(root);
            return status;
        }
        status = file->GetInfo(file, &file_info_guid, &info_size, info);
        if (EFI_ERROR(status)) {
            Print(L"  SFSP: getinfo2 status=0x%llx\r\n", (u64)status);
            st->BootServices->FreePool(info);
            file->Close(file);
            root->Close(root);
            return status;
        }
    } else if (EFI_ERROR(status)) {
        Print(L"  SFSP: getinfo1 status=0x%llx\r\n", (u64)status);
        file->Close(file);
        root->Close(root);
        return status;
    }

    if (info && info->FileSize > 0) {
        *size = (u64)info->FileSize;
        Print(L"  SFSP: '%s' filesize=%llu\r\n", path, *size);
        status = st->BootServices->AllocatePool(EfiLoaderData, (UINTN)*size, buffer);
        if (EFI_ERROR(status)) {
            st->BootServices->FreePool(info);
            file->Close(file);
            root->Close(root);
            return status;
        }

        UINTN bytes_read = 0;
        u64 total_read = 0;
        while (total_read < *size) {
            bytes_read = (UINTN)(*size - total_read);
            status = file->Read(file, &bytes_read, (u8*)*buffer + total_read);
            if (EFI_ERROR(status)) break;
            if (bytes_read == 0) break;
            total_read += bytes_read;
        }
        if (EFI_ERROR(status) || total_read != *size) {
            st->BootServices->FreePool(*buffer);
            *buffer = NULL;
            *size = 0;
            st->BootServices->FreePool(info);
            file->Close(file);
            root->Close(root);
            return EFI_LOAD_ERROR;
        }
    }

    if (info) st->BootServices->FreePool(info);
    file->Close(file);
    root->Close(root);
    return EFI_SUCCESS;
}

EFI_STATUS assets_load(EFI_SYSTEM_TABLE* st, EFI_HANDLE image, BootAssets* assets) {
    if (!st || !assets) return EFI_INVALID_PARAMETER;

    assets->leonelos.data = NULL;
    assets->leonelos.size = 0;
    assets->leonelos.width = LEONELOS_WIDTH;
    assets->leonelos.height = LEONELOS_HEIGHT;

    assets->icon.data = NULL;
    assets->icon.size = 0;
    assets->icon.width = ICON_WIDTH;
    assets->icon.height = ICON_HEIGHT;

    assets->spinner.data = NULL;
    assets->spinner.size = 0;
    assets->spinner.width = SPINNER_SIZE;
    assets->spinner.height = SPINNER_SIZE;

    EFI_STATUS status;

    status = asset_read_file(st, image, ASSET_LEONELOS_PATH,
                             &assets->leonelos.data, &assets->leonelos.size);
    if (EFI_ERROR(status) || assets->leonelos.size != (u64)LEONELOS_WIDTH * LEONELOS_HEIGHT * 4) {
        assets->leonelos.data = NULL;
        assets->leonelos.size = 0;
    }

    status = asset_read_file(st, image, ASSET_ICON_PATH,
                             &assets->icon.data, &assets->icon.size);
    if (EFI_ERROR(status) || assets->icon.size != (u64)ICON_WIDTH * ICON_HEIGHT * 4) {
        assets->icon.data = NULL;
        assets->icon.size = 0;
    }

    status = asset_read_file(st, image, ASSET_SPINNER_PATH,
                             &assets->spinner.data, &assets->spinner.size);
    if (EFI_ERROR(status) || assets->spinner.size != (u64)SPINNER_SIZE * SPINNER_SIZE * 4) {
        assets->spinner.data = NULL;
        assets->spinner.size = 0;
    }

    return EFI_SUCCESS;
}
