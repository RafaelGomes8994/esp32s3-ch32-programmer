# Changelog

All notable changes to this project will be documented in this file.

## [v0.1.0] - 2026-08-14

### Hardware Verified Baseline
- **Cycle-Accurate SWIO Protocol Driver**: Implemented 1-wire timing loop (`PrecDelay(10)`) on ESP32-S3 (240 MHz Xtensa LX7).
- **DMI Protocol Engine**: Implemented 32-bit register framing, `0x5AA50400` debug handshake, and target chip ID read (`0x0713BB91`).
- **Target Memory Access**: Abstract command memory word read/write engine for peripheral and flash memory addresses.
- **3-Stage Flash Unlock**: Implemented unlock sequence for `FLASH_KEYR`, `FLASH_OBKEYR`, and `FLASH_MODEKEYR`.
- **Fast Page Erase & Programming**: 64-byte block page erase and fast page programming (`targetProgramBinary()`).
- **Memory Readback Verification**: Automated byte-for-byte readback verification (`targetVerifyBinary()`).
- **Target Control**: Added explicit target reset/run command (`targetResetRun()`).
- **Host Python Tool**: Implemented command-line flasher (`tools/flash_tool.py`) with binary size validation and error handling.
- **Physical Application Test**: Verified real bare-metal CH32V003 application driving a physical DC motor via CP6208 H-bridge motor driver.
- **Modular Refactoring**: Cleanly separated repository into Programmer Core, Target Engine, Serial CLI, CH32 Blink example, Host Tool, and Technical Documentation.

## [v0.0.1] - 2026-08-13
- Initial repository setup and pin mapping.
