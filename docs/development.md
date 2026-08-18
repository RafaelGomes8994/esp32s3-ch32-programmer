# Development & Build Guide

This guide describes how to set up the development environment, compile the programmer firmware, build the CH32 target application, and run the test suite from a fresh clone.

---

## 1. Prerequisites & Toolchains

### A. ESP32-S3 Toolchain
* **PlatformIO Core (CLI)** or **PlatformIO IDE for VSCode**
* Alternatively: **Arduino IDE 2.x** with the `esp32` board package installed.

### B. CH32V003 RISC-V Cross Compiler
* **`riscv64-unknown-elf-gcc`** or **`xpack-riscv-none-elf-gcc`** with support for `-march=rv32ec -mabi=ilp32e`.
* Available via [xPack RISC-V GCC](https://github.com/xpack-dev-tools/riscv-none-elf-gcc-xpack) or bundled inside PlatformIO packages (`toolchain-riscv`).

### C. Host Python Environment
* Python 3.8+
* `pyserial` (`pip install pyserial`)

---

## 2. Building the ESP32-S3 Programmer

From the repository root:

```bash
# Navigate to programmer directory
cd firmware/programmer

# Build the firmware image
pio run

# Upload to connected ESP32-S3 board
pio run --target upload --upload-port COM10
```

---

## 3. Building the CH32V003 Example Firmware

```bash
# Navigate to example firmware directory
cd firmware/ch32_blink

# Build blink.elf and blink.bin using Makefile
make

# Or compile directly with riscv64-unknown-elf-gcc:
riscv64-unknown-elf-gcc -march=rv32ec -mabi=ilp32e -Os -nostdlib -T ch32v003.ld startup.s main.c -o blink.elf
riscv64-unknown-elf-objcopy -O binary blink.elf blink.bin
```

---

## 4. Running the Host Flasher

```bash
# Flash the generated binary and run the application
python tools/flash_tool.py --port COM10 --bin firmware/ch32_blink/blink.bin --addr 0x08000000 --reset
```

---

## 5. Development Guidelines & Safety Rules

* **Frozen Core Protocol**: Do not alter delay loops or cycle coefficients in `swio.cpp` without scope verification. The timing loop is cycle-sensitive for 240 MHz Xtensa LX7 cores.
* **Flash Algorithm Integrity**: Always preserve the 64-byte page-level erase and fast buffer program sequence (`FLASH_CTLR_PAGE_PG`, `FLASH_CTLR_BUF_RST`).
* **Non-Destructive Testing**: Test target detection with the `detect` CLI command before issuing destructive write or erase commands.
