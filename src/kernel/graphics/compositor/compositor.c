#include <compositor.h>
#include <framebuffer.h>
#include <font.h>
#include <heap.h>
#include <types.h>

#define CHROME_BORDER      0x181818
#define CHROME_TITLE_FOCUS 0x2E6DA4
#define CHROME_TITLE_BLUR  0x464646
#define CHROME_CLOSE       0xA22B2B
#define CHROME_TEXT        0xFFFFFF
#define CHROME_CONTENT_BG  0x141414

static Framebuffer* g_fb;
static Window* g_order[COMPOSITOR_MAX_WINDOWS];
static u32 g_count;
static u32 g_next_id = 1;
static bool g_dirty;
static u32* g_bg;
static u32 g_bg_count;
static void (*g_overlay)(void);

static u32 comp_pack(u8 r, u8 g, u8 b) {
    if (g_fb && g_fb->pixel_format == PIXEL_FMT_RGBA) {
        return ((u32)0xFF << 24) | ((u32)b << 16) | ((u32)g << 8) | r;
    }
    return ((u32)0xFF << 24) | ((u32)r << 16) | ((u32)g << 8) | b;
}

u32 compositor_rgb(u32 rgb) {
    return comp_pack((u8)(rgb >> 16), (u8)(rgb >> 8), (u8)rgb);
}

static u32 comp_index(Window* w) {
    for (u32 i = 0; i < g_count; i++) {
        if (g_order[i] == w) return i;
    }
    return g_count;
}

static void comp_blit(Window* w) {
    u32 stride = g_fb->pitch / 4;
    u32* dst = (u32*)g_fb->base;
    i32 sx0 = w->x, sy0 = w->y;
    i32 sx1 = sx0 + (i32)w->w, sy1 = sy0 + (i32)w->h;
    i32 ox = 0, oy = 0;
    if (sx0 < 0) { ox = -sx0; sx0 = 0; }
    if (sy0 < 0) { oy = -sy0; sy0 = 0; }
    if (sx1 > (i32)g_fb->width)  sx1 = g_fb->width;
    if (sy1 > (i32)g_fb->height) sy1 = g_fb->height;
    if (sx0 >= sx1 || sy0 >= sy1) return;
    u32 roww = (u32)(sx1 - sx0);
    for (i32 y = sy0; y < sy1; y++) {
        u32 srow = ((u32)oy + (u32)(y - sy0)) * w->w + (u32)ox;
        u32 drow = (u32)y * stride + (u32)sx0;
        for (u32 c = 0; c < roww; c++) {
            dst[drow + c] = w->backbuffer[srow + c];
        }
    }
}

void compositor_init(void) {
    g_fb = framebuffer_get();
    g_count = 0;
    g_next_id = 1;
    g_dirty = true;
    g_overlay = NULL;
    g_bg = NULL;
    if (g_fb) {
        g_bg_count = g_fb->width * g_fb->height;
        g_bg = (u32*)kmalloc((usize)g_bg_count * 4);
        if (g_bg) {
            for (u32 i = 0; i < g_bg_count; i++) g_bg[i] = 0;
        }
    }
    for (u32 i = 0; i < COMPOSITOR_MAX_WINDOWS; i++) g_order[i] = NULL;
}

void compositor_set_background(u32 rgb) {
    if (!g_bg) return;
    u32 c = compositor_rgb(rgb);
    for (u32 i = 0; i < g_bg_count; i++) g_bg[i] = c;
    g_dirty = true;
}

void compositor_set_background_gradient(u32 rgb_top, u32 rgb_bottom) {
    if (!g_bg || !g_fb || g_fb->height == 0) return;
    u32 r0 = (rgb_top >> 16) & 0xFF, g0 = (rgb_top >> 8) & 0xFF, b0 = rgb_top & 0xFF;
    u32 r1 = (rgb_bottom >> 16) & 0xFF, g1 = (rgb_bottom >> 8) & 0xFF, b1 = rgb_bottom & 0xFF;
    for (u32 y = 0; y < g_fb->height; y++) {
        s32 t = (s32)(y * 255u / g_fb->height);
        u8 r = (u8)(r0 + ((s32)r1 - (s32)r0) * t / 255);
        u8 g = (u8)(g0 + ((s32)g1 - (s32)g0) * t / 255);
        u8 b = (u8)(b0 + ((s32)b1 - (s32)b0) * t / 255);
        u32 c = comp_pack(r, g, b);
        u32* row = g_bg + (usize)y * g_fb->width;
        for (u32 x = 0; x < g_fb->width; x++) row[x] = c;
    }
    g_dirty = true;
}

void compositor_set_overlay(void (*fn)(void)) {
    g_overlay = fn;
    g_dirty = true;
}

Window* compositor_create_window(i32 x, i32 y, u32 w, u32 h, const char* title,
                                 void (*content_draw)(Window* w),
                                 void (*key_handler)(Window* w, char c)) {
    if (!g_fb || g_count >= COMPOSITOR_MAX_WINDOWS) return NULL;
    if (!title) return NULL;
    Window* wn = (Window*)kmalloc(sizeof(Window));
    if (!wn) return NULL;
    wn->backbuffer = (u32*)kmalloc((usize)w * h * 4);
    if (!wn->backbuffer) {
        kfree(wn);
        return NULL;
    }
    wn->active = true;
    wn->id = g_next_id++;
    wn->x = x;
    wn->y = y;
    wn->w = w;
    wn->h = h;
    wn->visible = true;
    wn->dirty = true;
    wn->focused = false;
    wn->dragging = false;
    wn->drag_off_x = 0;
    wn->drag_off_y = 0;
    wn->user = NULL;
    wn->content_draw = content_draw;
    wn->key_handler = key_handler;
    u32 i = 0;
    while (title[i] && i < WINDOW_MAX_TITLE - 1) {
        wn->title[i] = title[i];
        i++;
    }
    wn->title[i] = 0;
    g_order[g_count++] = wn;
    g_dirty = true;
    return wn;
}

void compositor_raise(Window* w) {
    u32 idx = comp_index(w);
    if (idx >= g_count) return;
    for (u32 i = idx; i + 1 < g_count; i++) g_order[i] = g_order[i + 1];
    g_order[g_count - 1] = w;
    g_dirty = true;
}

void compositor_focus(Window* w) {
    if (!w) return;
    Window* old = NULL;
    for (u32 i = 0; i < g_count; i++) {
        if (g_order[i]->focused) { old = g_order[i]; break; }
    }
    if (old == w) {
        compositor_raise(w);
        return;
    }
    if (old) { old->focused = false; old->dirty = true; }
    w->focused = true;
    w->dirty = true;
    compositor_raise(w);
    g_dirty = true;
}

void compositor_unfocus_all(void) {
    bool any = false;
    for (u32 i = 0; i < g_count; i++) {
        if (g_order[i]->focused) {
            g_order[i]->focused = false;
            g_order[i]->dirty = true;
            any = true;
        }
    }
    if (any) g_dirty = true;
}

void compositor_invalidate(Window* w) {
    if (!w) return;
    w->dirty = true;
    g_dirty = true;
}

void compositor_destroy_window(Window* w) {
    u32 idx = comp_index(w);
    if (idx >= g_count) return;
    kfree(w->backbuffer);
    kfree(w);
    for (u32 i = idx; i + 1 < g_count; i++) g_order[i] = g_order[i + 1];
    g_count--;
    g_dirty = true;
}

Window* compositor_window_at(i32 x, i32 y) {
    for (u32 i = g_count; i-- > 0;) {
        Window* w = g_order[i];
        if (!w->visible) continue;
        if (x >= w->x && x < w->x + (i32)w->w &&
            y >= w->y && y < w->y + (i32)w->h) {
            return w;
        }
    }
    return NULL;
}

Window* compositor_focused_window(void) {
    for (u32 i = 0; i < g_count; i++) {
        if (g_order[i]->focused) return g_order[i];
    }
    return NULL;
}

u32 compositor_window_count(void) {
    return g_count;
}

u32 compositor_window_index(Window* w) {
    return comp_index(w);
}

Window* compositor_window_by_index(u32 i) {
    return i < g_count ? g_order[i] : NULL;
}

u32 window_content_x(Window* w) {
    return WINDOW_BORDER;
}

u32 window_content_y(Window* w) {
    return WINDOW_BORDER + WINDOW_TITLEBAR_H;
}

u32 window_content_w(Window* w) {
    if (!w || w->w <= 2 * WINDOW_BORDER) return 0;
    return w->w - 2 * WINDOW_BORDER;
}

u32 window_content_h(Window* w) {
    u32 top = WINDOW_BORDER + WINDOW_TITLEBAR_H;
    if (!w || w->h <= top + WINDOW_BORDER) return 0;
    return w->h - top - WINDOW_BORDER;
}

void window_fill(Window* w, u32 rgb) {
    if (!w || !w->backbuffer) return;
    u32 c = compositor_rgb(rgb);
    for (u32 i = 0; i < w->w * w->h; i++) w->backbuffer[i] = c;
}

void window_rect(Window* w, i32 x, i32 y, u32 ww, u32 hh, u32 rgb) {
    if (!w || !w->backbuffer) return;
    u32 c = compositor_rgb(rgb);
    for (u32 row = 0; row < hh; row++) {
        i32 yy = y + (i32)row;
        if (yy < 0 || (u32)yy >= w->h) continue;
        for (u32 col = 0; col < ww; col++) {
            i32 xx = x + (i32)col;
            if (xx < 0 || (u32)xx >= w->w) continue;
            w->backbuffer[(u32)yy * w->w + (u32)xx] = c;
        }
    }
}

void window_pixel(Window* w, i32 x, i32 y, u32 rgb) {
    if (!w || !w->backbuffer) return;
    if (x < 0 || y < 0 || (u32)x >= w->w || (u32)y >= w->h) return;
    w->backbuffer[(u32)y * w->w + (u32)x] = compositor_rgb(rgb);
}

void window_text(Window* w, i32 x, i32 y, const char* s, u32 fg_rgb, u32 bg_rgb) {
    if (!w || !w->backbuffer || !s) return;
    u32 fg = compositor_rgb(fg_rgb);
    u32 bg = compositor_rgb(bg_rgb);
    for (u32 i = 0; s[i]; i++) {
        const u8* glyph = font_glyph((u8)s[i]);
        for (u32 row = 0; row < FONT_GLYPH_H; row++) {
            u8 bits = glyph[row];
            for (u32 col = 0; col < FONT_GLYPH_W; col++) {
                i32 xx = x + (i32)col;
                i32 yy = y + (i32)row;
                if (xx < 0 || yy < 0 || (u32)xx >= w->w || (u32)yy >= w->h) continue;
                w->backbuffer[(u32)yy * w->w + (u32)xx] =
                    (bits & (0x80 >> col)) ? fg : bg;
            }
        }
        x += FONT_GLYPH_W;
    }
}

static void comp_draw_chrome(Window* w) {
    u32 tw = w->w, th = w->h;
    if (tw <= 2 * WINDOW_BORDER || th <= 2 * WINDOW_BORDER) return;
    u32 bar = w->focused ? CHROME_TITLE_FOCUS : CHROME_TITLE_BLUR;

    window_rect(w, 0, 0, tw, WINDOW_BORDER, CHROME_BORDER);
    window_rect(w, 0, th - WINDOW_BORDER, tw, WINDOW_BORDER, CHROME_BORDER);
    window_rect(w, 0, 0, WINDOW_BORDER, th, CHROME_BORDER);
    window_rect(w, tw - WINDOW_BORDER, 0, WINDOW_BORDER, th, CHROME_BORDER);

    window_rect(w, WINDOW_BORDER, WINDOW_BORDER,
                tw - 2 * WINDOW_BORDER, WINDOW_TITLEBAR_H, bar);

    u32 ty = WINDOW_BORDER + (WINDOW_TITLEBAR_H - FONT_GLYPH_H) / 2;
    window_text(w, WINDOW_BORDER + 4, (i32)ty, w->title, CHROME_TEXT, bar);

    if (tw > WINDOW_BORDER + WINDOW_CLOSE_W + 4) {
        u32 cx = tw - WINDOW_BORDER - WINDOW_CLOSE_W - 2;
        u32 cy = WINDOW_BORDER + (WINDOW_TITLEBAR_H - WINDOW_CLOSE_H) / 2;
        window_rect(w, (i32)cx, (i32)cy, WINDOW_CLOSE_W, WINDOW_CLOSE_H, CHROME_CLOSE);
        u32 inset = 3;
        for (u32 i = 0; i + 2 * inset < WINDOW_CLOSE_H; i++) {
            window_pixel(w, (i32)(cx + inset + i), (i32)(cy + inset + i), CHROME_TEXT);
            window_pixel(w, (i32)(cx + inset + i), (i32)(cy + WINDOW_CLOSE_H - inset - 1 - i), CHROME_TEXT);
        }
    }
}

static void comp_refresh(Window* w) {
    window_fill(w, CHROME_CONTENT_BG);
    if (w->content_draw) w->content_draw(w);
    comp_draw_chrome(w);
    w->dirty = false;
}

void compositor_redraw(void) {
    if (!g_dirty) return;
    g_dirty = false;

    for (u32 i = 0; i < g_count; i++) {
        if (g_order[i]->dirty) comp_refresh(g_order[i]);
    }

    if (g_bg && g_fb && g_fb->base) {
        u32* dst = (u32*)g_fb->base;
        for (u32 i = 0; i < g_bg_count; i++) dst[i] = g_bg[i];
    }

    for (u32 i = 0; i < g_count; i++) {
        if (g_order[i]->visible) comp_blit(g_order[i]);
    }

    if (g_overlay) g_overlay();
}
