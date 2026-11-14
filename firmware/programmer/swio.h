#pragma once

#include <Arduino.h>

// Default SWIO pin is GPIO10 (override via -DSWIO_PIN=21 in platformio.ini).
// A physical 4.7k-10k ohm pull-up between CH32V003 PD1 (SWIO) and 3.3V is required.
#ifndef SWIO_PIN
#define SWIO_PIN 10
#endif

// ESP32-S3 native USB CDC (D-/D+) occupies GPIO19 and GPIO20.
#if defined(ARDUINO_USB_CDC_ON_BOOT) && ARDUINO_USB_CDC_ON_BOOT && ((SWIO_PIN == 19) || (SWIO_PIN == 20))
#error "GPIO19/GPIO20 are reserved for ESP32-S3 native USB CDC (D-/D+). Choose GPIO10 or GPIO21."
#endif

enum class SwioBitResult : uint8_t {
  Zero,
  One,
  Timeout,
};

bool swioInit();

// CNLohr ESP32-S2 startup state: actively drive SWIO high for 5 ms.
void swioSynchronize();
void swioResetPulse();
void swioSetT1Coeff(int coeff);
int swioGetT1Coeff();

// A non-critical diagnostic measurement of the exact upstream delay loop.
uint32_t swioMeasureUpstreamT1Cycles();

void IRAM_ATTR swioDriveLow();
void IRAM_ATTR swioDriveHigh();
void IRAM_ATTR swioRelease();
void IRAM_ATTR swioWriteBit(bool value);
SwioBitResult IRAM_ATTR swioReadBit();
