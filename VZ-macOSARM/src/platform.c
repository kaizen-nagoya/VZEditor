// https://github.com/kaizen-nagoya/VZEditor/tree/main/VZ-macOSARM/src/platform.c
#include "../include/platform.h"
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <string.h>

static struct termios oldt;

void platform_init(void) {
    tcgetattr(STDIN_FILENO, &oldt);
    struct termios newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    newt.c_cc[VMIN] = 1;
    newt.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
}

void platform_restore(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}

int platform_getkey(void) {
    unsigned char c;
    if (read(STDIN_FILENO, &c, 1) <= 0) return -1;
    return (int)c;
}

void platform_puts(const char *s) {
    write(STDOUT_FILENO, s, strlen(s));
}
