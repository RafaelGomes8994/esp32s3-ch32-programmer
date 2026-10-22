ENTRY(_start)

MEMORY
{
  FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 16K
  RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 2K
}

SECTIONS
{
  .text :
  {
    . = ALIGN(4);
    KEEP(*(.text._start))
    *(.text*)
    *(.rodata*)
    *(.srodata*)
    . = ALIGN(4);
    _etext = .;
  } > FLASH AT > FLASH

  .dalign :
  {
    . = ALIGN(4);
    _data_vma = .;
  } > RAM AT > FLASH

  .dlalign :
  {
    . = ALIGN(4);
    _data_lma = .;
  } > FLASH AT > FLASH

  .data :
  {
    . = ALIGN(4);
    *(.data*)
    *(.sdata*)
    . = ALIGN(4);
    _edata = .;
  } > RAM AT > FLASH

  .bss :
  {
    . = ALIGN(4);
    _sbss = .;
    *(.bss*)
    *(.sbss*)
    *(COMMON*)
    . = ALIGN(4);
    _ebss = .;
  } > RAM

  _stack_top = ORIGIN(RAM) + LENGTH(RAM);
}
