/* Initial kernel stack size */
KERNEL_STACK_SIZE = 4k;

/* ENTRY POINT */
ENTRY(_start)

MEMORY
{
    /* RAM */
    DDR (rwx) : ORIGIN = 0x80004000, LENGTH = 0x20000000
}

/* SEGMENTS */
PHDRS
{
    text  PT_LOAD;
    data  PT_LOAD;
    stack PT_NULL;
}

/* SECTIONS */
SECTIONS
{
    /* Kernel Virtual Space Base Address */
    KernelVirtualBase = 0x80000000;

    /* Start up code and text */
    .text : ALIGN(4096) {
        __image_start = .;
        _text_start = .;
            *(.text.startup);
            *(.text);
        _text_end = .;
    } > DDR : text
    
    /* Rodara section: read only data*/
    .rodata : ALIGN(4) {
        _rodata_start = .;
            *(.rodata)
            *(.rodata.*)
        _rodata_end = .;
    } > DDR : text
    
    /* Data section: read write only data */
    .data : ALIGN(4) {
        _data_start = .;
            *(.data)
            *(.data.*)
        _data_end = .;
    } > DDR : data
    
    /* Uninitialized data section */
    .bss : ALIGN(4) {
        _bss_start = .;
            *(.bss)
            *(.bss.*)
            *(COMMON)
        _bss_end = .;
    } > DDR : data
    
    /* Kernel stack section */
    .stacks : ALIGN(8) {
        /* Initial kernel stack */
        __kernel_stack_base = .;
        . += KERNEL_STACK_SIZE;
        . = ALIGN (4096);
        __kernel_stack = .;
    } > DDR : stack

    /DISCARD/ : { *(.dynstr*) }
    /DISCARD/ : { *(.dynamic*) }
    /DISCARD/ : { *(.plt*) }
    /DISCARD/ : { *(.interp*) }
    /DISCARD/ : { *(.gnu*) }
    /DISCARD/ : { *(.note*) }
    
    __image_end = .;
}
