#ifndef LEONELOS_MOUSE_H
#define LEONELOS_MOUSE_H

#include <types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MOUSE_LEFT   0x01
#define MOUSE_RIGHT  0x02
#define MOUSE_MIDDLE 0x04

void mouse_init(void);
void mouse_get_position(u32* x, u32* y);
void mouse_set_position(u32 x, u32 y);
u8 mouse_get_buttons(void);
bool mouse_has_delta(void);
void mouse_get_delta(i32* dx, i32* dy);
void mouse_clear_delta(void);

#ifdef __cplusplus
}
#endif

#endif
