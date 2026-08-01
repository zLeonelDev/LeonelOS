#ifndef LEONELOS_API_H
#define LEONELOS_API_H

#include <types.h>

typedef struct {
    void* (*allocate)(usize size);
    void  (*deallocate)(void* ptr);
    void* (*allocate_aligned)(usize size, usize alignment);
} MemoryAPI;

typedef struct {
    s32 (*open)(const char* path, u32 flags);
    s32 (*close)(s32 fd);
    s64 (*read)(s32 fd, void* buffer, usize size);
    s64 (*write)(s32 fd, const void* buffer, usize size);
    s32 (*mkdir)(const char* path);
    s32 (*unlink)(const char* path);
} FileAPI;

typedef struct {
    s32 (*create_window)(u32 width, u32 height, const char* title);
    s32 (*destroy_window)(s32 window_id);
    s32 (*draw_rect)(s32 window_id, u32 x, u32 y, u32 w, u32 h, u32 color);
    s32 (*draw_text)(s32 window_id, u32 x, u32 y, const char* text);
    s32 (*draw_image)(s32 window_id, u32 x, u32 y, const void* data, u32 w, u32 h);
    s32 (*present)(s32 window_id);
} GraphicsAPI;

typedef struct {
    u32 (*get_keys)();
    u32 (*get_mouse_buttons)();
    s32 (*get_mouse_x)();
    s32 (*get_mouse_y)();
} InputAPI;

extern MemoryAPI Memory;
extern FileAPI File;
extern GraphicsAPI Graphics;
extern InputAPI Input;

s32 api_init();

#endif