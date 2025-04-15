SECTIONS
{
  . = 0x80000;
  .text.boot : { KEEP(*(.text.boot)) }
  .text : { *(.text) }
  .rodata : { *(.rodata) }
  .data : { *(.data) }

  /* Record start/end of bss to fill with 0 */
  . = ALIGN(0x8);
  __bss_begin = .;
  .bss (NOLOAD): { *(.bss) }
  __bss_end = .;
  _end = .;

  . = ALIGN(0x10);
  __stack_top = . + 0x10000;  /* 64KB stack */
}
