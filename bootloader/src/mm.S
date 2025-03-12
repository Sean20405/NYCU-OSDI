/* Set the memory to zero
   x0: pointer to the memory region
   x1: size of the memory region */
.globl memzero
memzero: 
	str     xzr, [x0], #8
	subs    x1, x1, #1
	b.ne    memzero
	ret