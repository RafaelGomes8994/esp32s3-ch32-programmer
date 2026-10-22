.section .text._start
.global _start
.type _start, @function

_start:
    /* Load stack pointer with absolute address 0x20000800 (immune to PC aliasing 0x00000000 vs 0x08000000) */
    lui sp, 0x20001
    addi sp, sp, -2048

    /* Copy .data from Flash (_data_lma) to RAM (_data_vma .. _edata) */
    la a0, _data_lma
    la a1, _data_vma
    la a2, _edata
    bgeu a1, a2, 2f
1:
    lw a3, 0(a0)
    sw a3, 0(a1)
    addi a0, a0, 4
    addi a1, a1, 4
    bltu a1, a2, 1b
2:

    /* Clear .bss (_sbss .. _ebss) */
    la a0, _sbss
    la a1, _ebss
    bgeu a0, a1, 4f
3:
    sw zero, 0(a0)
    addi a0, a0, 4
    bltu a0, a1, 3b
4:

    j main
