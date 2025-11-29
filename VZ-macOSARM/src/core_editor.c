// https://github.com/kaizen-nagoya/VZEditor/blob/main/VZ-macOSARM/src/core_editor.c (skelton)
#include "../include/platform.h"
#include <stdlib.h>
#include <string.h>

// Very small skeleton for editor core; expand gradually copying logic
// from original VZ: buffer management, cursor, file I/O, etc.

void example_copy(void *dst, const void *src, unsigned long len) {
    // call optimized assembly copy
    copybuf(dst, src, len);
}
