#include <efi.h>
#include <efilib.h>
#include <boot.h>
#include <graphics.h>

VOID bootui_draw(FramebufferInfo* fb, BootAssets* assets) {
    if (!fb || !fb->address || !assets) return;

    graphics_clear(fb, 0x00000000);

    u32 center_x = fb->width / 2;
    u32 center_y = fb->height / 2;

    if (assets->leonelos.data && assets->leonelos.size) {
        u32 lw = assets->leonelos.width;
        u32 lh = assets->leonelos.height;
        u32 logo_x = center_x - lw / 2;
        u32 logo_y = (center_y > lh + LOGO_TOP_OFFSET) ? center_y - (lh + LOGO_TOP_OFFSET) : 0;
        graphics_draw_rgba(fb, assets->leonelos.data, lw, lh, logo_x, logo_y);
    }

    if (assets->icon.data && assets->icon.size) {
        u32 iw = assets->icon.width;
        u32 ih = assets->icon.height;
        u32 dw = (u32)((float)iw * ICON_SCALE);
        u32 dh = (u32)((float)ih * ICON_SCALE);
        u32 icon_x = center_x - dw / 2;
        u32 icon_y = (center_y > dh + ICON_TOP_OFFSET) ? center_y - (dh + ICON_TOP_OFFSET) : 0;
        graphics_draw_rgba_scaled(fb, assets->icon.data, iw, ih, icon_x, icon_y, dw, dh);
    }

    if (assets->spinner.data && assets->spinner.size) {
        u32 sx = center_x - SPINNER_SIZE / 2;
        u32 sy = center_y + SPINNER_TOP_OFFSET;
        graphics_draw_rgba(fb, assets->spinner.data, SPINNER_SIZE, SPINNER_SIZE, sx, sy);
    }
}
