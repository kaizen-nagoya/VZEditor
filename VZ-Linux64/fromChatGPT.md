# VZEditor-x86_64 — Starter Kit
This document contains a ready-to-use starter kit for porting the MS-DOS VZEditor to x86-64 Linux (NASM + small C helper) and running inside Docker. Copy the files into the repo structure and follow the README to build and run.
## Repository layout

```
VZEditor-x86_64/
├─ src/
│  ├─ asm/
│  │   ├─ main.asm
│  │   └─ editor.asm
│  ├─ c/
│  │   └─ term_helper.c
├─ Makefile
├─ Dockerfile
└─ README.md
```

## Makefile

```makefile
# Makefile
ASM=nasm
ASMFLAGS=-f elf64 -g
CC=gcc
CFLAGS=-O2 -Wall -fPIC
LD=gcc

BINARY=build/vzproto

all: build $(BINARY)

build:
	mkdir -p build

# link the final binary (no PIE for simplicity)
$(BINARY): main.o editor.o term_helper.o | build
	$(LD) -no-pie -o $@ main.o editor.o term_helper.o

main.o: src/asm/main.asm
	$(ASM) $(ASMFLAGS) -o main.o src/asm/main.asm

editor.o: src/asm/editor.asm
	$(ASM) $(ASMFLAGS) -o editor.o src/asm/editor.asm

term_helper.o: src/c/term_helper.c
	$(CC) $(CFLAGS) -c -o term_helper.o src/c/term_helper.c

clean:
	rm -f *.o main.o editor.o term_helper.o build/* $(BINARY)

.PHONY: all clean build
```
## Dockerfile

```dockerfile
# Dockerfile
FROM debian:bookworm-slim

RUN apt-get update && \
    apt-get install -y --no-install-recommends build-essential nasm gcc && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /opt/vz
COPY . /opt/vz

RUN make all

ENTRYPOINT ["/opt/vz/build/vzproto"]
```

## src/c/term_helper.c
A tiny C helper to switch terminal to raw mode and restore it. Linked into the final binary.

```c
// term_helper.c - set terminal to raw mode and restore
#include <termios.h>
#include <unistd.h>

static struct termios oldt;

void term_raw_on() {
    struct termios t;
    tcgetattr(0, &oldt);
    t = oldt;
    t.c_lflag &= ~(ICANON | ECHO);
    t.c_cc[VMIN] = 1;
    t.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &t);
}

void term_raw_off() {
    tcsetattr(0, TCSANOW, &oldt);
}

// optional helper to write a buffer via libc (not used directly)
ssize_t libc_write(const void *buf, size_t count) {
    return write(1, buf, count);
}
```

## src/asm/editor.asm

Editor service routines: simple screen drawing, status bar, and cursor handling. Designed to be lightweight and easy to extend.

```nasm
; editor.asm - helper routines for simple terminal drawing and cursor state
; Assembled with NASM -f elf64

section .data
    esc_crlf db 0x1b, '[2J', 0x1b, '[H' ; clear screen + home (string)
    status_fmt db 'VZ x64 prototype  - Ln:%d Col:%d   (Press q to quit)\r\n',0

section .bss
    ; cursor row/col (0-based)
    cur_row resq 1
    cur_col resq 1

section .text
global draw_clear
global draw_status
global move_cursor_abs

; write syscall wrapper: rdi=addr, rsi=len
_write:
    mov rax, 1
    mov rdi, 1
    mov rsi, rdi
    ; overwritten by caller
    ret

; draw_clear: clears screen and moves to home
; expects no args
draw_clear:
    ; write esc_crlf
    mov rax, 1          ; sys_write
    mov rdi, 1          ; fd=stdout
    lea rsi, [rel esc_crlf]
    mov rdx, 7
    syscall
    ret

; draw_status: prints a status line at bottom (simple)
; uses cur_row and cur_col for display
draw_status:
    ; build a small status string in stack and write
    ; For simplicity we'll write a fixed status showing row/col via integer to ASCII
    push rbp
    mov rbp, rsp
    sub rsp, 64
    mov rbx, rsp

    ; get cur_row
    mov rax, [rel cur_row]
    mov rdi, rax
    ; convert rdi to decimal string at rbx+32
    lea rsi, [rbx+32]
    call int_to_dec
    ; rsi points to string
    ; build final message: "Ln:NN Col:MM\r\n"
    ; write "Ln:"
    mov rax, 1
    mov rdi, 1
    lea rsi, [rel ln_label]
    mov rdx, ln_label_len
    syscall
    ; write number
    lea rsi, [rbx+32]
    mov rdx, 20
    syscall
    ; write space
    lea rsi, [rel space_str]
    mov rdx, 1
    syscall
    ; write "Col:"
    lea rsi, [rel col_label]
    mov rdx, col_label_len
    syscall
    ; get cur_col and write
    mov rax, [rel cur_col]
    mov rdi, rax
    lea rsi, [rbx+48]
    call int_to_dec
    lea rsi, [rbx+48]
    mov rdx, 20
    syscall
    ; write CRLF
    lea rsi, [rel crlf]
    mov rdx, 2
    syscall

    add rsp, 64
    pop rbp
    ret

; move_cursor_abs: move cursor to absolute row/col (1-based expected in rdi,rsi)
; rdi=row, rsi=col
move_cursor_abs:
    push rbp
    mov rbp, rsp
    sub rsp, 64
    mov rcx, rdi
    mov rdx, rsi
    ; build escape seq: ESC [ <row> ; <col> H
    lea rbx, [rsp+16]
    mov rax, rcx
    mov rdi, rax
    lea rsi, [rbx]
    call int_to_dec_buf
    ; append ";"
    mov byte [rbx + r8], ';'
    inc r8
    mov rax, rdx
    mov rdi, rax
    lea rsi, [rbx + r8]
    call int_to_dec_buf
    ; append 'H'
    mov byte [rbx + r8], 'H'
    inc r8
    ; write ESC[
    mov rax, 1
    mov rdi, 1
    lea rsi, [rel escbrk]
    mov rdx, 2
    syscall
    ; write built buffer at rbx length r8
    lea rsi, [rbx]
    mov rdx, r8
    syscall
    add rsp, 64
    pop rbp
    ret

; small helpers and data used above
section .data
    ln_label db 'Ln:',0
    ln_label_len equ $-ln_label
    col_label db 'Col:',0
    col_label_len equ $-col_label
    space_str db ' '
    crlf db 0x0d,0x0a
    escbrk db 0x1b, '['

; integer to decimal string simple routine
; Input: rdi = unsigned integer
; Output: rsi points to buffer containing decimal string (null terminated)
; Clobbers: rax, rbx, rcx, rdx, r8
int_to_dec:
    push rbp
    mov rbp, rsp
    mov rbx, rsi
    mov rcx, 0
    mov rax, rdi
    cmp rax, 0
    jne .conv_loop
    mov byte [rbx], '0'
    mov byte [rbx+1], 0
    mov rsi, rbx
    pop rbp
    ret
.conv_loop:
    mov rdx, 0
.conv_digit:
    mov rdx, 0
    mov rbx, 10
    div rbx
    ; quotient in rax, remainder in rdx
    push rdx
    inc rcx
    cmp rax, 0
    jne .conv_loop
    ; now pop digits in reverse
    mov r8, rsi
    xor r9, r9
.pop_loop:
    pop rdx
    add rdx, '0'
    mov [r8], dl
    inc r8
    dec rcx
    jne .pop_loop
    mov byte [r8], 0
    mov rsi, rsi
    pop rbp
    ret

; int_to_dec_buf is similar but returns length in r8
; Input: rdi=number, rsi=buffer
; Output: r8 = length, buffer filled as ASCII (no null)
int_to_dec_buf:
    push rbp
    mov rbp, rsp
    mov rbx, rsi
    xor r8, r8
    mov rax, rdi
    cmp rax, 0
    jne .conv2
    mov byte [rbx], '0'
    mov r8, 1
    mov rsi, rbx
    pop rbp
    ret
.conv2:
    ; store digits on stack
    xor rcx, rcx
    .loop2:
        xor rdx, rdx
        mov rsi, 10
        div rsi
        push rdx
        inc rcx
        cmp rax, 0
        jne .loop2
    mov rdi, rbx
    mov r9, rcx
.pop2:
    pop rdx
    add dl, '0'
    mov [rdi], dl
    inc rdi
    dec rcx
    jne .pop2
    mov r8, r9
    pop rbp
    ret
```

Note: these helper routines are purposely small; you can replace them with more robust C-based formatting as desired.

## src/asm/main.asm
This is the main program. It initializes raw terminal mode, clears screen, handles input (including arrow keys and 'q' to quit), updates cursor coordinates and status line, and restores the terminal on exit.

```nasm
; main.asm - main editor loop
; NASM ELF64

global main
extern term_raw_on
extern term_raw_off
extern draw_clear
extern draw_status
extern move_cursor_abs

section .bss
    inbuf resb 8

section .data
    esc db 0x1b

section .text
main:
    ; set up stack frame (optional)
    push rbp
    mov rbp, rsp

    ; call term_raw_on
    call term_raw_on

    ; clear screen
    call draw_clear

    ; initialize cursor to 1,1 (we'll store as 1-based to print)
    mov qword [rel cur_row], 1
    mov qword [rel cur_col], 1

    ; draw status once
    call draw_status

.loop:
    ; read 1 byte
    mov rax, 0
    mov rdi, 0
    lea rsi, [rel inbuf]
    mov rdx, 1
    syscall
    cmp rax, 1
    jne .exit

    mov al, [rel inbuf]
    cmp al, 'q'
    je .exit
    cmp al, 0x1b
    jne .normal_char

    ; Escape sequence: read two more bytes if available
    ; read '['
    mov rax, 0
    mov rdi, 0
    lea rsi, [rel inbuf]
    mov rdx, 1
    syscall
    cmp rax, 1
    jne .loop
    mov al, [rel inbuf]
    cmp al, '['
    jne .loop
    ; read final char
    mov rax, 0
    mov rdi, 0
    lea rsi, [rel inbuf]
    mov rdx, 1
    syscall
    cmp rax, 1
    jne .loop
    mov al, [rel inbuf]
    cmp al, 'A'   ; up
    je .up
    cmp al, 'B'   ; down
    je .down
    cmp al, 'C'   ; right
    je .right
    cmp al, 'D'   ; left
    je .left
    jmp .loop

.up:
    ; decrement row if >1
    mov rax, [rel cur_row]
    cmp rax, 1
    jle .loop
    dec qword [rel cur_row]
    jmp .updated

.down:
    ; increment row
    inc qword [rel cur_row]
    jmp .updated

.left:
    mov rax, [rel cur_col]
    cmp rax, 1
    jle .loop
    dec qword [rel cur_col]
    jmp .updated

.right:
    inc qword [rel cur_col]
    jmp .updated

.normal_char:
    ; For now, normal characters will just be echoed
    mov rax, 1
    mov rdi, 1
    lea rsi, [rel inbuf]
    mov rdx, 1
    syscall
    jmp .loop

.updated:
    ; move cursor and update status
    ; get cur_row and cur_col (both 1-based already)
    mov rdi, [rel cur_row]
    mov rsi, [rel cur_col]
    ; move cursor
    call move_cursor_abs
    ; draw a simple status line
    call draw_status
    jmp .loop

.exit:
    ; restore terminal
    call term_raw_off
    ; clear screen before exit
    call draw_clear
    ; exit(0)
    mov rax, 60
    xor rdi, rdi
    syscall
```

## README.md

````markdown
# VZEditor-x86_64 Starter

This starter kit provides a minimal x86-64 NASM + C project that demonstrates a tiny terminal-based editor prototype. It runs in Linux and is set up to build inside Docker.

## Build & Run (docker)

```bash
# from repo root
docker build -t vzeditor .
docker run --rm -it vzeditor
````

## Build locally

```bash
make all
./build/vzproto
```

## What it does

* switches terminal to raw mode
* clears the screen
* handles arrow keys (Up/Down/Left/Right) to move an internal cursor
* echoes normal characters to the screen
* shows a small status output
* press `q` to quit (restores terminal and clears screen)

## Next steps for porting VZEditor

* implement an in-memory text buffer (gap buffer / rope)
* implement file load/save via syscalls or libc wrappers
* implement command parsing and editor commands from original VZEditor
* progressively port original ASM routines, verifying behavior against original files

```
## Notes & guidance

1. These files are intentionally simple and easy to extend. The NASM routines for formatting are minimal; for complex formatting or UTF-8 handling, prefer small C helpers and call them from assembly.
2. If you plan to port large parts of the original 16-bit code, rework data structures for 64-bit (pointers, sizes). Avoid assuming segment:offset or fixed physical addresses.
3. When you want to add features (search, replace, multi-line buffer), I can generate more robust implementations (gap buffer in assembly or C) and show how to integrate them.

If you'd like, I can now:

- generate the exact files as a downloadable archive,
- replace the minimal numeric conversion helpers with C-based formatting for clarity,
- or expand the editor to include a small gap-buffer implementation in assembly.

Tell me which of those next steps you'd like and I will produce it.

```
## from
https://github.com/kaizen-nagoya/VZEditor/blob/main/VZ-Linux64/Readme.md
