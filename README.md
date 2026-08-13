# ESP32-S3 CH32V003 Programmer

An open-source, high-speed single-wire SWIO programmer and flasher for the **WCH CH32V003** microcontroller family built on the **ESP32-S3**.

---

## Key Features

* **Hardware-Verified Single-Wire Debug Protocol (SWIO)**: Cycle-accurate Xtensa LX7 bit-banging implementation.
* **Target Detection & ID Verification**: Reads CH32 Chip ID (`0x0713BB91`) and `DMHARTINFO` (`0x002120F4`).
* **Fast Page Erase & Programming**: 64-byte block page erase and fast page programming engine (`targetProgramBinary()`).
* **Verification & Target Reset**: Integrated binary verification (`targetVerifyBinary()`) and execution reset (`targetResetRun()`).
* **Cross-Platform Host Tooling**: Python host flashing utility (`tools/flash_tool.py`).

---

## Repository Structure

```
esp32s3-ch32-programmer/
├── docs/                               # Architecture, Hardware Setup, and Protocol Docs
│   ├── architecture.md
│   ├── hardware_setup.md
│   └── cli_protocol.md
├── firmware/
│   ├── ch32_blink/                     # CH32V003 Bare-Metal C Target Firmware Example
│   │   ├── ch32v003.ld
│   │   ├── main.c
│   │   ├── startup.s
│   │   ├── Makefile
│   │   └── blink.bin
│   └── environment_test/               # ESP32-S3 Programmer Firmware (PlatformIO)
│       ├── platformio.ini
│       ├── environment_test.ino        # Serial CLI Test Interface
│       ├── swio.h / swio.cpp           # Programmer Core (SWIO driver)
│       ├── dmi.h / dmi.cpp             # Programmer Core (DMI protocol)
│       └── target.h / target.cpp       # Target / Flash Engine
└── tools/
    └── flash_tool.py                   # Host Flasher Python Tool
```

---

## Quick Start

### 1. Build and Upload Programmer Firmware (ESP32-S3)
Using PlatformIO or Arduino IDE:
```bash
cd firmware/environment_test
pio run --target upload
```

### 2. Build CH32V003 Target Firmware
```bash
cd firmware/ch32_blink
make
```

### 3. Program CH32V003 Target
```bash
python tools/flash_tool.py --port COM10 --bin firmware/ch32_blink/blink.bin --addr 0x08000000 --reset
```

---

## Documentation

* [Architecture Overview](docs/architecture.md)
* [Hardware Wiring & Schematics](docs/hardware_setup.md)
* [Serial CLI Protocol Reference](docs/cli_protocol.md)
