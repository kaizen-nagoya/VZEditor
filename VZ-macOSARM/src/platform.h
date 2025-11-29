// https://github.com/kaizen-nagoya/VZEditor/blob/main/VZ-macOSARM/src/platform.h
#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>

void platform_init(void);
void platform_restore(void);
int platform_getkey(void);
void platform_puts(const char *s);

// Optimized copy: dest, src, len
void copybuf(void *dest, const void *src, uint64_t len);

#endif // PLATFORM_H
