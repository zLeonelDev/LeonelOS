#ifndef LEONELOS_COMPOSITOR_H
#define LEONELOS_COMPOSITOR_H

#include <types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WINDOW_MAX_TITLE 48
#define WINDOW_TITLEBAR_H 20
#define WINDOW_BORDER 1
#define WINDOW_CLOSE_W 16
#define WINDOW_CLOSE_H 14
#define COMPOSITOR_MAX_WINDOWS 16

typedef struct Window Window;
struct Window {
    bool active;
    u32 id;
    i32 x, y;
    u32 w, h;
    char title[WINDOW_MAX_TITLE];
    u32* backbuffer;
    bool dirty;
    bool visible;
    bool focused;
    bool dragging;
    i32 drag_off_x, drag_off_y;
    void* user;
    void (*content_draw)(Window* w);
    void (*key_handler)(Window* w, char c);
};

void compositor_init(void);
void compositor_set_background(u32 rgb);
void compositor_set_background_gradient(u32 rgb_top, u32 rgb_bottom);
u32 compositor_rgb(u32 rgb);
void compositor_set_overlay(void (*fn)(void));

Window* compositor_create_window(i32 x, i32 y, u32 w, u32 h, const char* title,
                                 void (*content_draw)(Window* w),
                                 void (*key_handler)(Window* w, char c));
void compositor_destroy_window(Window* w);
void compositor_focus(Window* w);
void compositor_raise(Window* w);
void compositor_unfocus_all(void);
void compositor_invalidate(Window* w);
void compositor_redraw(void);
Window* compositor_window_at(i32 x, i32 y);
Window* compositor_focused_window(void);
u32 compositor_window_count(void);
u32 compositor_window_index(Window* w);
Window* compositor_window_by_index(u32 i);

u32 window_content_x(Window* w);
u32 window_content_y(Window* w);
u32 window_content_w(Window* w);
u32 window_content_h(Window* w);

void window_fill(Window* w, u32 rgb);
void window_rect(Window* w, i32 x, i32 y, u32 ww, u32 hh, u32 rgb);
void window_pixel(Window* w, i32 x, i32 y, u32 rgb);
void window_text(Window* w, i32 x, i32 y, const char* s, u32 fg_rgb, u32 bg_rgb);

#ifdef __cplusplus
}
#endif

#endif
