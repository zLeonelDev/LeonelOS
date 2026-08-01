#ifndef LEONELOS_DEBUG_H
#define LEONELOS_DEBUG_H

#include <types.h>

#ifdef __cplusplus
extern "C" {
#endif

void debug_init(void);
void debug_print(const char* fmt, ...);
void debug_log(const char* fmt, ...);
void debug_hex_dump(void* ptr, usize len);

#ifdef __cplusplus
}
#endif

#define debug_print(...) debug_print(__VA_ARGS__)
#define debug_log(...) debug_log(__VA_ARGS__)

#endif