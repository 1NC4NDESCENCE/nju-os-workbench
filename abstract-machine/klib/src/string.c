#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
    size_t i = 0;
    for (; s[i]; i++);
    return i;
}

char *strcpy(char *dst, const char *src) {
    char *save = dst;
    while ((*dst++ = *src++));
    return save;
}

char *strncpy(char *dst, const char *src, size_t n) {
    size_t i=0;
    for (; i<n && src[i]; i++) dst[i] = src[i];
    for (; i<n; i++) dst[i] = '\0';
    return dst;
}

char *strcat(char *dst, const char *src) {
    char *save = dst;
    while (*dst) dst++;
    strcpy (dst, src);
    return save;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s2 && *s1 == *s2) { s1++; s2++; }
    return *s1 - *s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    while (*s1 && *s2 && --n && *s1 == *s2) { s1++; s2++; }
    return *s1 - *s2;
}

void *memset(void *s, int c, size_t n) {
    char *ch_s = s;
    for (size_t i=0; i<n; i++) { ch_s[i] = c; }
    return s;
}

void *memmove(void *dst, const void *src, size_t n) {
    char tmp[n];
    char *ch_dst = dst;
    const char *ch_src = src; 
    for (size_t i=0; i<n; i++) tmp[i] = ch_src[i];
    for (size_t i=0; i<n; i++) ch_dst[i] = tmp[i];
    return dst;
}

void *memcpy(void *out, const void *in, size_t n) {
    char *dst = out; 
    const char *src = in;
    while (n--) *dst++ = *src++;
    return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const char *ch_s1 = s1, *ch_s2 = s2;
    while (--n && *ch_s1 == *ch_s2) { ch_s1++; ch_s2++; } 
    return *ch_s1 - *ch_s2;
}

#endif
