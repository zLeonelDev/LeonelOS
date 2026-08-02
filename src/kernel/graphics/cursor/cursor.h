#ifndef LEONELOS_CURSOR_H
#define LEONELOS_CURSOR_H

#include <types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CURSOR_WIDTH  16
#define CURSOR_HEIGHT 16

void cursor_init(void);
void cursor_set_position(u32 x, u32 y);
void cursor_restore(void);
void cursor_draw(void);

#ifdef __cplusplus
}
#endif

#endif
