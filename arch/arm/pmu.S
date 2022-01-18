/**
 * @file        pmu.S
 * @author      Carlos Fernandes
 * @version     1.0
 * @date        14 February, 2021
 * @brief       ARMv7-A Performance Monitor Unit Functions
 */


/* Includes ---------------------------------------------------------- */


/* Defines ----------------------------------------------------------- */


/* Macros --------------------------------------------------- */


/* Imported Functions ---------------------------------------- */


/* Function -------------------------------------------------- */
.text
.align 2

.globl pmu_int_perfcounters
.func pmu_int_perfcounters
    // void pmu_int_perfcounters(uint32_t do_reset, uint32_t enable_divider);
pmu_int_perfcounters:
    // Reset Counters?
    cmp     r0, #0x1
    moveq   r0, #0x07                       // Peform reset: reset all counters to zero
    movne   r0, #0x01
    // Enable Divider?
    cmp     r1, #0x1
    orreq   r0, r0, #0x08                   // Enable "by 64" divider for CCNT
    // Program the performance-counter control-register
    orr     r0, r0, #0x10
    mcr     p15, 0, r0, c9, c12, 0
    // Enable all counters
    ldr     r0, =0x8000000f
    mcr     p15, 0, r0, c9, c12, 1
    // Clear overflows
    mcr     p15, 0, r0, c9, c12, 3
    // Return
    bx      lr
.endfunc


.globl pmu_get_cyclecount
.func pmu_get_cyclecount
    // uint32_t pmu_get_cyclecount(void);
pmu_get_cyclecount:
    // Read CCNT Register
    mrc     p15, 0, r0, c9, c13, 0
    // Return
    bx      lr
.endfunc