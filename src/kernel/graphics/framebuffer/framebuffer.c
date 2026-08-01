#include <framebuffer.h>
#include <debug.h>
#include <types.h>

static Framebuffer g_framebuffer;

static inline u8 pixel_format_to_kind(u32 pf) {
    switch (pf) {
        case 0:  return PIXEL_FMT_RGBA;   /* EFI PixelRedGreenBlueReserved8BitPerColor */
        case 1:  return PIXEL_FMT_BGRA;   /* EFI PixelBlueGreenRedReserved8BitPerColor */
        case 2:  return PIXEL_FMT_BGRA;   /* EFI PixelBitMask — handled by masks, default BGRA */
        default: return PIXEL_FMT_BGRA;
    }
}

static inline u32 pack_rgba(u8 kind, u8 r, u8 g, u8 b, u8 a) {
    if (kind == PIXEL_FMT_RGBA) {
        /* Byte order in memory: byte0=R, byte1=G, byte2=B, byte3=A. As a u32,
         * that's (A<<24) | (B<<16) | (G<<8) | R. */
        return ((u32)a << 24) | ((u32)b << 16) | ((u32)g << 8) | r;
    }
    /* BGRA: byte0=B, byte1=G, byte2=R, byte3=A. As a u32: (A<<24)|(R<<16)|(G<<8)|B. */
    return ((u32)a << 24) | ((u32)r << 16) | ((u32)g << 8) | b;
}

void framebuffer_init(FramebufferInfo* info) {
    g_framebuffer.base = info->address;
    g_framebuffer.width = info->width;
    g_framebuffer.height = info->height;
    g_framebuffer.pitch = info->pitch;
    g_framebuffer.bpp = info->bpp;
    g_framebuffer.pixel_format = pixel_format_to_kind(info->pixel_format);

    debug_log("Framebuffer: %ux%u pitch=%u fmt=%s\n",
              g_framebuffer.width, g_framebuffer.height, g_framebuffer.pitch,
              g_framebuffer.pixel_format == PIXEL_FMT_RGBA ? "RGBA" : "BGRA");
}

Framebuffer* framebuffer_get() {
    return &g_framebuffer;
}

void framebuffer_clear(u32 color) {
    u32* dst = (u32*)g_framebuffer.base;
    u64 count = (u64)g_framebuffer.width * g_framebuffer.height;
    for (u64 i = 0; i < count; i++) {
        dst[i] = color;
    }
}

void framebuffer_set_pixel(u32 x, u32 y, u32 color) {
    if (x >= g_framebuffer.width || y >= g_framebuffer.height) return;
    u32* dst = (u32*)g_framebuffer.base;
    dst[y * (g_framebuffer.pitch / 4) + x] = color;
}

u32 framebuffer_get_pixel(u32 x, u32 y) {
    if (x >= g_framebuffer.width || y >= g_framebuffer.height) return 0;
    u32* dst = (u32*)g_framebuffer.base;
    return dst[y * (g_framebuffer.pitch / 4) + x];
}

void framebuffer_draw_rect(u32 x, u32 y, u32 w, u32 h, u32 color) {
    if (x >= g_framebuffer.width || y >= g_framebuffer.height) return;
    if (x + w > g_framebuffer.width) w = g_framebuffer.width - x;
    if (y + h > g_framebuffer.height) h = g_framebuffer.height - y;

    u32* dst = (u32*)g_framebuffer.base;
    u32 stride = g_framebuffer.pitch / 4;

    for (u32 row = 0; row < h; row++) {
        for (u32 col = 0; col < w; col++) {
            dst[(y + row) * stride + (x + col)] = color;
        }
    }
}

void framebuffer_draw_rgba(void* data, u32 src_w, u32 src_h,
                             u32 dst_x, u32 dst_y,
                             u32 dst_w, u32 dst_h) {
    if (!data) return;
    if (dst_x >= g_framebuffer.width || dst_y >= g_framebuffer.height) return;
    if (dst_x + dst_w > g_framebuffer.width) dst_w = g_framebuffer.width - dst_x;
    if (dst_y + dst_h > g_framebuffer.height) dst_h = g_framebuffer.height - dst_y;

    u8 kind = g_framebuffer.pixel_format;
    u32* dst = (u32*)g_framebuffer.base;
    u32* src = (u32*)data;
    u32 stride = g_framebuffer.pitch / 4;

    u32 x_step = (src_w << 16) / dst_w;
    u32 y_step = (src_h << 16) / dst_h;
    u32 fy = 0;

    for (u32 y = 0; y < dst_h; y++) {
        u32 src_y = fy >> 16;
        fy += y_step;
        if (src_y >= src_h) src_y = src_h - 1;

        u32 fx = 0;
        for (u32 x = 0; x < dst_w; x++) {
            u32 src_x = fx >> 16;
            fx += x_step;
            if (src_x >= src_w) src_x = src_w - 1;

            u32 color = src[src_y * src_w + src_x];
            /* Assets are RGBA byte order: byte0=R, byte1=G, byte2=B, byte3=A.
             * On a little-endian u32, that lands as A at MSB and R at LSB. */
            u8 a = (color >> 24) & 0xFF;
            u8 b = (color >> 16) & 0xFF;
            u8 g = (color >> 8) & 0xFF;
            u8 r = color & 0xFF;

            if (a == 0) continue;

            u32 d_idx = (dst_y + y) * stride + dst_x + x;
            u32 out;
            if (a == 255) {
                out = pack_rgba(kind, r, g, b, 0xFF);
            } else {
                u32 bg = dst[d_idx];
                u8 br, bgch, bb;
                if (kind == PIXEL_FMT_RGBA) {
                    br = bg & 0xFF;
                    bgch = (bg >> 8) & 0xFF;
                    bb = (bg >> 16) & 0xFF;
                } else {
                    bb = bg & 0xFF;
                    bgch = (bg >> 8) & 0xFF;
                    br = (bg >> 16) & 0xFF;
                }
                u8 rr = (u8)((r * a + br * (255 - a) + 127) / 255);
                u8 gg = (u8)((g * a + bgch * (255 - a) + 127) / 255);
                u8 bb_out = (u8)((b * a + bb * (255 - a) + 127) / 255);
                out = pack_rgba(kind, rr, gg, bb_out, 0xFF);
            }
            dst[d_idx] = out;
        }
    }
}

void framebuffer_blit() {
}
