#include <desktop.h>
#include <compositor.h>
#include <cursor.h>
#include <framebuffer.h>
#include <font.h>
#include <keyboard.h>
#include <mouse.h>
#include <timer.h>
#include <types.h>
#include <memory/physical/physical.h>
#include <memory/heap/heap.h>
#include <bootui.h>

#define TASKBAR_H 30
#define TASKBAR_BG 0x1B1D21
#define TASKBAR_TOP 0x33363B
#define TASKBAR_BTN 0x34383E
#define TASKBAR_BTN_FOCUS 0x2E6DA4
#define TASKBAR_TEXT 0xE8ECF0
#define TASKBAR_BTN_MAX_W 170

#define BG_TOP 0x0B1B2B
#define BG_BOTTOM 0x050A12

#define SYSINFO_W 340
#define SYSINFO_H 200
#define SYSINFO_BG 0x14181E
#define SYSINFO_ACCENT 0x4FC3F7
#define SYSINFO_TEXT 0xD0D4D8
#define SYSINFO_SEP 0x2A2F36

#define TERM_W 380
#define TERM_H 260
#define TERM_CAP 1024
#define TERM_BG 0x0A0C0A
#define TERM_FG 0x66D966

typedef struct {
    char buf[TERM_CAP];
    u32 cap;
    u32 write;
    u32 cols;
} TermState;

static bool g_prev_left;
static Window* g_drag_win;

static void append_str(char* dst, const char* s) {
    if (!dst) return;
    while (*s) *dst++ = *s++;
    *dst = 0;
}

static void u32_to_dec(u32 v, char* out) {
    char tmp[12];
    u32 i = 0;
    if (v == 0) { out[0] = '0'; out[1] = 0; return; }
    while (v) { tmp[i++] = (char)('0' + v % 10); v /= 10; }
    u32 n = 0;
    while (i) out[n++] = tmp[--i];
    out[n] = 0;
}

static void append_dec(char* dst, u32 v) {
    char n[16];
    u32_to_dec(v, n);
    append_str(dst, n);
}

static void boot_animation(u32 ms) {
    u32 sx, sy, ss;
    bootui_spinner_pos(&sx, &sy, &ss);
    u64 t0 = timer_get_ticks();
    u64 target = (u64)ms / (1000 / TIMER_FREQUENCY);
    while (timer_get_ticks() - t0 < target) {
        asm volatile("hlt");
        if ((timer_get_ticks() - t0) % 2 == 0) {
            bootui_draw_ring(sx, sy, ss);
        }
    }
}

static void sysinfo_draw(Window* w) {
    window_fill(w, SYSINFO_BG);
    u32 cx = window_content_x(w);
    u32 cy = window_content_y(w);
    window_text(w, (i32)cx + 6, (i32)cy + 6, "LeonelOS v0.1.0", SYSINFO_ACCENT, SYSINFO_BG);
    window_rect(w, (i32)cx + 4, (i32)cy + 18, window_content_w(w) - 8, 1, SYSINFO_SEP);

    Framebuffer* fb = framebuffer_get();
    u32 free_kb = (u32)(get_free_memory() / 1024);
    u32 up = (u32)(timer_get_ticks() / TIMER_FREQUENCY);
    u32 mx, my;
    mouse_get_position(&mx, &my);

    char line[64];
    u32 y = cy + 28;

    line[0] = 0;
    append_str(line, "Screen: ");
    append_dec(line, fb->width);
    append_str(line, "x");
    append_dec(line, fb->height);
    window_text(w, (i32)cx + 6, (i32)y, line, SYSINFO_TEXT, SYSINFO_BG);
    y += 11;

    line[0] = 0;
    append_str(line, "Free mem: ");
    append_dec(line, free_kb);
    append_str(line, " KB");
    window_text(w, (i32)cx + 6, (i32)y, line, SYSINFO_TEXT, SYSINFO_BG);
    y += 11;

    line[0] = 0;
    append_str(line, "Uptime: ");
    append_dec(line, up);
    append_str(line, " s");
    window_text(w, (i32)cx + 6, (i32)y, line, SYSINFO_TEXT, SYSINFO_BG);
    y += 11;

    line[0] = 0;
    append_str(line, "CPU: x86-64 SMP");
    window_text(w, (i32)cx + 6, (i32)y, line, SYSINFO_TEXT, SYSINFO_BG);
    y += 11;

    line[0] = 0;
    append_str(line, "Mouse: ");
    append_dec(line, mx);
    append_str(line, ", ");
    append_dec(line, my);
    window_text(w, (i32)cx + 6, (i32)y, line, SYSINFO_TEXT, SYSINFO_BG);
}

static void term_draw(Window* w) {
    TermState* s = (TermState*)w->user;
    window_fill(w, TERM_BG);
    if (!s) return;

    u32 cw = window_content_w(w);
    u32 ch = window_content_h(w);
    u32 cols = cw / FONT_GLYPH_W;
    u32 rows = ch / FONT_GLYPH_H;
    if (!cols || !rows) return;
    s->cols = cols;
    u32 cap = cols * rows;
    if (cap > TERM_CAP) cap = TERM_CAP;
    s->cap = cap;

    u32 cx = window_content_x(w) + 2;
    u32 cy = window_content_y(w) + 2;

    u32 start = s->write > cap ? s->write - cap : 0;
    u32 cell = 0;
    for (u32 i = start; i < s->write && cell < cap; i++, cell++) {
        char c[2] = { s->buf[i % TERM_CAP], 0 };
        u32 gx = cx + (cell % cols) * FONT_GLYPH_W;
        u32 gy = cy + (cell / cols) * FONT_GLYPH_H;
        window_text(w, (i32)gx, (i32)gy, c, TERM_FG, TERM_BG);
    }
}

static void term_key(Window* w, char c) {
    TermState* s = (TermState*)w->user;
    if (!s) return;
    if (c == 0x08) {
        if (s->write > 0) s->write--;
    } else if (c == '\n' || c == '\r') {
        u32 cols = s->cols ? s->cols : 1;
        s->write = ((s->write / cols) + 1) * cols;
        if (s->write > TERM_CAP * 4) s->write = TERM_CAP * 4;
    } else if ((u8)c >= 32 && (u8)c < 127) {
        s->buf[s->write % TERM_CAP] = c;
        s->write++;
    }
    compositor_invalidate(w);
}

static void taskbar_button_rect(u32 i, u32* bx, u32* bw) {
    Window* w = compositor_window_by_index(i);
    u32 x = 4;
    for (u32 k = 0; k < i; k++) {
        Window* ww = compositor_window_by_index(k);
        if (!ww) break;
        u32 bw2 = 8 + font_string_width(ww->title);
        if (bw2 > TASKBAR_BTN_MAX_W) bw2 = TASKBAR_BTN_MAX_W;
        x += bw2 + 4;
    }
    *bx = x;
    u32 bw2 = 8 + font_string_width(w ? w->title : "");
    if (bw2 > TASKBAR_BTN_MAX_W) bw2 = TASKBAR_BTN_MAX_W;
    *bw = bw2;
}

static void taskbar_draw(void) {
    Framebuffer* fb = framebuffer_get();
    u32 y0 = fb->height - TASKBAR_H;
    framebuffer_draw_rect(0, y0, fb->width, TASKBAR_H, compositor_rgb(TASKBAR_BG));
    framebuffer_draw_rect(0, y0, fb->width, 1, compositor_rgb(TASKBAR_TOP));

    u32 count = compositor_window_count();
    for (u32 i = 0; i < count; i++) {
        Window* w = compositor_window_by_index(i);
        if (!w) break;
        u32 bx, bw;
        taskbar_button_rect(i, &bx, &bw);
        if (bx + bw >= fb->width) break;
        u32 col = w->focused ? TASKBAR_BTN_FOCUS : TASKBAR_BTN;
        framebuffer_draw_rect(bx, y0 + 5, bw, TASKBAR_H - 10, compositor_rgb(col));
        u32 tw = font_string_width(w->title);
        u32 tx = bx + (bw - tw) / 2;
        u32 ty = y0 + 5 + (TASKBAR_H - 10 - FONT_GLYPH_H) / 2;
        for (u32 k = 0; w->title[k]; k++) {
            font_draw_char(tx + k * FONT_GLYPH_W, ty, (u8)w->title[k],
                           compositor_rgb(TASKBAR_TEXT), compositor_rgb(col));
        }
    }
}

static void taskbar_click(u32 mx) {
    u32 count = compositor_window_count();
    for (u32 i = 0; i < count; i++) {
        Window* w = compositor_window_by_index(i);
        if (!w) break;
        u32 bx, bw;
        taskbar_button_rect(i, &bx, &bw);
        if (bx + bw >= framebuffer_get()->width) break;
        if (mx >= bx && mx < bx + bw) {
            compositor_focus(w);
            return;
        }
    }
}

static void mouse_press(void) {
    Framebuffer* fb = framebuffer_get();
    u32 mx, my;
    mouse_get_position(&mx, &my);

    if (my >= fb->height - TASKBAR_H) {
        taskbar_click(mx);
        return;
    }

    Window* w = compositor_window_at((i32)mx, (i32)my);
    if (!w) {
        compositor_unfocus_all();
        return;
    }

    i32 rx = (i32)mx - w->x;
    i32 ry = (i32)my - w->y;

    if (w->w > WINDOW_BORDER + WINDOW_CLOSE_W + 4) {
        i32 cx = (i32)(w->w - WINDOW_BORDER - WINDOW_CLOSE_W - 2);
        if (rx >= cx && rx < cx + (i32)WINDOW_CLOSE_W &&
            ry >= (i32)WINDOW_BORDER &&
            ry < (i32)(WINDOW_BORDER + WINDOW_TITLEBAR_H)) {
            if (g_drag_win == w) { g_drag_win = NULL; w->dragging = false; }
            compositor_destroy_window(w);
            return;
        }
    }

    bool in_titlebar = ry >= (i32)WINDOW_BORDER &&
                       ry < (i32)(WINDOW_BORDER + WINDOW_TITLEBAR_H);

    compositor_focus(w);
    if (in_titlebar) {
        w->dragging = true;
        g_drag_win = w;
        w->drag_off_x = (i32)mx - w->x;
        w->drag_off_y = (i32)my - w->y;
    }
}

static void mouse_release(void) {
    if (g_drag_win) {
        g_drag_win->dragging = false;
        g_drag_win = NULL;
    }
}

static void update_drag(void) {
    Window* w = g_drag_win;
    if (!w || !w->dragging) return;
    Framebuffer* fb = framebuffer_get();
    u32 mx, my;
    mouse_get_position(&mx, &my);

    i32 nx = (i32)mx - w->drag_off_x;
    i32 ny = (i32)my - w->drag_off_y;

    i32 min_x = -(i32)w->w + 24;
    i32 max_x = (i32)fb->width - 24;
    i32 min_y = -(i32)WINDOW_TITLEBAR_H;
    i32 max_y = (i32)fb->height - TASKBAR_H - 8;
    if (nx < min_x) nx = min_x;
    if (nx > max_x) nx = max_x;
    if (ny < min_y) ny = min_y;
    if (ny > max_y) ny = max_y;

    if (nx != w->x || ny != w->y) {
        w->x = nx;
        w->y = ny;
        compositor_invalidate(w);
    }
}

static void handle_keys(void) {
    while (keyboard_has_char()) {
        char c = keyboard_read_char();
        Window* w = compositor_focused_window();
        if (w && w->key_handler) w->key_handler(w, c);
    }
}

void desktop_run(void) {
    Framebuffer* fb = framebuffer_get();
    if (!fb) {
        for (;;) asm volatile("hlt");
    }

    boot_animation(3000);

    compositor_init();
    compositor_set_background_gradient(BG_TOP, BG_BOTTOM);
    compositor_set_overlay(taskbar_draw);

    compositor_create_window(26, 36, SYSINFO_W, SYSINFO_H, "System Info",
                             sysinfo_draw, NULL);

    g_drag_win = compositor_create_window(400, 60, TERM_W, TERM_H, "Terminal",
                                          term_draw, term_key);
    if (g_drag_win) {
        TermState* ts = (TermState*)kcalloc(1, sizeof(TermState));
        g_drag_win->user = ts;
        static const char seed[] = "Welcome to LeonelOS! Type here.";
        if (ts) {
            for (u32 i = 0; seed[i]; i++) {
                ts->buf[ts->write % TERM_CAP] = seed[i];
                ts->write++;
            }
        }
        compositor_focus(g_drag_win);
    }

    cursor_init();

    g_prev_left = false;
    g_drag_win = NULL;

    u64 last_input = 0;
    u64 last_draw = 0;
    for (;;) {
        asm volatile("hlt");
        u64 now = timer_get_ticks();

        if (now - last_input >= 1) {
            last_input = now;
            u8 btns = mouse_get_buttons();
            bool ldown = (btns & MOUSE_LEFT) != 0;
            if (ldown && !g_prev_left) mouse_press();
            else if (!ldown && g_prev_left) mouse_release();
            g_prev_left = ldown;
            if (ldown) update_drag();
            handle_keys();
        }

        if (now - last_draw >= 2) {
            last_draw = now;
            compositor_redraw();
            cursor_restore();
            u32 mx, my;
            mouse_get_position(&mx, &my);
            if (mx >= fb->width) mx = fb->width - 1;
            if (my >= fb->height) my = fb->height - 1;
            cursor_set_position(mx, my);
            cursor_draw();
        }
    }
}
