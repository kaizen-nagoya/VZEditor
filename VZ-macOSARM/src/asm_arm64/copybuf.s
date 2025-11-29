// https://github.com/kaizen-nagoya/VZEditor/blob/main/VZ-macOSARM/src/asm_arm64/copybuf.s
// Simple byte-wise copy implemented in ARM64 assembly
// void copybuf(void *dest, const void *src, uint64_t len)

    .text
    .global copybuf
    .type copybuf, @function
copybuf:
    cmp x2, #0
    beq .done
.loop:
    ldrb w3, [x1], #1
    strb w3, [x0], #1
    subs x2, x2, #1
    bne .loop
.done:
    ret
