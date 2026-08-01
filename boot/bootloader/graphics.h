#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <boot.h>

VOID graphics_clear(FramebufferInfo* fb, u32 color);
VOID graphics_draw_pixel(FramebufferInfo* fb, u32 x, u32 y, u32 color);
u32  graphics_get_pixel(FramebufferInfo* fb, u32 x, u32 y);
VOID graphics_draw_rect(FramebufferInfo* fb, u32 x, u32 y, u32 w, u32 h, u32 color);
VOID graphics_draw_rgba(FramebufferInfo* fb, void* src, u32 src_w, u32 src_h,
                        u32 dst_x, u32 dst_y);
VOID graphics_draw_rgba_scaled(FramebufferInfo* fb, void* src, u32 src_w, u32 src_h,
                               u32 dst_x, u32 dst_y, u32 dst_w, u32 dst_h);

#endif
