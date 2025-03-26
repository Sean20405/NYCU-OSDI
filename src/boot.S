.section ".text.boot"

.global _start
_start:  // read cpu id, stop slave cores
    mrs     x0, mpidr_el1  // read multiprocessor id register
    and     x0, x0, #0xff  // get cpu id
    cbz     x0, master     // if cpu id is 0, branch to master
    b       proc_hang      // otherwise, hang the core

master:
    // Set the stack pointer
    ldr     x0, =__stack_top
    mov     sp, x0

    // Clear the BSS section
	ldr	    x0, =__bss_begin
	ldr	    x1, =__bss_end
    sub     x1, x1, x0
    cbz     x1, main
    bl      memzero

    // Call the main function
    bl      main

proc_hang:
    wfe
    b       proc_hang