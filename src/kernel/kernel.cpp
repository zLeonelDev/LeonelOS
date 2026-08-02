#include <types.h>
#include <debug.h>
#include <arch/x86_64/bootinfo.h>
#include <arch/x86_64/cpu/cpu.h>
#include <arch/x86_64/gdt/gdt.h>
#include <arch/x86_64/interrupts/interrupts.h>
#include <arch/x86_64/drivers/timer/timer.h>
#include <arch/x86_64/drivers/keyboard/keyboard.h>
#include <arch/x86_64/drivers/mouse/mouse.h>
#include <graphics/framebuffer/framebuffer.h>
#include <graphics/bootui/bootui.h>
#include <memory/physical/physical.h>
#include <memory/heap/heap.h>
#include <graphics/font/font.h>
#include <graphics/desktop/desktop.h>

extern "C" void kernel_main(FramebufferInfo* framebuffer, BootAssets* assets, MemoryMap* memory_map) {
    debug_init();
    debug_log("LeonelOS Kernel v0.1.0 - Start\n");

    framebuffer_init(framebuffer);
    debug_log("Framebuffer: %ux%u pitch=%u\n",
              (unsigned int)framebuffer->width,
              (unsigned int)framebuffer->height,
              (unsigned int)framebuffer->pitch);

    cpu_init();

    gdt_init();
    interrupts_init();

    timer_init(TIMER_FREQUENCY);
    keyboard_init();
    mouse_init();

    init_physical_memory(memory_map);

    init_heap();

    /* PMM sanity test: allocate three regions, write a marker into each,
     * verify it reads back correctly, then free them.  Confirms the
     * bitmap tracks pages and returns contiguous regions we can touch. */
    {
        phys_addr_t a = allocate_physical(4);
        phys_addr_t b = allocate_physical(16);
        phys_addr_t c = allocate_physical(1);
        if (a == PHYS_ALLOC_FAILED || b == PHYS_ALLOC_FAILED ||
            c == PHYS_ALLOC_FAILED) {
            debug_log("PMM test FAILED: allocation returned PHYS_ALLOC_FAILED\n");
        } else if (a == b || b == c || a == c) {
            debug_log("PMM test FAILED: overlapping regions\n");
        } else {
            volatile u32* pa = (volatile u32*)a;
            volatile u32* pb = (volatile u32*)b;
            volatile u32* pc = (volatile u32*)c;
            *pa = 0xDEADBEEF; *pb = 0xCAFEBABE; *pc = 0x12345678;
            u32 ok = (*pa == 0xDEADBEEF) && (*pb == 0xCAFEBABE) && (*pc == 0x12345678);
            debug_log("PMM test: %s (a=%lx b=%lx c=%lx)\n",
                      ok ? "OK" : "FAILED (read-back mismatch)",
                      (unsigned long)a, (unsigned long)b, (unsigned long)c);
            free_physical(a, 4);
            free_physical(b, 16);
            free_physical(c, 1);
            debug_log("PMM test: freed back, free=%lu KB\n",
                      (unsigned long)(get_free_memory() / 1024));
        }
    }

    interrupts_enable();
    debug_log("Kernel initialized successfully, interrupts on\n");

    bootui_init(assets);
    bootui_draw_boot_screen();

    font_init();

    desktop_run();
}
