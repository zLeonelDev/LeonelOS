#include <types.h>
#include <graphics.h>

/*
 * Pixel packing.
 *
 * Assets live in memory in RGBA byte order: byte0=R, byte1=G, byte2=B, byte3=A.
 * On a little-endian u32, that lands as A in the MSB and R in the LSB, so a
 * raw src u32 is read as:
 *     r = (u32 >> 16) & 0xFF  (no — that's B)
 * The correct read of an RGBA byte-order asset is:
 *     a = (u32 >> 24) & 0xFF
 *     r = u32 & 0xFF         (LSB)
 *     g = (u32 >> 8) & 0xFF
 *     b = (u32 >> 16) & 0xFF
 *
 * The destination framebuffer may be either RGBA or BGRA byte order. We
 * detect this from fb->pixel_format (EFI value 0 / 1).
 *
 * Per-byte memory layout to dst for each pixel format (little-endian u32):
 *   RGBA: byte0=R, byte1=G, byte2=B, byte3=A -> u32 = (A<<24)|(B<<16)|(G<<8)|R
 *   BGRA: byte0=B, byte1=G, byte2=R, byte3=A -> u32 = (A<<24)|(R<<16)|(G<<8)|B
 */

#define PF_UNKNOWN 0xFFFFFFFFu

static u32 pack_bgra(u8 r, u8 g, u8 b, u8 a) {
    return (0xFF000000u) | ((u32)r << 16) | ((u32)g << 8) | (u32)b;
}

static u32 pack_rgba(u8 r, u8 g, u8 b, u8 a) {
    return (0xFF000000u) | ((u32)b << 16) | ((u32)g << 8) | (u32)r;
}

/* Read pixel from background framebuffer back to (r,g,b) according to format. */
static void unpack_bg(u32 bg, u32 pf, u8* br, u8* bgch, u8* bb) {
    if (pf == 1) {
        /* BGRA */
        *bb = (u8)(bg & 0xFF);
        *bgch = (u8)((bg >> 8) & 0xFF);
        *br = (u8)((bg >> 16) & 0xFF);
    } else {
        /* RGBA */
        *br = (u8)(bg & 0xFF);
        *bgch = (u8)((bg >> 8) & 0xFF);
        *bb = (u8)((bg >> 16) & 0xFF);
    }
}

static u32 pack_dst(u8 r, u8 g, u8 b, u32 pf) {
    if (pf == 1) return pack_bgra(r, g, b, 0xFF);
    return pack_rgba(r, g, b, 0xFF);
}

static u32 pf_or_default(const FramebufferInfo* fb) {
    if (!fb) return 1;
    if (fb->pixel_format == 0) return 1;
    return fb->pixel_format;
}

VOID graphics_clear(FramebufferInfo* fb, u32 color) {
    if (!fb || !fb->address) return;
    u32* dst = (u32*)fb->address;
    u64 count = (u64)fb->width * fb->height;
    for (u64 i = 0; i < count; i++) dst[i] = color;
}

VOID graphics_draw_pixel(FramebufferInfo* fb, u32 x, u32 y, u32 color) {
    if (!fb || !fb->address) return;
    if (x >= fb->width || y >= fb->height) return;
    u32* dst = (u32*)fb->address;
    dst[y * (fb->pitch / 4) + x] = color;
}

u32 graphics_get_pixel(FramebufferInfo* fb, u32 x, u32 y) {
    if (!fb || !fb->address) return 0;
    if (x >= fb->width || y >= fb->height) return 0;
    u32* dst = (u32*)fb->address;
    return dst[y * (fb->pitch / 4) + x];
}

VOID graphics_draw_rect(FramebufferInfo* fb, u32 x, u32 y, u32 w, u32 h, u32 color) {
    if (!fb || !fb->address) return;
    if (x >= fb->width || y >= fb->height) return;
    if (x + w > fb->width) w = fb->width - x;
    if (y + h > fb->height) h = fb->height - y;

    u32* dst = (u32*)fb->address;
    u32 stride = fb->pitch / 4;

    for (u32 row = 0; row < h; row++) {
        u32* line = dst + (y + row) * stride + x;
        for (u32 col = 0; col < w; col++) line[col] = color;
    }
}

static VOID blend_pixel(FramebufferInfo* fb, u32* dst, u32 src_rgba) {
    u8 a = (u8)(src_rgba >> 24);
    if (a == 0) return;
    /* Source assets are RGBA byte order: byte0=R, byte1=G, byte2=B, byte3=A.
     * Read them as a transparent texture independent of the dst format. */
    u8 r1 = (u8)(src_rgba & 0xFF);
    u8 g1 = (u8)((src_rgba >> 8) & 0xFF);
    u8 b1 = (u8)((src_rgba >> 16) & 0xFF);

    if (a == 255) {
        *dst = pack_dst(r1, g1, b1, pf_or_default(fb));
        return;
    }

    u32 bg = *dst;
    u8 br2, bgch2, bb2;
    unpack_bg(bg, pf_or_default(fb), &br2, &bgch2, &bb2);

    u32 ia = 255 - a;
    u8 r = (u8)(((u32)r1 * a + (u32)br2 * ia + 127) / 255);
    u8 g = (u8)(((u32)g1 * a + (u32)bgch2 * ia + 127) / 255);
    u8 b = (u8)(((u32)b1 * a + (u32)bb2 * ia + 127) / 255);

    *dst = pack_dst(r, g, b, pf_or_default(fb));
}

VOID graphics_draw_rgba_scaled(FramebufferInfo* fb, void* src, u32 src_w, u32 src_h,
                               u32 dst_x, u32 dst_y, u32 dst_w, u32 dst_h) {
    if (!fb || !fb->address || !src) return;
    if (dst_x >= fb->width || dst_y >= fb->height) return;
    if (dst_x + dst_w > fb->width) dst_w = fb->width - dst_x;
    if (dst_y + dst_h > fb->height) dst_h = fb->height - dst_y;
    if (dst_w == 0 || dst_h == 0 || src_w == 0 || src_h == 0) return;

    u32* dst = (u32*)fb->address;
    u32* src_px = (u32*)src;
    u32 stride = fb->pitch / 4;

    float x_ratio = (float)src_w / (float)dst_w;
    float y_ratio = (float)src_h / (float)dst_h;

    for (u32 y = 0; y < dst_h; y++) {
        u32 src_y = (u32)(y * y_ratio);
        if (src_y >= src_h) src_y = src_h - 1;
        for (u32 x = 0; x < dst_w; x++) {
            u32 src_x = (u32)(x * x_ratio);
            if (src_x >= src_w) src_x = src_w - 1;
            blend_pixel(fb, &dst[(dst_y + y) * stride + dst_x + x], src_px[src_y * src_w + src_x]);
        }
    }
}

VOID graphics_draw_rgba(FramebufferInfo* fb, void* src, u32 src_w, u32 src_h,
                        u32 dst_x, u32 dst_y) {
    graphics_draw_rgba_scaled(fb, src, src_w, src_h, dst_x, dst_y, src_w, src_h);
}
