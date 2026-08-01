#include <efi.h>
#include <efilib.h>
#include <boot.h>

/* Static, pool-free buffer large enough to hold any realistic firmware
 * memory map. Using a static buffer ensures GetMemoryMap sees a stable
 * map between the probe and the real call (anything we AllocatePool
 * in between invalidates the previously captured map_key). */
static EFI_MEMORY_DESCRIPTOR g_memmap_raw[2048];

EFI_STATUS memory_map_get(EFI_SYSTEM_TABLE* st, MemoryMap* mm) {
    if (!st || !st->BootServices || !mm) return EFI_INVALID_PARAMETER;

    UINTN map_size = sizeof(g_memmap_raw);
    UINTN map_key = 0;
    UINTN descriptor_size = 0;
    UINT32 descriptor_version = 0;
    EFI_STATUS status;

    status = st->BootServices->GetMemoryMap(
        &map_size, g_memmap_raw, &map_key, &descriptor_size, &descriptor_version);
    if (status == EFI_BUFFER_TOO_SMALL) {
        Print(L"  Memory: map too large (%llu bytes)\r\n", (u64)map_size);
        return EFI_BUFFER_TOO_SMALL;
    }
    if (EFI_ERROR(status)) {
        Print(L"  Memory: GetMemoryMap status=0x%llx\r\n", (u64)status);
        return status;
    }

    UINTN entry_count = map_size / descriptor_size;

    MemoryMapEntry* entries = NULL;
    status = st->BootServices->AllocatePool(
        EfiLoaderData, entry_count * sizeof(MemoryMapEntry), (VOID**)&entries);
    if (EFI_ERROR(status)) return status;

    EFI_MEMORY_DESCRIPTOR* d = g_memmap_raw;
    u64 total = 0;
    u64 usable = 0;
    for (UINTN i = 0; i < entry_count; i++) {
        u64 bytes = d->NumberOfPages * EFI_PAGE_SIZE;
        total += bytes;
        entries[i].base_addr = (u64)d->PhysicalStart;
        entries[i].length = bytes;
        entries[i].type = (u32)d->Type;
        entries[i].attr = (u32)d->Attribute;
        if (d->Type == EfiConventionalMemory) usable += bytes;
        d = (EFI_MEMORY_DESCRIPTOR*)((u8*)d + descriptor_size);
    }

    mm->entries = entries;
    mm->count = entry_count;
    mm->map_key = map_key;
    mm->total_memory = total;
    mm->usable_memory = usable;

    return EFI_SUCCESS;
}
