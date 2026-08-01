#ifndef LEONELOS_TYPES_H
#define LEONELOS_TYPES_H

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
typedef int8_t    s8;
typedef int16_t   s16;
typedef int32_t   s32;
typedef int64_t   s64;
typedef uintptr_t usize;
typedef intptr_t  isize;

typedef float  f32;
typedef double f64;

typedef u64 phys_addr_t;
typedef u64 virt_addr_t;

#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

#define ALIGN_UP(x, align) (((x) + (align) - 1) & ~((align) - 1))
#define ALIGN_DOWN(x, align) ((x) & ~((align) - 1))

#define PAGE_SIZE 0x1000
#define PAGE_SHIFT 12
#define PAGE_MASK (PAGE_SIZE - 1)

#define KERNEL_BASE 0xFFFFFFFF80000000
#define KERNEL_STACK_SIZE 0x40000

#define PAGE_PRESENT    (1ULL << 0)
#define PAGE_WRITE      (1ULL << 1)
#define PAGE_USER       (1ULL << 2)
#define PAGE_WRITE_THROUGH (1ULL << 3)
#define PAGE_CACHE_DISABLE (1ULL << 4)
#define PAGE_ACCESSED   (1ULL << 5)
#define PAGE_DIRTY      (1ULL << 6)
#define PAGE_HUGE       (1ULL << 7)
#define PAGE_GLOBAL     (1ULL << 8)
#define PAGE_NX         (1ULL << 63)

#define KERNEL_PAGE_FLAGS (PAGE_PRESENT | PAGE_WRITE | PAGE_GLOBAL)

#define KILOBYTE 1024
#define MEGABYTE (1024 * KILOBYTE)
#define GIGABYTE (1024 * MEGABYTE)

#define KERNEL_HEAP_START 0xFFFFFFFFC0000000
#define KERNEL_HEAP_SIZE  (256 * MEGABYTE)

#define USERSPACE_BASE 0x0000000000000000
#define USERSPACE_TOP  0x00007FFFFFFFFFFF

#define KERNEL_VIRTUAL_BASE 0xFFFFFFFF80000000
#define KERNEL_VIRTUAL_END  0xFFFFFFFFFFFFFFFF

#define PHYSICAL_MEMORY_BASE 0x0000000000000000
#define PHYSICAL_MEMORY_MAX  0x000000FFFFFFFFFF

#define INTERRUPT_GATE 0x8E
#define TRAP_GATE      0x8F
#define TASK_GATE      0x85

#define GDT_NULL        0x00
#define GDT_KERNEL_CODE 0x08
#define GDT_KERNEL_DATA 0x10
#define GDT_USER_DATA   0x18
#define GDT_USER_CODE   0x20
#define GDT_TSS         0x28

#define IRQ_BASE 0x20

#define SYSCALL_VECTOR 0x80

#define PAGE_TABLE_ENTRIES 512

typedef struct {
    u64 base;
    u16 limit;
} __attribute__((packed)) DescriptorTablePtr;

typedef struct {
    u64 rip;
    u64 cs;
    u64 rflags;
    u64 rsp;
    u64 ss;
} __attribute__((packed)) InterruptFrame;

typedef struct {
    u64 rax, rbx, rcx, rdx, rsi, rdi, rbp, rsp;
    u64 r8, r9, r10, r11, r12, r13, r14, r15;
    u64 rip, rflags, cs, ss;
} __attribute__((packed)) CpuRegisters;

#endif