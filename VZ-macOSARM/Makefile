// https://github.com/kaizen-nagoya/VZEditor/blob/main/VZ-macOSARM/Makefile
# Simple Makefile: build for host linux/arm64 by default.
CC = clang
CFLAGS = -O2 -g -Iinclude -Wall -Wextra
ASM = as
LDFLAGS = -lncurses

SRCS = src/main.c src/platform_term.c src/core_editor.c
OBJS = $(SRCS:.c=.o) src/asm_arm64/copybuf.o

all: vz

vz: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

src/asm_arm64/copybuf.o: src/asm_arm64/copybuf.s
	clang -c $< -o $@

clean:
	rm -f vz $(OBJS)

.PHONY: all clean
