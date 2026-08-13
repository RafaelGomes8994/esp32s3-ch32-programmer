/*
 * CH32V003 Firmware Application - PC4 Output Example / Motor Driver Test
 * 
 * Hardware Target: CH32V003A4M6 (SOP-16)
 * - Pin 4: PC4 (Output Push-Pull)
 * - Pin 7: SWIO (Single-Wire Debug Interface)
 * - Pin 14: VSS (GND)
 * - Pin 15: VDD (3.3V)
 */

#include <stdint.h>

#define RCC_BASE        0x40021000UL
#define RCC_APB2PCENR   *(volatile uint32_t*)(RCC_BASE + 0x18UL)
#define RCC_IOPCEN      (1UL << 4)

#define GPIOC_BASE      0x40011000UL
#define GPIOC_CFGLR     *(volatile uint32_t*)(GPIOC_BASE + 0x00UL)
#define GPIOC_BSHR      *(volatile uint32_t*)(GPIOC_BASE + 0x10UL)
#define GPIOC_BCR       *(volatile uint32_t*)(GPIOC_BASE + 0x14UL)

#define LED_PIN         4

#ifndef DIAGNOSTIC_PERMANENT_HIGH
#define DIAGNOSTIC_PERMANENT_HIGH 1
#endif

static void delay_cycles(volatile uint32_t count) {
    while (count--) {
        __asm__ volatile ("nop");
    }
}

int main(void) {
    // 1. Enable GPIOC peripheral clock
    RCC_APB2PCENR |= RCC_IOPCEN;

    // 2. Configure PC4 as General Purpose Output Push-Pull 10MHz (0x1 in bits [19:16])
    uint32_t cfg = GPIOC_CFGLR;
    cfg &= ~(0xFUL << (LED_PIN * 4));
    cfg |= (0x1UL << (LED_PIN * 4));
    GPIOC_CFGLR = cfg;

#if DIAGNOSTIC_PERMANENT_HIGH
    // Diagnostic Mode: Hold PC4 permanently HIGH (3.3V)
    GPIOC_BSHR = (1UL << LED_PIN);

    while (1) {
    }
#else
    // Standard Blink Loop: ~500 ms delay per toggle at default ~8MHz HSI clock
    while (1) {
        GPIOC_BSHR = (1UL << LED_PIN);       // PC4 HIGH (3.3V)
        delay_cycles(250000UL);
        GPIOC_BCR = (1UL << LED_PIN);        // PC4 LOW (0.0V)
        delay_cycles(250000UL);
    }
#endif

    return 0;
}
