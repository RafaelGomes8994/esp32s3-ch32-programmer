# Changelog

All notable changes to this project will be documented in this file.

## [v0.2.0] - 2026-11-13

### ESP-IDF 5.x Stabilization, SWIO Auto-Calibration & CLI Refinement
- **Pinned PlatformIO Core Stack**: Locked `platform = espressif32 @ 6.13.0` in `firmware/programmer/platformio.ini` for deterministic ESP-IDF 5.x / Arduino-ESP32 compilation.
- **IO_MUX & Simple GPIO Bidirectional Sync**: Explicitly enabled `GPIO.func_out_sel_cfg[SWIO_PIN].val = 0x100` and `PIN_INPUT_ENABLE(GPIO_PIN_MUX_REG[SWIO_PIN])` across init, synchronize, and reset pulses for reliable turnaround reads under ESP-IDF 5.x.
- **Configurable SWIO Pin & USB-CDC Guard**: Allowed compile-time `-DSWIO_PIN` override (default `GPIO10`, recommended alternate `GPIO21`) with a `#error` guard preventing accidental assignment to native USB-CDC pins `GPIO19`/`GPIO20`.
- **Dynamic `t1coeff` Auto-Calibration & Reset Recovery**: Added adaptive `t1coeff` sweep and 20 ms SWIO reset pulse fallback in `dmiSynchronize()` alongside `0x5AA50401`/`0x5AA50FFF` handling.
- **Host CLI Auto-Port Detection & Chunked Retries**: Enhanced `tools/flash_tool.py` with cross-platform USB serial port probing, buffered chunk retries, and cleaner verification progress logs.
- **CH32V003 I2C Coprocessor Example**: Added `firmware/ch32_i2c_coprocessor/` bare-metal CH32V003 I2C slave (`0x30`) + software I2C master bridge reference.
- **Clean Programmer CLI**: Removed project-specific `i2cscan`/`diag` debug handlers and `<Wire.h>` coupling from `firmware/programmer/programmer.ino`; standardized startup help text and active `SWIO_PIN` banner.
- **Hardware & Isolation Documentation**: Documented parallel `PD1 <-> 3.3V` pull-up wiring (not in series), common `3.3V`/`GND` requirements, and shared-bus target isolation troubleshooting in `docs/hardware.md` and `tests/README.md`.

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
