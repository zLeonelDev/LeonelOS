#ifndef LEONELOS_FRAMEBUFFER_H
#define LEONELOS_FRAMEBUFFER_H

#include <types.h>
#include <arch/x86_64/bootinfo.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Pixel formats (mirror the EFI_GRAPHICS_PIXEL_FORMAT enum).
 * Only the ones we care about are enumerated here; default/mask-formats
 * fall through to RGBA at the bottom.
 */
#define PIXEL_FMT_UNKNOWN         0
#define PIXEL_FMT_RGBA            1   /* EFI PixelRedGreenBlueReserved8BitPerColor */
#define PIXEL_FMT_BGRA            2   /* EFI PixelBlueGreenRedReserved8BitPerColor */

typedef struct {
    void* base;
    u32 width;
    u32 height;
    u32 pitch;
    u32 bpp;
    u32 pixel_format;
} Framebuffer;

void framebuffer_init(FramebufferInfo* info);
Framebuffer* framebuffer_get();
void framebuffer_clear(u32 color);
void framebuffer_set_pixel(u32 x, u32 y, u32 color);
u32 framebuffer_get_pixel(u32 x, u32 y);
void framebuffer_draw_rect(u32 x, u32 y, u32 w, u32 h, u32 color);
void framebuffer_draw_rgba(void* data, u32 src_w, u32 src_h,
                             u32 dst_x, u32 dst_y,
                             u32 dst_w, u32 dst_h);
void framebuffer_blit();

#ifdef __cplusplus
}
#endif

#endif
