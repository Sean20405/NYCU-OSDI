.section ".text.boot"

.global _start
_start:  // read cpu id, stop slave cores
    mov     x20, x0        // x20: dtb_addr
    mrs     x0, mpidr_el1  // read multiprocessor id register
    and     x0, x0, #0xff  // get cpu id
    cbz     x0, master     // if cpu id is 0, branch to master
    b       proc_hang      // otherwise, hang the core

master:
    adr     x4, _start          // get the current address

    ldr     x1, =__boot_begin   // Current bootloader address
    ldr     x2, =__boot_end
    sub     x2, x2, x1          // bootloader size

    // check if the bootloader is already relocated
    cmp     x4, x1              
    b.eq    already_relocated

    mov     x0, x4              // x0: src, x1: dst, x2: size

copy_bootloader:
    cbz     x2, finish_copy     // If size is 0, finish copying
    ldr     x5, [x0], #8
    str     x5, [x1], #8
    sub     x2, x2, #8
    b       copy_bootloader

finish_copy:
    // Calculate the relative offset to jump to the new location
    ldr     x0, =__boot_begin   // new bootloader address
    sub     x6, x0, x4          // x6: offset
    
    adr     x5, setting
    add     x5, x5, x6          // Calculate the new address of the following code
    br      x5

already_relocated:
    b       setting

setting:
    // Set the stack pointer
    ldr     x0, =__stack_top
    mov     sp, x0

    // Clear the BSS section
    ldr     x0, =__bss_begin
    ldr     x1, =__bss_end
    sub     x1, x1, x0
    cbz     x1, main
    bl      memzero

    // Call the main function
    bl      main

memzero: 
    /* Set the memory to zero
       x0: pointer to the memory region
       x1: size of the memory region */
	str     xzr, [x0], #8
	subs    x1, x1, #1
	b.ne    memzero
	ret

proc_hang:
    wfe
    b       proc_hang