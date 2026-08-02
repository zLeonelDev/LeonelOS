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
#define ICON_TARGET_W_FP     819   /* 0.20 * 4096 — slightly smaller, drawn close to the logo */
#define SPINNER_TARGET_W_FP  320   /* ~50px on a 640-framebuffer; ~60px on a 1366x768 monitor */
#define GAP_LOGO_ICON_FP    128    /* tight: 0.03125 — logo and icon sit close together as a single block */
#define BOOTUI_TOP_MARGIN_FP 512  /* 0.125 of basis — pushes the banner up from the top of the framebuffer */
#define BOOTUI_BOTTOM_MARGIN_FP 1280 /* 0.3125 of basis — pins the spinner in the lower third of the framebuffer */

static BootAssets* g_assets = NULL;

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

    u32 gap_li = scale_fp(GAP_LOGO_ICON_FP, basis);
    u32 top_margin = scale_fp(BOOTUI_TOP_MARGIN_FP, basis);
    u32 bottom_margin = scale_fp(BOOTUI_BOTTOM_MARGIN_FP, basis);
    /* Banner (logo + icon) anchored from the top of the framebuffer. */
    g_layout_logo_y = top_margin;
    g_layout_icon_y = g_layout_logo_y + g_layout_logo_h + gap_li;
    /* Spinner anchored from the bottom of the framebuffer so it sits in
     * the lower third regardless of icon size. */
    g_layout_spinner_y = fb->height - bottom_margin - g_layout_spinner_size;
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

    /* The spinner is a procedural ring; it's drawn by bootui_animate_loading,
     * so nothing to draw here. */
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

/*
 * Compatibility stub — the procedural spinner is now driven by
 * bootui_draw_ring(), which the animator redraws every ~30 ms. This stub is
 * kept so external callers that still mention bootui_draw_spinner continue
 * to compile; it just forwards to the ring renderer at the layout size.
 */
void bootui_draw_spinner(u32 x, u32 y) {
    (void)x;
    (void)y;
    if (g_layout_spinner_size > 0) {
        bootui_draw_ring(g_layout_spinner_x, g_layout_spinner_y, g_layout_spinner_size);
    }
}

/*
 * Procedural spinner ring with an animated arc. The ring is a thin annulus
 * (inner radius ~70% of outer) drawn in cyan. A leading "arc" sector covers
 * 90 degrees and rotates with each call (angle advances by ~6 degrees per
 * draw at 100 Hz, yielding a full rotation every second).
 *
 * Inside the arc the colour is the bright cyan; outside it's a much darker
 * cyan so the ring shape stays visible but the highlight clearly indicates
 * rotation.
 */
#define SPINNER_SPEED_DEG  6
#define SPINNER_ARC_DEG    150

static u32 g_spinner_angle = 0;

void bootui_draw_ring(u32 x, u32 y, u32 size) {
    Framebuffer* fb = framebuffer_get();
    if (!fb || !fb->base || size == 0) return;

    u8 kind = fb->pixel_format;
    u32 stride = fb->pitch / 4;
    u32 cx = size / 2;
    u32 cy = size / 2;
    u32 r_out_sq = (size * size) / 4;
    u32 r_in_sq  = (size * size * 49) / 400;  /* 0.7^2 = 0.49 */
    if (r_in_sq == 0) r_in_sq = 1;

    /* Smooth brightness within the arc via a cubic ease so the leading
     * edge fades out rather than having a hard boundary. */
    for (u32 dy = 0; dy < size; dy++) {
        u32 dst_row = y + dy;
        if (dst_row >= fb->height) break;

        for (u32 dx = 0; dx < size; dx++) {
            u32 dst_col = x + dx;
            if (dst_col >= fb->width) break;

            s32 ax = (s32)dx - (s32)cx;
            s32 ay = (s32)dy - (s32)cy;
            u32 aax = (u32)(ax < 0 ? -ax : ax);
            u32 aay = (u32)(ay < 0 ? -ay : ay);
            u32 r2 = aax * aax + aay * aay;
            if (r2 >= r_out_sq || r2 < r_in_sq) continue;

            /* Convert (ax, ay) to an angle in 0..359, measured clockwise
             * from the 12-o'clock position so the math lines up with the
             * way people expect a clock-face spinner to behave. We use
             * atan2-style cross/dot with 12-o'clock = (0,-r). */
            u32 angle;
            if (aax == 0 && aay == 0) {
                angle = 0;
            } else {
                /* Map (ax, ay) to a unit direction in screen coords
                 * (y grows downward). Heading is 0 at -y, 90 at +x. */
                s32 dot = -ay;                 /* (0,-1) . (ax,ay) */
                s32 cross = ax;                /* (0,-1) x (ax,ay).z */
                if (dot > 0 && cross >= 0)
                    angle = (u32)((u64)cross * 45 / (dot + cross));
                else if (dot > 0 && cross < 0)
                    angle = 360 - (u32)((u64)(-cross) * 45 / (dot - cross));
                else if (dot <= 0 && cross > 0)
                    angle = 180 - (u32)((u64)cross * 45 / (-dot + cross));
                else if (dot <= 0 && cross < 0)
                    angle = 180 + (u32)((u64)(-cross) * 45 / (-dot - cross));
                else
                    angle = 0;
            }
            angle %= 360;

            u32 delta = (angle + 360 - g_spinner_angle) % 360;

            u8 rr, gg, bb;
            /* Sin-like brightness that peaks at the leading edge
             * (delta=0) and dips at the trailing edge (delta=180). The
             * peak-to-trough range is wide so rotation is visible. */
            s32 c = bootui_cos_deg(delta);  /* +1 at delta=0, -1 at delta=180 */
            /* -1..+1 -> 0..255 brightness, then bias back so even the dim
             * side stays visible (no full black). */
            u32 bright = (u32)((c + FIX_ONE) * 127 / (2 * FIX_ONE));
            gg = (u8)(60 + (bright * 180) / 255);
            bb = (u8)(100 + (bright * 155) / 255);
            rr = 0;

            u32 d_idx = dst_row * stride + dst_col;
            u32 out;
            if (kind == PIXEL_FMT_RGBA) {
                out = (0xFFu << 24) | ((u32)bb << 16) | ((u32)gg << 8) | rr;
            } else {
                out = (0xFFu << 24) | ((u32)rr << 16) | ((u32)gg << 8) | bb;
            }
            *(u32*)((u8*)fb->base + d_idx * 4) = out;
        }
    }

    g_spinner_angle = (g_spinner_angle + SPINNER_SPEED_DEG) % 360;
}

void bootui_animate_loading() {
    Framebuffer* fb = framebuffer_get();
    if (!fb) {
        for (;;) asm volatile("hlt");
    }

    /* Boot screen finalize: replace the initially-rendered static ring with
     * the first rotated frame, so the highlight arc appears immediately. */
    bootui_draw_ring(g_layout_spinner_x, g_layout_spinner_y, g_layout_spinner_size);

    u64 last_draw_tick = 0;
    for (;;) {
        asm volatile("hlt");
        u64 now = timer_get_ticks();
        /* Redraw every 2 timer ticks (~20 ms). With SPINNER_SPEED_DEG=6
         * per draw that gives ~300°/s — a full revolution every ~1.2 s. */
        if ((now - last_draw_tick) >= 2) {
            last_draw_tick = now;
            bootui_draw_ring(g_layout_spinner_x, g_layout_spinner_y, g_layout_spinner_size);
        }
        while (keyboard_has_char()) {
            char c = keyboard_read_char();
            debug_log("KEY: %c\n", c);
        }
    }
}
