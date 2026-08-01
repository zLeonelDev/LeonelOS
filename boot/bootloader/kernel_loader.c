#include <efi.h>
#include <efilib.h>
#include <boot.h>

#pragma pack(push, 1)
typedef struct {
    unsigned char e_ident[16];
    u16 e_type;
    u16 e_machine;
    u32 e_version;
    u64 e_entry;
    u64 e_phoff;
    u64 e_shoff;
    u32 e_flags;
    u16 e_ehsize;
    u16 e_phentsize;
    u16 e_phnum;
    u16 e_shentsize;
    u16 e_shnum;
    u16 e_shstrndx;
} Elf64Header;

typedef struct {
    u32 p_type;
    u32 p_flags;
    u64 p_offset;
    u64 p_vaddr;
    u64 p_paddr;
    u64 p_filesz;
    u64 p_memsz;
    u64 p_align;
} Elf64Phdr;
#pragma pack(pop)

#define ELFMAG0 0x7f
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'
#define PT_LOAD 1

EFI_STATUS kernel_load(EFI_SYSTEM_TABLE* st, EFI_HANDLE image, VOID** entry) {
    EFI_STATUS status;
    void* kernel_file = NULL;
    u64 kernel_size = 0;

    status = asset_read_file(st, image, KERNEL_ELF_PATH, &kernel_file, &kernel_size);
    if (EFI_ERROR(status)) {
        Print(L"ERROR: Failed to read kernel.elf\r\n");
        return status;
    }

    Elf64Header* hdr = (Elf64Header*)kernel_file;
    if (kernel_size < sizeof(Elf64Header) ||
        hdr->e_ident[0] != ELFMAG0 || hdr->e_ident[1] != ELFMAG1 ||
        hdr->e_ident[2] != ELFMAG2 || hdr->e_ident[3] != ELFMAG3) {
        Print(L"ERROR: Invalid ELF magic\r\n");
        st->BootServices->FreePool(kernel_file);
        return EFI_UNSUPPORTED;
    }
    if (hdr->e_machine != 0x3E) { /* EM_X86_64 */
        Print(L"ERROR: Not an x86_64 ELF\r\n");
        st->BootServices->FreePool(kernel_file);
        return EFI_UNSUPPORTED;
    }
    if (hdr->e_phoff == 0 || hdr->e_phentsize < sizeof(Elf64Phdr) || hdr->e_phnum == 0) {
        Print(L"ERROR: Bad program headers\r\n");
        st->BootServices->FreePool(kernel_file);
        return EFI_UNSUPPORTED;
    }

    Elf64Phdr* phdrs = (Elf64Phdr*)((u8*)kernel_file + hdr->e_phoff);

    u64 load_start = ~0ull;
    u64 load_end = 0;
    for (int i = 0; i < hdr->e_phnum; i++) {
        Elf64Phdr* p = &phdrs[i];
        if (p->p_type != PT_LOAD || p->p_memsz == 0) continue;
        if (p->p_vaddr < load_start) load_start = p->p_vaddr;
        if (p->p_vaddr + p->p_memsz > load_end) load_end = p->p_vaddr + p->p_memsz;
    }

    if (load_start == ~0ull) {
        Print(L"ERROR: No loadable segments\r\n");
        st->BootServices->FreePool(kernel_file);
        return EFI_UNSUPPORTED;
    }

    /* Reserve the exact physical range the kernel image spans. The file
     * buffer holding the ELF is a pool allocation that may itself occupy
     * this range, so free it first, then read the file again after the
     * range has been reserved. */
    st->BootServices->FreePool(kernel_file);
    kernel_file = NULL;

    EFI_PHYSICAL_ADDRESS phys = load_start;
    UINTN pages = (UINTN)((load_end - load_start + 0xFFF) >> 12);
    status = st->BootServices->AllocatePages(AllocateAddress, EfiLoaderData, pages, &phys);
    if (EFI_ERROR(status) || phys != load_start) {
        Print(L"ERROR: Cannot reserve kernel memory (range 0x%llx..0x%llx) status=0x%llx\r\n",
              (u64)load_start, (u64)load_end, (u64)status);
        return EFI_OUT_OF_RESOURCES;
    }

    status = asset_read_file(st, image, KERNEL_ELF_PATH, &kernel_file, &kernel_size);
    if (EFI_ERROR(status)) {
        Print(L"ERROR: Failed to re-read kernel.elf\r\n");
        return status;
    }
    hdr = (Elf64Header*)kernel_file;
    phdrs = (Elf64Phdr*)((u8*)kernel_file + hdr->e_phoff);

    for (int i = 0; i < hdr->e_phnum; i++) {
        Elf64Phdr* p = &phdrs[i];
        if (p->p_type != PT_LOAD || p->p_memsz == 0) continue;

        u8* dst = (u8*)(UINTN)p->p_vaddr;
        const u8* src = (const u8*)kernel_file + p->p_offset;

        for (u64 k = 0; k < p->p_filesz; k++) dst[k] = src[k];
        for (u64 k = p->p_filesz; k < p->p_memsz; k++) dst[k] = 0;

        Print(L"LOAD: virt=0x%llx size=0x%llx file_size=0x%llx\r\n",
              p->p_vaddr, p->p_memsz, p->p_filesz);
    }

    *entry = (VOID*)(UINTN)hdr->e_entry;

    Print(L"Kernel entry: 0x%llx\r\n", (u64)(UINTN)*entry);

    st->BootServices->FreePool(kernel_file);
    return EFI_SUCCESS;
}
