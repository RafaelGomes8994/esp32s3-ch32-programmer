# Project Context & Hardware Specifications

## Target Microcontroller Specifications
- **Model**: WCH CH32V003A4M6
- **Architecture**: 32-bit QingKe RISC-V V2A (`rv32ec` core)
- **Flash Memory**: 16 KB (`0x08000000` – `0x08003FFF`)
- **SRAM**: 2 KB (`0x20000000` – `0x200007FF`)
- **Package**: SOP-16
- **Single-Wire Debug Pin**: Pin 7 (`PD1` / `SWIO`)
- **Demo Output Pin**: Pin 4 (`PC4`)

## Programmer Board Specifications
- **Model**: YD-ESP32-S3 / ESP32-S3-DevKitC-1 (N8R2)
- **Microcontroller**: ESP32-S3 Dual Core 240 MHz Xtensa LX7
- **Flash / PSRAM**: 8 MB Flash, 2 MB PSRAM
- **SWIO Interface Pin**: GPIO10 (Open-drain with external 4.7k–10k pull-up to 3.3V)
- **Host Interface**: USB-UART Serial (115200 baud, 8N1)

## Verified Hardware Baseline (v0.1.0)
- Full end-to-end flash programming pipeline: `flash_tool.py` $\rightarrow$ `targetProgramBinary` $\rightarrow$ `targetVerifyBinary` $\rightarrow$ `targetResetRun`.
- Physical application execution verified on CP6208 motor driver.
