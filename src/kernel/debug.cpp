#include <debug.h>
#include <types.h>
#include <io.h>
#include <stdint.h>
#include <stdarg.h>

#define COM1 0x3F8

static int debug_serial_initialized = 0;

static void debug_serial_write(u8 byte) {
    while ((port_byte_in(COM1 + 5) & 0x20) == 0);
    port_byte_out(COM1, byte);
}

extern "C" {

void debug_init(void) {
    if (debug_serial_initialized) return;
    
    port_byte_out(COM1 + 1, 0x00);   /* IER: disable interrupts */
    port_byte_out(COM1 + 3, 0x80);   /* LCR: enable DLAB */
    port_byte_out(COM1 + 0, 0x01);   /* DLL: 115200 baud divisor low */
    port_byte_out(COM1 + 1, 0x00);   /* DLM: divisor high */
    port_byte_out(COM1 + 3, 0x03);   /* LCR: 8N1 */
    port_byte_out(COM1 + 4, 0x03);   /* MCR: DTR+RTS */
    
    debug_serial_initialized = 1;
}

static void debug_putc(char c) {
    if (c == '\n') {
        debug_serial_write('\r');
    }
    debug_serial_write(c);
}

static int debug_put_num(char* buf, u64 val, int radix, const char* digits) {
    char tmp[24];
    int n = 0;
    if (val == 0) {
        tmp[n++] = '0';
    } else {
        while (val > 0 && n < 24) {
            tmp[n++] = digits[val % radix];
            val /= radix;
        }
    }
    for (int i = 0; i < n; i++) {
        buf[i] = tmp[n - 1 - i];
    }
    return n;
}

static int debug_vprint(const char* fmt, va_list args) {
    char buffer[512];
    int len = 0;
    
    for (int i = 0; fmt[i] && len < 510; i++) {
        if (fmt[i] == '%' && fmt[i + 1]) {
            i++;
            int lcount = 0;
            while (fmt[i] == 'l') {
                lcount++;
                i++;
            }
            switch (fmt[i]) {
                case 's': {
                    const char* str = va_arg(args, char*);
                    for (int j = 0; str[j] && len < 510; j++) {
                        buffer[len++] = str[j];
                    }
                    break;
                }
                case 'c': {
                    buffer[len++] = (char)va_arg(args, int);
                    break;
                }
                case 'd': {
                    if (lcount) {
                        s64 val = va_arg(args, s64);
                        if (val < 0) {
                            buffer[len++] = '-';
                            val = -val;
                        }
                        len += debug_put_num(buffer + len, (u64)val, 10, "0123456789");
                    } else {
                        int val = va_arg(args, int);
                        if (val < 0) {
                            buffer[len++] = '-';
                            val = -val;
                        }
                        len += debug_put_num(buffer + len, (u64)(u32)val, 10, "0123456789");
                    }
                    break;
                }
                case 'u': {
                    u64 val = lcount ? va_arg(args, u64) : (u64)va_arg(args, unsigned int);
                    len += debug_put_num(buffer + len, val, 10, "0123456789");
                    break;
                }
                case 'x':
                case 'X': {
                    u64 val = lcount ? va_arg(args, u64) : (u64)va_arg(args, unsigned int);
                    len += debug_put_num(buffer + len, val, 16,
                                         fmt[i] == 'X' ? "0123456789ABCDEF" : "0123456789abcdef");
                    break;
                }
                case 'p': {
                    usize val = va_arg(args, usize);
                    buffer[len++] = '0';
                    buffer[len++] = 'x';
                    len += debug_put_num(buffer + len, (u64)val, 16, "0123456789abcdef");
                    break;
                }
                case '%': {
                    buffer[len++] = '%';
                    break;
                }
                default: {
                    buffer[len++] = '%';
                    buffer[len++] = fmt[i];
                    break;
                }
            }
        } else {
            buffer[len++] = fmt[i];
        }
    }
    
    for (int i = 0; i < len; i++) {
        debug_putc(buffer[i]);
    }
    
    return len;
}

void debug_print(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    debug_vprint(fmt, args);
    va_end(args);
}

void debug_log(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    debug_vprint(fmt, args);
    va_end(args);
}

void debug_hex_dump(void* ptr, usize len) {
    u8* data = (u8*)ptr;
    for (usize i = 0; i < len; i += 16) {
        debug_log("%08x: ", (usize)(data + i));
        for (int j = 0; j < 16; j++) {
            if (i + j < len) {
                debug_log("%02x ", data[i + j]);
            } else {
                debug_log("   ");
            }
        }
        debug_log(" ");
        for (int j = 0; j < 16 && i + j < len; j++) {
            char c = data[i + j];
            if (c >= 32 && c < 127) {
                debug_log("%c", c);
            } else {
                debug_log(".");
            }
        }
        debug_log("\n");
    }
}

} // extern "C"