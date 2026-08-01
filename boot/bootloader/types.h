#ifndef BOOTLOADER_TYPES_H
#define BOOTLOADER_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;
typedef int8_t    i8;
typedef int16_t   i16;
typedef int32_t   i32;
typedef int64_t   i64;
typedef uintptr_t usize;
typedef intptr_t  isize;

#ifndef NULL
#define NULL ((void*)0)
#endif

#define KILOBYTE 1024
#define MEGABYTE (1024 * KILOBYTE)
#define GIGABYTE (1024 * MEGABYTE)

#define PAGE_SIZE 0x1000
#define PAGE_SHIFT 12

#endif
