#include <string.h>

void* memset(void* dest, int c, usize n) {
    u8* dst = (u8*)dest;
    for (usize i = 0; i < n; i++) {
        dst[i] = (u8)c;
    }
    return dest;
}

void* memcpy(void* dest, const void* src, usize n) {
    u8* dst = (u8*)dest;
    const u8* srcc = (const u8*)src;
    for (usize i = 0; i < n; i++) {
        dst[i] = srcc[i];
    }
    return dest;
}

usize strlen(const char* str) {
    usize len = 0;
    while (str[len]) len++;
    return len;
}

int strcmp(const char* a, const char* b) {
    while (*a && *b && *a == *b) {
        a++;
        b++;
    }
    return (u8)*a - (u8)*b;
}

int strncmp(const char* a, const char* b, usize n) {
    while (n > 0 && *a && *b && *a == *b) {
        a++;
        b++;
        n--;
    }
    if (n == 0) return 0;
    return (u8)*a - (u8)*b;
}

char* strcpy(char* dest, const char* src) {
    char* d = dest;
    while ((*d++ = *src++));
    return dest;
}

char* strncpy(char* dest, const char* src, usize n) {
    char* d = dest;
    usize i;
    for (i = 0; i < n && src[i]; i++) {
        d[i] = src[i];
    }
    for (; i < n; i++) {
        d[i] = '\0';
    }
    return dest;
}

char* strcat(char* dest, const char* src) {
    char* d = dest + strlen(dest);
    strcpy(d, src);
    return dest;
}

char* strchr(const char* str, int c) {
    while (*str) {
        if (*str == (char)c) return (char*)str;
        str++;
    }
    return NULL;
}

char* strrchr(const char* str, int c) {
    const char* last = NULL;
    while (*str) {
        if (*str == (char)c) last = str;
        str++;
    }
    return (char*)last;
}

void* memmove(void* dest, const void* src, usize n) {
    u8* dst = (u8*)dest;
    const u8* srcc = (const u8*)src;
    if (dst < srcc) {
        for (usize i = 0; i < n; i++) {
            dst[i] = srcc[i];
        }
    } else {
        for (usize i = n; i > 0; i--) {
            dst[i - 1] = srcc[i - 1];
        }
    }
    return dest;
}