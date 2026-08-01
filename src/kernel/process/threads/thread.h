#ifndef LEONELOS_THREAD_H
#define LEONELOS_THREAD_H

#include <types.h>

#define THREAD_NAME_MAX 64
#define THREAD_MAX 256
#define THREAD_STACK_SIZE (128 * KILOBYTE)

typedef void (*thread_func_t)(void*);

typedef struct Thread {
    usize id;
    char name[THREAD_NAME_MAX];
    u8* stack;
    usize stack_size;
    usize rip;
    usize rsp;
    usize rbp;
    u64 rflags;
    bool running;
    bool exited;
    int exit_code;
    void* user_data;
    struct Thread* next;
} Thread;

typedef struct {
    Thread* current;
    Thread* head;
    usize count;
    usize next_id;
} ThreadManager;

s32 thread_init();
Thread* thread_create(const char* name, thread_func_t entry, void* arg);
void thread_exit(int code);
void thread_yield();
Thread* thread_get_current();
Thread* thread_get_by_id(usize id);
void thread_schedule();

#endif