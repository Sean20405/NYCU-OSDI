SECTIONS
{
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
}
