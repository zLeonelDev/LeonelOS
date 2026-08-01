#include <interrupts.h>
#include <idt.h>
#include <pic.h>

void interrupts_init(void) {
    idt_init();
    pic_init();
}
