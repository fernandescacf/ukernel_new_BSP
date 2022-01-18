#define EARLY_UART_ADDR     (0x1C090000)

#define UART_DATA           (0x00)
#define UART_TFR            (0x18)

.text

.global early_putc
.func   early_putc
early_putc:
    ldr     r1, =EARLY_UART_ADDR
1:  ldr     r2, [r1, #UART_TFR]
    tst     r2, #(1 << 5)
    bne     1b
    str     r0, [r1, #UART_DATA]
    bx      lr
.endfunc

.global early_puts
.func   early_puts
early_puts:
    mov     r12, lr
    mov     r3, r0
1:  ldrb    r0, [r3], #1
    cmp     r0, #0x0
    bxeq    r12
    bl      early_putc
    b       1b
.endfunc