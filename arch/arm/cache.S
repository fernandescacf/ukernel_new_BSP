/**
 * @file        cache.S
 * @author      Carlos Fernandes
 * @version     2.0
 * @date        20 May, 2021
 * @brief       ARMv7-A Cache Driver
 */


/* Includes ---------------------------------------------------------- */


/* Defines ----------------------------------------------------------- */


/* Macros ------------------------------------------------------------ */


/* section ----------------------------------------------------------- */
.text


/* Aligment ---------------------------------------------------------- */
.align 2


/* Functions --------------------------------------------------------- */

.globl v7_invalidate_Icache_all
.func v7_invalidate_Icache_all
v7_invalidate_Icache:
    mov     r1, #0
    mcr     p15, 0, r1, c7, c5, 0   // ICIALLU - Invalidate all Instruction Cache
    bx      lr
.endfunc

.global v7_invalidate_l1
.func   v7_invalidate_l1
v7_invalidate_l1:
    mov     r0, #0
    mcr     p15, 2, r0, c0, c0, 0
    mrc     p15, 1, r0, c0, c0, 0

    movw    r1, #0x7fff
    and     r2, r1, r0, lsr #13

    movw    r1, #0x3ff

    and     r3, r1, r0, lsr #3      // NumWays - 1
    add     r2, r2, #1              // NumSets

    and     r0, r0, #0x7
    add     r0, r0, #4              // SetShift

    clz     r1, r3                  // WayShift
    add     r4, r3, #1              // NumWays
1:  sub     r2, r2, #1              // NumSets--
    mov     r3, r4                  // Temp = NumWays
2:  subs    r3, r3, #1              // Temp--
    mov     r5, r3, lsl r1
    mov     r6, r2, lsl r0
    orr     r5, r5, r6              // Reg = (Temp<<WayShift)|(NumSets<<SetShift)
    mcr     p15, 0, r5, c7, c6, 2
    bgt     2b
    cmp     r2, #0
    bgt     1b
    dsb     st
    isb
    bx     lr
.endfunc

.global v7_flush_icache_all
.func   v7_flush_icache_all
v7_flush_icache_all:
    mov     r0, #0
    mcr     p15, 0, r0, c7, c1, 0       // Invalidate I-cache inner shareable
    bx      lr
.endfunc