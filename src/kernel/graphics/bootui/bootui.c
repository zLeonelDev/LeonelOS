#include <bootui.h>
#include <framebuffer.h>
#include <keyboard.h>
#include <timer.h>
#include <types.h>
#include <debug.h>

#define SPINNER_SPEED 3

#define FIX_SHIFT 12
#define FIX_ONE (1 << FIX_SHIFT)

/* Layout is expressed as a fraction of the smaller framebuffer dimension so
 * the boot screen scales naturally across 2048x2048, 1366x768, 1024x768, etc.
 * Each unit is a fixed-point Q12 value for accurate non-integer scaling. */
#define LOGO_TARGET_W_FP    3072   /* 0.75 * 4096 */
#define ICON_TARGET_W_FP    1024   /* 0.25 * 4096 */
#define SPINNER_TARGET_W_FP  819   /* 0.20 * 4096 — small enough that the spinner ring stays fully on-screen */
#define GAP_FP              256    /* 0.0625 * 4096 — vertical gap between elements */

static BootAssets* g_assets = NULL;
static u32 g_spinner_angle = 0;

static u32 g_layout_logo_x = 0;
static u32 g_layout_logo_y = 0;
static u32 g_layout_logo_w = 0;
static u32 g_layout_logo_h = 0;

static u32 g_layout_icon_x = 0;
static u32 g_layout_icon_y = 0;
static u32 g_layout_icon_w = 0;
static u32 g_layout_icon_h = 0;

static u32 g_layout_spinner_x = 0;
static u32 g_layout_spinner_y = 0;
static u32 g_layout_spinner_size = 0;

static const s32 k_sin_tab[91] = {
    0, 71, 143, 214, 286, 357, 428, 499, 570, 641, 711, 782, 852, 921,
    991, 1060, 1129, 1198, 1266, 1334, 1401, 1468, 1534, 1600, 1666,
    1731, 1796, 1860, 1923, 1986, 2048, 2110, 2171, 2231, 2290, 2349,
    2408, 2465, 2522, 2578, 2633, 2687, 2741, 2793, 2845, 2896, 2946,
    2996, 3044, 3091, 3138, 3183, 3228, 3271, 3314, 3355, 3396, 3435,
    3474, 3511, 3547, 3582, 3617, 3650, 3681, 3712, 3742, 3770, 3798,
    3824, 3849, 3873, 3896, 3917, 3937, 3956, 3974, 3991, 4006, 4021,
    4034, 4046, 4056, 4065, 4074, 4080, 4086, 4090, 4094, 4095, 4096
};

static s32 bootui_sin_deg(u32 deg) {
    deg %= 360;
    s32 s;
    if (deg < 90) {
        s = k_sin_tab[deg];
    } else if (deg < 180) {
        s = k_sin_tab[180 - deg];
    } else if (deg < 270) {
        s = -k_sin_tab[deg - 180];
    } else {
        s = -k_sin_tab[360 - deg];
    }
    return s;
}

static s32 bootui_cos_deg(u32 deg) {
    return bootui_sin_deg(deg + 90);
}

/* scale a fixed-point target dimension (Q12) by the smaller framebuffer side. */
static u32 scale_fp(u32 target_fp, u32 basis) {
    u64 n = (u64)target_fp * basis;
    return (u32)(n >> FIX_SHIFT);
}

static void bootui_compute_layout(Framebuffer* fb) {
    u32 basis = fb->width < fb->height ? fb->width : fb->height;

    u32 logo_w = scale_fp(LOGO_TARGET_W_FP, basis);
    if (g_assets && g_assets->leonelos.width > 0 && g_assets->leonelos.height > 0) {
        u64 h = (u64)logo_w * g_assets->leonelos.height / g_assets->leonelos.width;
        if (h > (u64)basis) h = basis;
        g_layout_logo_w = logo_w;
        g_layout_logo_h = (u32)h;
        g_layout_logo_x = (fb->width - g_layout_logo_w) / 2;
    } else {
        g_layout_logo_w = g_layout_logo_h = 0;
    }

    u32 icon_w = scale_fp(ICON_TARGET_W_FP, basis);
    if (g_assets && g_assets->icon.width > 0 && g_assets->icon.height > 0) {
        u64 h = (u64)icon_w * g_assets->icon.height / g_assets->icon.width;
        if (h > (u64)basis) h = basis;
        g_layout_icon_w = icon_w;
        g_layout_icon_h = (u32)h;
        g_layout_icon_x = (fb->width - g_layout_icon_w) / 2;
    } else {
        g_layout_icon_w = g_layout_icon_h = 0;
    }

    g_layout_spinner_size = scale_fp(SPINNER_TARGET_W_FP, basis);
    g_layout_spinner_x = (fb->width - g_layout_spinner_size) / 2;

    u32 gap = scale_fp(GAP_FP, basis);
    u32 total_h = g_layout_logo_h + gap + g_layout_icon_h + gap + g_layout_spinner_size;
    u32 start_y = (fb->height >= total_h) ? (fb->height - total_h) / 2 : 0;

    g_layout_logo_y = start_y;
    g_layout_icon_y = start_y + g_layout_logo_h + gap;
    g_layout_spinner_y = start_y + g_layout_logo_h + gap + g_layout_icon_h + gap;
}

void bootui_init(BootAssets* assets) {
    g_assets = assets;
    Framebuffer* fb = framebuffer_get();
    if (fb) bootui_compute_layout(fb);
}

void bootui_draw_boot_screen() {
    Framebuffer* fb = framebuffer_get();
    if (!fb || !fb->base) return;

    framebuffer_clear(0x00000000);

    if (!g_assets || g_layout_logo_w == 0) {
        if (g_layout_logo_w == 0 && g_assets) bootui_compute_layout(fb);
        if (g_layout_logo_w == 0) return;
    }

    if (g_assets->leonelos.data && g_layout_logo_w > 0 && g_layout_logo_h > 0) {
        framebuffer_draw_rgba(g_assets->leonelos.data,
                              g_assets->leonelos.width, g_assets->leonelos.height,
                              g_layout_logo_x, g_layout_logo_y,
                              g_layout_logo_w, g_layout_logo_h);
    }

    if (g_assets->icon.data && g_layout_icon_w > 0 && g_layout_icon_h > 0) {
        framebuffer_draw_rgba(g_assets->icon.data,
                              g_assets->icon.width, g_assets->icon.height,
                              g_layout_icon_x, g_layout_icon_y,
                              g_layout_icon_w, g_layout_icon_h);
    }

    if (g_assets->spinner.data && g_layout_spinner_size > 0) {
        bootui_draw_spinner(g_layout_spinner_x, g_layout_spinner_y);
    }
}

void bootui_set_loading_text(const char* text) {
    UNUSED(text);
}

static void rotate_point(u32 src_size, s32 sx, s32 sy, u32 angle_deg, s32* rx, s32* ry) {
    s32 cx = (s32)src_size / 2;
    s32 cy = (s32)src_size / 2;
    s32 dx = (s32)sx - cx;
    s32 dy = (s32)sy - cy;
    s32 ca = bootui_cos_deg(angle_deg);
    s32 sa = bootui_sin_deg(angle_deg);
    *rx = cx + (s32)(((s64)dx * ca - (s64)dy * sa) / FIX_ONE);
    *ry = cy + (s32)(((s64)dx * sa + (s64)dy * ca) / FIX_ONE);
}

void bootui_draw_spinner(u32 x, u32 y) {
    if (!g_assets || !g_assets->spinner.data) return;

    Framebuffer* fb = framebuffer_get();
    if (!fb || !fb->base) return;

    u8 kind = fb->pixel_format;
    u32 src_size = g_assets->spinner.width;
    if (src_size == 0) return;

    u32 dst_size = g_layout_spinner_size;
    if (dst_size == 0) return;

    u32 stride = fb->pitch / 4;
    u8* src_b = (u8*)g_assets->spinner.data;

    g_spinner_angle += SPINNER_SPEED;
    if (g_spinner_angle >= 360) g_spinner_angle -= 360;
    u32 angle = g_spinner_angle;

    for (u32 dy = 0; dy < dst_size; dy++) {
        u32 dst_row = y + dy;
        if (dst_row >= fb->height) break;

        for (u32 dx = 0; dx < dst_size; dx++) {
            u32 dst_col = x + dx;
            if (dst_col >= fb->width) break;

            /* Map destination pixel to source pixel using the rotation
             * angle, then nearest-neighbor sample the source. */
            s32 rx, ry;
            rotate_point(src_size, (s32)dx, (s32)dy, angle, &rx, &ry);
            if (rx < 0 || rx >= (s32)src_size || ry < 0 || ry >= (s32)src_size) continue;

            u32 src_idx = (u32)ry * src_size + (u32)rx;
            /* Asset RGBA byte order: byte0=R, byte1=G, byte2=B, byte3=A. */
            u8 r = src_b[src_idx * 4 + 0];
            u8 g = src_b[src_idx * 4 + 1];
            u8 b = src_b[src_idx * 4 + 2];
            u8 a = src_b[src_idx * 4 + 3];

            if (a == 0) continue;

            u32 d_idx = dst_row * stride + dst_col;
            u32 out;
            if (a == 255) {
                /* Format layout in little-endian memory:
                 *   RGBA: byte0=R, byte1=G, byte2=B, byte3=A -> (A<<24)|(B<<16)|(G<<8)|R
                 *   BGRA: byte0=B, byte1=G, byte2=R, byte3=A -> (A<<24)|(R<<16)|(G<<8)|B
                 */
                if (kind == PIXEL_FMT_RGBA) {
                    out = (0xFFu << 24) | ((u32)b << 16) | ((u32)g << 8) | r;
                } else {
                    out = (0xFFu << 24) | ((u32)r << 16) | ((u32)g << 8) | b;
                }
            } else {
                u32 bg = *(u32*)((u8*)fb->base + d_idx * 4);
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
                if (kind == PIXEL_FMT_RGBA) {
                    out = (0xFFu << 24) | ((u32)bb_out << 16) | ((u32)gg << 8) | rr;
                } else {
                    out = (0xFFu << 24) | ((u32)rr << 16) | ((u32)gg << 8) | bb_out;
                }
            }
            *(u32*)((u8*)fb->base + d_idx * 4) = out;
        }
    }
}

void bootui_animate_loading() {
    Framebuffer* fb = framebuffer_get();
    if (!fb || !g_assets || !g_assets->spinner.data) {
        for (;;) asm volatile("hlt");
    }

    u64 last_draw = 0;
    for (;;) {
        u64 now = timer_get_ticks();
        if (now != last_draw) {
            last_draw = now;
            if ((now & 0x1) == 0) {
                bootui_draw_spinner(g_layout_spinner_x, g_layout_spinner_y);
            }
        }
        while (keyboard_has_char()) {
            char c = keyboard_read_char();
            debug_log("KEY: %c\n", c);
        }
        asm volatile("hlt");
    }
}
