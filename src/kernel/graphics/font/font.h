#ifndef LEONELOS_FONT_H
#define LEONELOS_FONT_H

#include <types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FONT_GLYPH_W 8
#define FONT_GLYPH_H 8

void font_init();
const u8* font_glyph(u8 c);
u32 font_glyph_width(u8 c);
u32 font_glyph_height(u8 c);
u32 font_string_width(const char* s);
u32 font_string_height(const char* s);
void font_draw_char(u32 x, u32 y, u8 c, u32 fg, u32 bg);
void font_draw_string(u32 x, u32 y, const char* s, u32 fg, u32 bg);

#ifdef __cplusplus
}
#endif

#endif
