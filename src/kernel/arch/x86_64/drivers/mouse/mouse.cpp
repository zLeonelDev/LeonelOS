#include "mouse.h"
#include <debug.h>
#include <interrupts.h>
#include <io.h>
#include <irq.h>
#include <pic.h>

#define MOUSE_DATA_PORT    0x60
#define MOUSE_STATUS_PORT  0x64

#define PS2_STATUS_OUT_BUF 0x01
#define PS2_STATUS_IN_BUF  0x02

#define MOUSE_ACK          0xFA

#define MOUSE_CMD_ENABLE_AUX 0xA8
#define MOUSE_CMD_READ_CFG   0x20
#define MOUSE_CMD_WRITE_CFG  0x60
#define MOUSE_CMD_SEND_AUX   0xD4
#define MOUSE_CMD_SET_SAMPLE 0xF3
#define MOUSE_CMD_ENABLE     0xF4

#define MOUSE_SAMPLE_60      60

#define MOUSE_SCALE 2

static i32 g_x = 0;
static i32 g_y = 0;
static i32 g_dx = 0;
static i32 g_dy = 0;
static u8 g_buttons = 0;

static bool g_waiting_ack = false;
static u8 g_packet[3];
static u8 g_packet_idx = 0;

static void mouse_wait_write(void) {
    for (u32 i = 0; i < 10000; i++) {
        if (!(inb(MOUSE_STATUS_PORT) & PS2_STATUS_IN_BUF)) return;
    }
}

static void mouse_wait_read(void) {
    for (u32 i = 0; i < 10000; i++) {
        if (inb(MOUSE_STATUS_PORT) & PS2_STATUS_OUT_BUF) return;
    }
}

static void mouse_send_to_aux(u8 cmd) {
    mouse_wait_write();
    outb(MOUSE_STATUS_PORT, MOUSE_CMD_SEND_AUX);
    mouse_wait_write();
    outb(MOUSE_DATA_PORT, cmd);
    mouse_wait_read();
    inb(MOUSE_DATA_PORT);
}

static void mouse_irq_handler(void) {
    if (!(inb(MOUSE_STATUS_PORT) & PS2_STATUS_OUT_BUF)) return;

    u8 data = inb(MOUSE_DATA_PORT);

    if (g_waiting_ack) {
        g_waiting_ack = false;
        return;
    }

    g_packet[g_packet_idx] = data;
    g_packet_idx++;

    if (g_packet_idx < 3) return;

    u8 b0 = g_packet[0];
    if (b0 & 0x08) {
        i32 dx = g_packet[1];
        i32 dy = g_packet[2];
        if (b0 & 0x10) dx -= 256;
        if (b0 & 0x20) dy -= 256;

        dx *= MOUSE_SCALE;
        dy *= MOUSE_SCALE;

        g_buttons = (u8)(b0 & 0x07);
        g_dx += dx;
        g_dy += -dy;
        g_x += dx;
        g_y += -dy;
    }

    g_packet_idx = 0;
}

void mouse_init(void) {
    g_x = 0;
    g_y = 0;
    g_dx = 0;
    g_dy = 0;
    g_buttons = 0;
    g_waiting_ack = false;
    g_packet_idx = 0;

    mouse_wait_write();
    outb(MOUSE_STATUS_PORT, MOUSE_CMD_ENABLE_AUX);

    mouse_wait_write();
    outb(MOUSE_STATUS_PORT, MOUSE_CMD_READ_CFG);
    mouse_wait_read();
    u8 cfg = inb(MOUSE_DATA_PORT);
    cfg |= 0x02;
    mouse_wait_write();
    outb(MOUSE_STATUS_PORT, MOUSE_CMD_WRITE_CFG);
    mouse_wait_write();
    outb(MOUSE_DATA_PORT, cfg);

    mouse_send_to_aux(MOUSE_CMD_SET_SAMPLE);
    mouse_send_to_aux(MOUSE_SAMPLE_60);
    mouse_send_to_aux(MOUSE_CMD_ENABLE);

    irq_register_handler(12, mouse_irq_handler);
    irq_enable(12);
    debug_log("Mouse: PS/2 ready\n");
}

void mouse_get_position(u32* x, u32* y) {
    interrupts_disable();
    *x = g_x < 0 ? 0 : (g_x > 65535 ? 65535 : (u32)g_x);
    *y = g_y < 0 ? 0 : (g_y > 65535 ? 65535 : (u32)g_y);
    interrupts_enable();
}

void mouse_set_position(u32 x, u32 y) {
    interrupts_disable();
    g_x = (i32)x;
    g_y = (i32)y;
    interrupts_enable();
}

u8 mouse_get_buttons(void) {
    return g_buttons;
}

bool mouse_has_delta(void) {
    return g_dx != 0 || g_dy != 0;
}

void mouse_get_delta(i32* dx, i32* dy) {
    interrupts_disable();
    *dx = g_dx;
    *dy = g_dy;
    interrupts_enable();
}

void mouse_clear_delta(void) {
    interrupts_disable();
    g_dx = 0;
    g_dy = 0;
    interrupts_enable();
}
