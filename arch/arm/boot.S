#include <armv7.h>

#ifdef USE_EARLY_UART
    #include <uart.S>
#endif

/* Defines ----------------------------------------------------------- */
#ifndef PAGE_TABLE_SIZE
    #define PAGE_TABLE_SIZE (0x00004000)
#endif

#ifndef PTE_FLAGS
    // Page Entry Config: 1MB section WBWA Sharable
    #define PTE_FLAGS       (0x0001140E)
#endif

#ifndef TTB_FLAGS
    // Translation Table Config: TTB_IRGN_WBWA | TTB_S | TTB_NOS | TTB_RGN_OC_WBWA
    #define TTB_FLAGS       (0x6A)
#endif

#ifndef DOMAIN_CONFIG
    // All domains as client
    #define DOMAIN_CONFIG   (0x55555555)
#endif

#ifndef BASE_VIRTUAL_ADDR
    // Kernel base virtual address (KernelVirtualBase is set in the linker script)
    #define BASE_VIRTUAL_ADDR   (KernelVirtualBase)
#endif

#ifndef INITIAL_MAP_SIZE
    // Initial map size of 2MB
    #define INITIAL_MAP_SIZE    (0x2)
#endif

#ifdef USE_EARLY_UART
    #define UART_PTE_FLAGS  (0x00011412)
    // When Tex remap is not used: #define UART_PTE_FLAGS (0x10416)
#endif

// PRRR configuration used when SCTLR.TRE = 1
#define PRRR                (0xFF0A81A8)

// NMRR configuration used when SCTLR.TRE = 1
#define NMRR                (0x40E040E0)


/* Macros ------------------------------------------------------------ */
// Get CPU ID
.macro get_cpuid  reg 
    mrc    p15, 0, \reg, c0, c0, 5
    and    \reg, \reg, #0x03
.endm

// Get Kernel page table address
.macro get_pgt  reg
    adr     \reg, _start
    sub     \reg, \reg, #PAGE_TABLE_SIZE
.endm

.macro set_uart_pte pte, section
    ldr     \pte, =UART_PTE_FLAGS
    orr     \pte, \pte, \section, lsl #20
.endm

.macro set_pte  pte, section
    ldr     \pte, =PTE_FLAGS
    orr     \pte, \pte, \section, lsl #20
.endm

// Create startup subsection to ensure the exception table
// is placed in the begining of the kernel image
.section .text.startup
.global _start
_start:
    b       cpu_boot        // Reset            -> 0x00
    b       .               // Undefined        -> 0x04
    b       .               // Supervisor       -> 0x08
    b       .               // Pre-fetch Abort  -> 0x0c
    b       .               // Data Abort       -> 0x10
    b       .               // Hyper-visor      -> 0x14 
    b       .               // IRQ              -> 0x18
    b       .               // FIQ              -> 0x1c

cpu_boot:
    // Ensure that we are sunning in SVC Mode
    cps     #SVC_MODE

    // Get running cpu
    get_cpuid   r4
    
    // Check if cpu is the boot cpu (CPU 0)
    cmp     r4, #0
    bne     sec_cpu_boot

    // Initialize CPU
    bl      cpu_init

#ifdef CORTEX_A9
    // Enable SCU
    // TODO: call scu enable -> Should not be needed dor cortex-a7 (sunxi-h3)
#endif

    // Create setup initial page table
    bl      create_page_table

    // Enable virtual memory
    ldr     lr, _switch_data
    b       enable_mmu

sec_cpu_boot:
1:  wfi
    b       1b

cpu_init:
    // Disable Memory System (I-Cache, D-Cache, MMU)
    mrc     p15, 0, r0, c1, c0, 0
    bic     r0, r0, #SCTLR_M            // Clear M bit (MMU disable)
    bic     r0, r0, #SCTLR_D            // Clear D bit (D-cache disable)
    bic     r0, r0, #SCTLR_I            // Clear I bit (I-cache disable)
    mcr     p15, 0, r0, c1, c0, 0

    // Join SMP: Enable coherent requests to the processor
    mrc     p15, 0, r0, c1, c0, 1       // Read Auxiliary Control Register (ACTLR)
    tst     r0, #(1 << 6)               // SMP mode enabled?
    orreq   r0, r0, #(1 << 6)           // Enable SMP mode
#ifdef CORTEX_A9
    orr     r0, r0, #(1 << 0)           // Enable forward of cache and tlb operations
#endif
    mcr     p15, 0, r0, c1, c0, 1       // Write ACTLR

    // Invalidate Data Cache
    // invalidate_l1 curropts r0-r6 so store lr in r12
    mov     r12, lr
    bl      v7_invalidate_l1
    mov     lr, r12

    // Invalidate Instruction Cache + Branche Predictor
    mov     r0, #0
    mcr     p15, 0, r0, c7, c5, 0

    // Invalidate Intruction and Data TLBs
    mcr     p15, 0, r0, c8, c7, 0

    // Branch Prediction Enable
    mrc     p15, 0, r0, c1, c0, 0       // Read Control Register configuration data
    orr     r0, r0, #(0x1 << 11)        // Global BP Enable bit
    mcr     p15, 0, r0, c1, c0, 0       // Write Control Register configuration data

    // Set PRRR
    ldr     r0, =PRRR
    mcr     p15, 0, r0, c10, c2, 0
    // Set NMRR
    ldr     r0, =NMRR
    mcr     p15, 0, r0, c10, c2, 1

    dsb

    bx      lr

create_page_table:
    // Get initial kernel page table
    get_pgt r4
    // Clear whole page table
    mov    r0, r4                       // r0 = Page Table base
    mov    r3, #0
    add    r6, r0, #PAGE_TABLE_SIZE     // r6 = Page Table top
1:	str    r3, [r0], #4
    str    r3, [r0], #4
    str    r3, [r0], #4
    str    r3, [r0], #4
    teq    r0, r6
    bne    1b

    // Create identity mapping for the current address (PC)
    // Only used for switching from physical address to virtual
    // Will be removed later
    lsr     r6, pc, #20                 // start current address section
    set_pte r3, r6                      // generate page table entry
    str     r3, [r4, r6, lsl #2]        // Set identity mapping

    // Set up kernel "virtual" page table entries
    ldr     r6, =BASE_VIRTUAL_ADDR
    // Use physicall address stored in R3 (we should be in the same 1MB section)
    add     r0, r4, r6, lsr #18          // *r0 -> pgt[setion]
    str     r3, [r0], #4                // *r0++ = r3(pte)

    // Do we need to map more sections?
    ldr     r8, =INITIAL_MAP_SIZE
2:  subs    r8, r8, #0x1
    beq     3f
    // Set next page table entry
    add     r3, r3, #1 << 20            // Next PTE
    str     r3, [r0], #4                // *r0++ = r3(pte)
    b       2b
3:
#ifdef USE_EARLY_UART
    ldr     r6, =EARLY_UART_ADDR
    lsr     r6, r6, #20                 // Get uart section (MB)
    set_uart_pte r3, r6
    add     r0, r4, r6, lsl #2
    str     r3, [r0], #4 
#endif
    // return
    bx      lr

enable_mmu:
    // Set TTBCR register to use TTBR1 and TTBR0 spliting the 4Gbytes in half
    mov     r5, #0x1                    // Set N -> 0x01
    mcr     p15, 0, r5, c2, c0, 2
    // Set Domains
    ldr     r5, =DOMAIN_CONFIG
    mcr     p15, 0, r5, c3, c0, 0
    // Set TTBR1 (Translation Table Pointer)
    orr     r4, r4, #TTB_FLAGS
    mcr     p15, 0, r4, c2, c0, 1
    // Set TTBR0
    mcr     p15, 0, r4, c2, c0, 0
    // Enable MMU, Caches, Branch Prediction, Write Buffer, ...
    mrc     p15, 0, r5, c1, c0, 0
    orr     r5, r5, #(1<<0)             // Enable MMU
    bic     r5, r5, #(1<<1)             // Alignment fault checking disabled
    orr     r5, r5, #(1<<2)             // Data cache enable
    orr     r5, r5, #(1<<11)            // Branch prediction enable
    orr     r5, r5, #(1<<12)            // Instruction cache enable
    orr     r5, r5, #(1<<22)            // Unaligned data access enable
    orr     r5, r5, #(1<<28)            // TEX remap enable
    mcr     p15, 0, r5, c1, c0, 0
    isb
    dsb

    // Jump to virtual address space
    bx      lr

_mmap_switched:
    // We arrive here already executing in the address virtual space

    // Get .bss limits and stack value
    adr     r3, _switch_data + 4
    ldmia   r3!, {r5, r6, r7, r13}

    // Initialize .bss section
    mov     r0, #0x0
1:  cmp     r5, r6
    strne   r0, [r5], #4
    bne     1b

    // Set Exception Vector Base (R7)
    mcr     p15, 0, r7, c12, c0, 0

    bl      main

2:  wfi
    b       2b


/* Variables --------------------------------------------------------- */
.align  2
.type   __switch_data, %object
_switch_data:
    .long   _mmap_switched
    .long   _bss_start
    .long   _bss_end
    .long   _start
    .long   __kernel_stack
