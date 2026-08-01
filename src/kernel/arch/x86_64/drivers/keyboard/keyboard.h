#ifndef LEONELOS_KEYBOARD_H
#define LEONELOS_KEYBOARD_H

#include <types.h>

#ifdef __cplusplus
extern "C" {
#endif

void keyboard_init(void);
bool keyboard_has_char(void);
char keyboard_read_char(void);

#ifdef __cplusplus
}
#endif

#endif
