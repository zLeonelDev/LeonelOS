#include <types.h>
#include <debug.h>
#include <arch/x86_64/bootinfo.h>
#include <arch/x86_64/cpu/cpu.h>
#include <arch/x86_64/gdt/gdt.h>
#include <arch/x86_64/interrupts/interrupts.h>
#include <arch/x86_64/drivers/timer/timer.h>
#include <arch/x86_64/drivers/keyboard/keyboard.h>
#include <graphics/framebuffer/framebuffer.h>
#include <graphics/bootui/bootui.h>

extern "C" void kernel_main(FramebufferInfo* framebuffer, BootAssets* assets, MemoryMap* memory_map) {
    UNUSED(memory_map);

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

    interrupts_enable();
    debug_log("Kernel initialized successfully, interrupts on\n");

    bootui_init(assets);
    bootui_draw_boot_screen();

    bootui_animate_loading();
}
