#include "keyboard.h"
#include <debug.h>
#include <interrupts.h>
#include <io.h>
#include <irq.h>
#include <pic.h>

#define KEYBOARD_DATA_PORT   0x60
#define KEYBOARD_STATUS_PORT 0x64
#define KEYBOARD_BUFFER_SIZE 256

static const char kb_set1_normal[128] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t',
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0, 'a', 's',
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z', 'x', 'c', 'v',
    'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static const char kb_set1_shift[128] = {
    0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b', '\t',
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0, 'A', 'S',
    'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|', 'Z', 'X', 'C', 'V',
    'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static u8 g_buffer[KEYBOARD_BUFFER_SIZE];
static u32 g_head = 0;
static u32 g_tail = 0;
static bool g_shift = false;
static bool g_caps = false;
static bool g_extended = false;

static void keyboard_enqueue(char c) {
    u32 next = (g_tail + 1) % KEYBOARD_BUFFER_SIZE;
    if (next == g_head) return;
    g_buffer[g_tail] = (u8)c;
    g_tail = next;
}

static void keyboard_irq_handler(void) {
    u8 scancode = inb(KEYBOARD_DATA_PORT);

    if (scancode == 0xE0) {
        g_extended = true;
        return;
    }
    if (scancode == 0xE1) {
        return;
    }

    u8 code = scancode & 0x7F;
    bool pressed = !(scancode & 0x80);

    if (g_extended) {
        g_extended = false;
        return;
    }

    if (code == 0x2A || code == 0x36) {
        g_shift = pressed;
        return;
    }
    if (code == 0x3A) {
        if (pressed) g_caps = !g_caps;
        return;
    }
    if (!pressed) return;
    if (code >= 128) return;

    char c = g_shift ? kb_set1_shift[code] : kb_set1_normal[code];
    if (c >= 'a' && c <= 'z' && g_caps) {
        c = (char)(c - 'a' + 'A');
    }
    if (c != 0) {
        keyboard_enqueue(c);
    }
}

void keyboard_init(void) {
    g_head = 0;
    g_tail = 0;
    g_shift = false;
    g_caps = false;
    g_extended = false;
    irq_register_handler(1, keyboard_irq_handler);
    irq_enable(1);
    debug_log("Keyboard: PS/2 ready\n");
}

bool keyboard_has_char(void) {
    return g_head != g_tail;
}

char keyboard_read_char(void) {
    if (g_head == g_tail) return 0;
    interrupts_disable();
    char c = (char)g_buffer[g_head];
    g_head = (g_head + 1) % KEYBOARD_BUFFER_SIZE;
    interrupts_enable();
    return c;
}
