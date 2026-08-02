#include "cursor.h"
#include <framebuffer.h>
#include <types.h>

/* 16x16 arrow, MSB = leftmost column. Pointing to the top-left, tail
 * trailing down-right, hot-spot at (0,0). */
static const u16 g_cursor_shape[CURSOR_HEIGHT] = {
    0x8000, 0xC000, 0xE000, 0xF000,
    0xF800, 0xFC00, 0xFE00, 0xFF00,
    0xFF80, 0xFFC0, 0xFFE0, 0xFFF0,
    0xFFF8, 0xF7F0, 0xE730, 0xC260,
};

#define CURSOR_BLACK 0xFF000000u
#define CURSOR_WHITE 0xFFFFFFFFu

static u32 g_saved[CURSOR_HEIGHT][CURSOR_WIDTH];
static u32 g_cur_x = 0;
static u32 g_cur_y = 0;
static u32 g_cap_x = 0;
static u32 g_cap_y = 0;
static bool g_has_capture = false;

static bool cursor_shape_bit(u32 row, u32 col) {
    return (g_cursor_shape[row] & (0x8000u >> col)) != 0;
}

static void cursor_draw_shape(s32 ox, s32 oy, u32 color) {
    Framebuffer* fb = framebuffer_get();
    if (!fb || !fb->base) return;

    for (u32 row = 0; row < CURSOR_HEIGHT; row++) {
        for (u32 col = 0; col < CURSOR_WIDTH; col++) {
            if (!cursor_shape_bit(row, col)) continue;
            s32 px = (s32)g_cur_x + (s32)col + ox;
            s32 py = (s32)g_cur_y + (s32)row + oy;
            if (px < 0 || py < 0) continue;
            if ((u32)px >= fb->width || (u32)py >= fb->height) continue;
            framebuffer_set_pixel((u32)px, (u32)py, color);
        }
    }
}

void cursor_init(void) {
    g_cur_x = 0;
    g_cur_y = 0;
    g_cap_x = 0;
    g_cap_y = 0;
    g_has_capture = false;

    Framebuffer* fb = framebuffer_get();
    if (fb) {
        g_cur_x = fb->width / 2;
        g_cur_y = fb->height / 2;
    }
}

void cursor_set_position(u32 x, u32 y) {
    g_cur_x = x;
    g_cur_y = y;
}

void cursor_restore(void) {
    Framebuffer* fb = framebuffer_get();
    if (!fb || !fb->base || !g_has_capture) return;

    for (u32 row = 0; row < CURSOR_HEIGHT; row++) {
        for (u32 col = 0; col < CURSOR_WIDTH; col++) {
            u32 px = g_cap_x + col;
            u32 py = g_cap_y + row;
            if (px >= fb->width || py >= fb->height) continue;
            framebuffer_set_pixel(px, py, g_saved[row][col]);
        }
    }
    g_has_capture = false;
}

void cursor_draw(void) {
    Framebuffer* fb = framebuffer_get();
    if (!fb || !fb->base) return;

    g_cap_x = g_cur_x;
    g_cap_y = g_cur_y;

    for (u32 row = 0; row < CURSOR_HEIGHT; row++) {
        for (u32 col = 0; col < CURSOR_WIDTH; col++) {
            u32 px = g_cap_x + col;
            u32 py = g_cap_y + row;
            if (px >= fb->width || py >= fb->height) continue;
            g_saved[row][col] = framebuffer_get_pixel(px, py);
        }
    }
    g_has_capture = true;

    static const s32 k_offsets[8][2] = {
        { -1, -1 }, { 0, -1 }, { 1, -1 },
        { -1,  0 },           { 1,  0 },
        { -1,  1 }, { 0,  1 }, { 1,  1 },
    };
    for (u32 i = 0; i < 8; i++) {
        cursor_draw_shape(k_offsets[i][0], k_offsets[i][1], CURSOR_BLACK);
    }
    cursor_draw_shape(0, 0, CURSOR_WHITE);
}
