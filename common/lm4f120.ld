MEMORY
{
  FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 256K
  SRAM  (rwx) : ORIGIN = 0x20000000, LENGTH = 32K
}

/* Top of stack */
_estack = ORIGIN(SRAM) + LENGTH(SRAM);

SECTIONS
{
  .text :
  {
    KEEP(*(.isr_vector))
    *(.text*)
    *(.rodata*)
    _etext = .;
  } > FLASH

  .data : AT (_etext)
  {
    _data = .;
    *(.data*)
    _edata = .;
  } > SRAM

  .bss :
  {
    _bss = .;
    *(.bss*)
    *(COMMON)
    _ebss = .;
  } > SRAM
}
