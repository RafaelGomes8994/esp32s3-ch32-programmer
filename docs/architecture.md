# ESP32-S3 CH32V003 Programmer Architecture

This document details the modular software architecture of the open-source ESP32-S3 CH32V003 programmer.

---

## 1. Modular Subsystem Breakdown

The repository is structured into 6 decoupled modules:

```
esp32s3-ch32-programmer/
├── docs/                               # Module 6: Documentation
│   ├── architecture.md
│   ├── hardware.md
│   ├── flashing.md
│   ├── protocol.md
│   └── development.md
├── firmware/
│   ├── ch32_blink/                     # Module 4: CH32 Target Firmware Application
│   │   ├── ch32v003.ld
│   │   ├── main.c
│   │   ├── startup.s
│   │   ├── Makefile
│   │   └── blink.bin
│   └── programmer/                     # ESP32-S3 Programmer Firmware (PlatformIO)
│       ├── platformio.ini
│       ├── programmer.ino              # Module 3: Serial Test CLI
│       ├── swio.h / swio.cpp           # Module 1: Programmer Core (SWIO driver)
│       ├── dmi.h / dmi.cpp             # Module 1: Programmer Core (DMI protocol)
│       └── target.h / target.cpp       # Module 2: Target / Flash Engine
├── hardware/
│   └── wiring/
│       └── README.md                   # Visual wiring and schematic reference
├── tests/
│   └── README.md                       # Verification & regression test suite notes
├── tools/                              # Module 5: Host Python CLI Tool
│   └── flash_tool.py
├── LICENSE                             # Open Source License & Attributions
├── CHANGELOG.md                        # Version Release History
├── PROJECT_CONTEXT.md                  # Project Context & Hardware Specs
└── README.md                           # Master Project Documentation
```

---

## 2. Module Responsibilities

### Module 1: Programmer Core (`swio.*` & `dmi.*`)
* **Single-Wire I/O (`swio`)**: High-speed bit-banging protocol driver running on Xtensa LX7 (ESP32-S3) using precise cycle delays (`upstreamPrecDelay(10)` measured at ~48 CPU cycles). Manages open-drain drive high, drive low, line release, and 1-wire bit sampling.
* **Debug Module Interface (`dmi`)**: Handles 32-bit RISC-V debug register frames (`DMI_CONFIG 0x7D`, `DMI_SHADOW_CONFIG 0x7E`, `DMI_DMCONTROL 0x10`, `DMI_COMMAND 0x17`). Executes single-wire initialization handshakes (`0x5AA50400`).

---

### Module 2: Target / Flash Engine (`target.*`)
* **Chip Detection**: Reads `DMHARTINFO` (`0x002120F4`) and device identification memory at `0x1FFFF704` (`0x0713BB91`).
* **Flash Unlock**: Sends 3-stage unlock key sequences (`FLASH_KEYR`, `FLASH_OBKEYR`, `FLASH_MODEKEYR`) using keys `0x45670123` and `0xCDEF89AB`.
* **Fast Page Erase**: Performs targeted 64-byte page erases (`FLASH_CTLR_PAGE_ER`).
* **Fast 64-Byte Page Programming**: Programs 64-byte fast page buffers (`FLASH_CTLR_PAGE_PG`) and verifies readback.
* **Target Control**: Provides explicit `targetResetRun()` operation to resume execution.

---

### Module 3: Serial Test CLI (`programmer.ino`)
* Interactive line-buffered command parser running over ESP32-S3 USB Serial (115200 baud).
* Supports commands: `detect`, `unlock`, `erase confirm`, `testprogram confirm`, `flash <addr> <hex>`, `verify <addr> <hex>`, `reset`.

---

### Module 4: CH32 Target Firmware Application (`firmware/ch32_blink/`)
* Bare-metal C target application targeting CH32V003 (QingKe RISC-V V2A core).
* Configures PC4 (Physical Pin 4 on SOP-16) as 10 MHz Push-Pull Output.
* Includes both standard 1 Hz blink loop and diagnostic permanent-HIGH mode (`DIAGNOSTIC_PERMANENT_HIGH`).
* Cross-compiles using `riscv64-unknown-elf-gcc` or `riscv-none-elf-gcc` via `Makefile`.

---

### Module 5: Host Python Tool (`tools/flash_tool.py`)
* Cross-platform Python CLI utility utilizing PySerial.
* Automated workflow: connects to ESP32-S3, verifies CH32 chip presence, executes `targetProgramBinary()`, verifies payload via `targetVerifyBinary()`, and triggers `targetResetRun()`.

---

### Module 6: Documentation (`docs/`)
* Comprehensive technical documentation covering system architecture, hardware schematics/wiring, CLI command formats, and API references.
