ENTRY(_start)

MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 16K
  RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 2K
}

SECTIONS
{
  .text :
  {
    KEEP(*(.text._start))
    *(.text*)
    *(.rodata*)
    . = ALIGN(4);
  } > FLASH

  .data :
  {
    *(.data*)
    . = ALIGN(4);
  } > RAM AT > FLASH

  .bss :
  {
    *(.bss*)
    . = ALIGN(4);
  } > RAM

  _stack_top = ORIGIN(RAM) + LENGTH(RAM);
}
