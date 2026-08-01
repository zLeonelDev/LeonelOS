#include "timer.h"
#include <debug.h>
#include <io.h>
#include <irq.h>
#include <pic.h>

#define PIT_CHANNEL0  0x40
#define PIT_COMMAND   0x43
#define PIT_FREQUENCY 1193182

static volatile u64 g_ticks = 0;

static void timer_irq_handler(void) {
    g_ticks = g_ticks + 1;
    if ((g_ticks % TIMER_FREQUENCY) == 0) {
        debug_log("PIT: %llu s\n", (u64)(g_ticks / TIMER_FREQUENCY));
    }
}

void timer_init(u32 frequency) {
    if (frequency == 0 || frequency > PIT_FREQUENCY) {
        frequency = TIMER_FREQUENCY;
    }
    u32 divisor = PIT_FREQUENCY / frequency;
    outb(PIT_COMMAND, 0x36);
    outb(PIT_CHANNEL0, (u8)(divisor & 0xFF));
    outb(PIT_CHANNEL0, (u8)((divisor >> 8) & 0xFF));
    irq_register_handler(0, timer_irq_handler);
    irq_enable(0);
    debug_log("Timer: PIT at %u Hz\n", frequency);
}

u64 timer_get_ticks(void) {
    return g_ticks;
}

void timer_sleep(u32 ms) {
    u64 target = g_ticks + ((u64)ms * TIMER_FREQUENCY + 999) / 1000;
    while (g_ticks < target) {
        asm volatile("hlt");
    }
}
