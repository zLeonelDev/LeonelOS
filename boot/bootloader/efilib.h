#ifndef LEONELOS_EFILIB_H
#define LEONELOS_EFILIB_H

#include <efi.h>
#include <stdarg.h>

void efi_lib_init(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE* system_table);
UINTN StrLen(const CHAR16* s);
void Print(const CHAR16* fmt, ...);

#endif
