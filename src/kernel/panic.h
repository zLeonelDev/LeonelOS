#ifndef LEONELOS_PANIC_H
#define LEONELOS_PANIC_H

#include <types.h>

__attribute__((noreturn)) void panic(const char* file, int line, const char* fmt, ...);

#define panic(...) panic(__FILE__, __LINE__, __VA_ARGS__)

#define ASSERT(cond) \
    do { \
        if (!(cond)) { \
            panic("Assertion failed: %s", #cond); \
        } \
    } while (0)

#define ASSERT_MSG(cond, msg, ...) \
    do { \
        if (!(cond)) { \
            panic("Assertion failed: %s: " msg, #cond, __VA_ARGS__); \
        } \
    } while (0)

#endif