#include <thread.h>
#include <debug.h>
#include <heap.h>

static Thread thread_pool[THREAD_MAX];
static ThreadManager g_thread_manager = {0};

static usize next_thread_id = 1;

s32 thread_init() {
    for (usize i = 0; i < THREAD_MAX; i++) {
        thread_pool[i].id = 0;
        thread_pool[i].running = false;
        thread_pool[i].next = NULL;
    }
    g_thread_manager.current = NULL;
    g_thread_manager.head = NULL;
    g_thread_manager.count = 0;
    g_thread_manager.next_id = 1;
    
    debug_log("Thread subsystem initialized\n");
    return 0;
}

Thread* thread_create(const char* name, thread_func_t entry, void* arg) {
    for (usize i = 0; i < THREAD_MAX; i++) {
        if (thread_pool[i].id == 0) {
            Thread* t = &thread_pool[i];
            t->id = next_thread_id++;
            t->running = true;
            t->exited = false;
            t->exit_code = 0;
            t->user_data = arg;
            t->stack_size = THREAD_STACK_SIZE;
            t->stack = (u8*)kmalloc(THREAD_STACK_SIZE);
            t->rip = (usize)entry;
            t->rsp = (usize)t->stack + THREAD_STACK_SIZE;
            t->rbp = t->rsp;
            t->rflags = 0x202;
            
            usize j = 0;
            while (name[j] && j < THREAD_NAME_MAX - 1) {
                t->name[j] = name[j];
                j++;
            }
            t->name[j] = '\0';
            
            if (!g_thread_manager.head) {
                g_thread_manager.head = t;
            } else {
                Thread* cur = g_thread_manager.head;
                while (cur->next) cur = cur->next;
                cur->next = t;
            }
            
            g_thread_manager.count++;
            
            debug_log("Thread created: %s (id=%zu)\n", t->name, t->id);
            return t;
        }
    }
    return NULL;
}

void thread_exit(int code) {
    Thread* current = thread_get_current();
    if (current) {
        current->exited = true;
        current->exit_code = code;
        current->running = false;
        debug_log("Thread exited: %s (code=%d)\n", current->name, code);
    }
}

void thread_yield() {
}

Thread* thread_get_current() {
    return g_thread_manager.current;
}

Thread* thread_get_by_id(usize id) {
    Thread* t = g_thread_manager.head;
    while (t) {
        if (t->id == id) return t;
        t = t->next;
    }
    return NULL;
}

void thread_schedule() {
    if (!g_thread_manager.current) {
        g_thread_manager.current = g_thread_manager.head;
        return;
    }
    
    Thread* next = g_thread_manager.current->next;
    if (!next) {
        next = g_thread_manager.head;
    }
    g_thread_manager.current = next;
}