.section .text._start
.global _start
.type _start, @function

_start:
    /* Initialize stack pointer to top of 2KB RAM (0x20000800) */
    la sp, _stack_top
    j main
