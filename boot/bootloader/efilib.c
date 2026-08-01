#include <efi.h>
#include <efilib.h>
#include <types.h>

/* MSVC-target code that uses float emits a reference to _fltused; the
 * freestanding bootloader has no CRT to provide it. */
int _fltused = 1;

static EFI_SYSTEM_TABLE* gST = NULL;
static EFI_HANDLE gImageHandle = 0;

void efi_lib_init(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE* system_table) {
    gImageHandle = image_handle;
    gST = system_table;
}

UINTN StrLen(const CHAR16* s) {
    UINTN n = 0;
    if (!s) return 0;
    while (s[n]) n++;
    return n;
}

/* Format value (base 10 or 16) into buf, return character count. */
static UINTN u64_to_str(UINT64 value, UINTN base, CHAR16* buf, BOOLEAN upper) {
    CHAR16 digits[20];
    UINTN i = 0;
    if (value == 0) {
        digits[i++] = '0';
    } else {
        while (value > 0 && i < 20) {
            UINT64 d = value % base;
            digits[i++] = (CHAR16)(d < 10 ? ('0' + d)
                : (upper ? ('A' + (d - 10)) : ('a' + (d - 10))));
            value /= base;
        }
    }
    UINTN n = 0;
    while (i > 0) buf[n++] = digits[--i];
    buf[n] = 0;
    return n;
}

void Print(const CHAR16* fmt, ...) {
    if (!gST || !gST->ConOut) return;

    CHAR16 num_buf[20];
    CHAR16 out[256];
    UINTN o = 0;

    va_list ap;
    va_start(ap, fmt);

    for (const CHAR16* p = fmt; *p; p++) {
        if (*p != '%') {
            out[o++] = *p;
        } else {
            p++;
            if (*p == '%') {
                out[o++] = '%';
                continue;
            }
            if (!*p) break;

            BOOLEAN is64 = FALSE;
            if (*p == 'l' && p[1] == 'l') {
                is64 = TRUE;
                p++;
                p++;
            } else if (*p == 'l') {
                is64 = TRUE;
                p++;
            }

            CHAR16 spec = *p;
            switch (spec) {
                case 'u':
                case 'd':
                case 'x':
                case 'X':
                case 'p': {
                    UINT64 value;
                    UINTN n;
                    if (spec == 'p' || is64) {
                        value = va_arg(ap, UINT64);
                    } else {
                        value = (UINT64)va_arg(ap, UINT32);
                    }
                    if (spec == 'p') {
                        out[o++] = '0';
                        out[o++] = 'x';
                        n = u64_to_str(value, 16, num_buf, FALSE);
                    } else if (spec == 'x') {
                        n = u64_to_str(value, 16, num_buf, FALSE);
                    } else if (spec == 'X') {
                        n = u64_to_str(value, 16, num_buf, TRUE);
                    } else {
                        n = u64_to_str(value, 10, num_buf, FALSE);
                    }
                    for (UINTN i = 0; i < n && o < 250; i++) out[o++] = num_buf[i];
                    break;
                }
                case 's': {
                    CHAR16* s = va_arg(ap, CHAR16*);
                    if (!s) s = L"(null)";
                    while (*s && o < 250) out[o++] = *s++;
                    break;
                }
                case 'c': {
                    out[o++] = (CHAR16)va_arg(ap, int);
                    break;
                }
                default: {
                    out[o++] = '%';
                    if (spec) out[o++] = spec;
                    break;
                }
            }
        }
        if (o >= 250) {
            out[o] = 0;
            gST->ConOut->OutputString(gST->ConOut, out);
            o = 0;
        }
    }

    out[o] = 0;
    va_end(ap);
    gST->ConOut->OutputString(gST->ConOut, out);
}
