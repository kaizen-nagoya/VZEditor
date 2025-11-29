// https://github.com/kaizen-nagoya/VZEditor/blob/main/VZ-macOSARM/src/main.c
#include "../include/platform.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    platform_init();
    platform_puts("VZEditor-ARM: basic shell (press q to quit)\n");

    while (1) {
        int k = platform_getkey();
        if (k == 'q' || k == 'Q') break;
        if (k >= 32 && k < 127) {
            char b[2] = {(char)k, 0};
            platform_puts(b);
        }
    }

    platform_restore();
    platform_puts("\nExiting.\n");
    return 0;
}
