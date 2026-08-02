#ifndef LEONELOS_BOOTUI_H
#define LEONELOS_BOOTUI_H

#include <types.h>
#include <arch/x86_64/bootinfo.h>

#ifdef __cplusplus
extern "C" {
#endif

void bootui_init(BootAssets* assets);
void bootui_draw_boot_screen();
void bootui_set_loading_text(const char* text);
void bootui_draw_spinner(u32 x, u32 y);
void bootui_draw_ring(u32 x, u32 y, u32 size);
void bootui_spinner_pos(u32* x, u32* y, u32* size);
void bootui_animate_loading();

#ifdef __cplusplus
}
#endif

#endif