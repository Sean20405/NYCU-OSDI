.section ".text.boot"

.global _start
_start:  // read cpu id, stop slave cores
    mov     x20, x0        // x20: dtb_addr. WARNING: only used for debugging
    mrs     x0, mpidr_el1  // read multiprocessor id register
    and     x0, x0, #0xff  // get cpu id
    cbz     x0, master     // if cpu id is 0, branch to master
    b       proc_hang      // otherwise, hang the core

master:
    bl      from_el2_to_el1
    bl      set_exception_vector_table

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

memzero: 
    /** 
     * Set the memory to zero
     * x0: pointer to the memory region
     * x1: size of the memory region 
     */
	str     xzr, [x0], #8
	subs    x1, x1, #1
	b.ne    memzero
	ret

proc_hang:
    wfe
    b       proc_hang

from_el2_to_el1:
    /*
     * From ARMv8-A Reference Manual:
     *     HCR_ELx, Hypervisor Configuration Register: D8.2.32 (P1923)
     *         [31] : RW. Execution state control for EL1. 1 for aarch64
     *     SPSR_ELx, Saved Program Status Registers: D1.6.4 (P1417)
     *         [3:2]: Target exception level.
     *         [0]  : SPSel. Selects the stack pointer for the target Exception level.
     *         [8:6]: Interrupt mask bits. 1 for disable. A, I, F
     *         [9]  : Debug exception mask bit. 1 for disable
     *     ELR_ELx, Exception Link Register: D1.6.5 (P1420)
    */
    mov     x0, (1 << 31)   // EL1 uses aarch64
    msr     hcr_el2, x0

    mov     x0, 0x345       // EL1h (SPSel = 1) with interrupt disabled.
                            // 3c5 = ... 0011 1100 0101
                            // 345 = ... 0011 0100 0101 (enable IRQ)
    msr     spsr_el2, x0

    msr     elr_el2, lr
    eret                    // Exception Return. Return to EL1