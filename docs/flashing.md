# Host Flashing & Workflow Guide

This document describes the host Python command-line flashing utility (`tools/flash_tool.py`) used to detect, program, verify, and run firmware binaries on the CH32V003 target.

---

## 1. Quick Start Command

From the root of the repository, execute:

```bash
python tools/flash_tool.py --port COM10 --bin firmware/ch32_blink/blink.bin --addr 0x08000000 --reset
```

---

## 2. Command Line Arguments

| Argument | Type | Default | Required | Description |
| :--- | :--- | :--- | :--- | :--- |
| `--port` | string | `COM10` | No | Serial port corresponding to ESP32-S3 programmer (`COMx` on Windows, `/dev/ttyUSBx` on Linux/macOS) |
| `--bin` | string | — | **Yes** | Absolute or relative path to target binary file (`.bin`) |
| `--addr` | hex string | `0x08000000` | No | Target flash memory start address (Default: `0x08000000`) |
| `--reset` | flag | `False` | No | If supplied, sends explicit `reset` command after successful verification to resume target CPU execution |

---

## 3. Workflow Steps Executed by Host Tool

1. **Binary Bounds Validation**:
   * Reads target binary and validates size against the CH32V003 16 KB flash limit (`0x08000000` to `0x08003FFF`).
   * Exits with code `1` immediately if the binary exceeds bounds.

2. **Programmer Handshake & Target Detection**:
   * Opens serial port at **115200 baud**.
   * Validates target connection (`Target detect: OK`, `CH32 ID = 0x0713BB91`, `DMHARTINFO = 0x002120F4`).

3. **Fast Page Programming (`targetProgramBinary`)**:
   * Transmits hex payload in chunks.
   * Target engine unlocks flash controller and erases **only the specific 64-byte pages** touched by the binary.
   * Fast-programs 64-byte blocks into target flash.

4. **Readback Verification (`targetVerifyBinary`)**:
   * Reads flash contents byte-by-byte via abstract commands and verifies 100% data integrity (0 mismatches).

5. **Target Reset & Execution (`targetResetRun`)**:
   * If `--reset` flag is provided, sends explicit `reset` command via DMI to take the core out of debug halt and start execution.
   * If `--reset` is omitted, the target remains halted for debugging.

---

## 4. Expected Output Trace

```text
Loaded binary 'firmware/ch32_blink/blink.bin': 52 bytes
Target memory address: 0x08000000
Connecting to ESP32-S3 programmer on port COM10 (115200 baud)...
=== PROGRAMMER INITIALIZATION LOG ===
ESP32-S3 CH32 Programmer
SWIO init: OK
SWIO timing: upstream PrecDelay(10) measured ~48 CPU cycles
SWIO sync: OK (DMCFGR 0x7D = 0x5AA50401)
DMI communication: OK (DMSTATUS 0x11 = 0x004C0382)
Target detect: OK (CH32 ID = 0x0713BB91, DMHARTINFO = 0x002120F4)
Target memory read: OK (0x1FFFF704 = 0x0713BB91)

[STEP 1] Target Detection: OK

[STEP 2] Programming target binary via targetProgramBinary() (52 bytes)...
FLASH_CTLR before page erase: 0x00000200
FLASH_STATR after page erase wait: 0x00008020 (busy wait: OK)
  Progress: 52/52 bytes (at 0x08000000)
flash: OK (52 bytes processed at 0x08000000)

[STEP 2 RESULT] targetProgramBinary: OK

[STEP 3] Verifying programmed memory via targetVerifyBinary() (52 bytes)...
  Progress: 52/52 bytes (at 0x08000033)
verify: OK (52 bytes processed at 0x08000000)

[STEP 3 RESULT] targetVerifyBinary: OK

=== FLASHING & VERIFICATION SUMMARY ===
SUCCESS: Firmware successfully programmed and verified on CH32V003!

[STEP 4] Executing explicit targetResetRun()...
Reset/run: OK
[STEP 4 RESULT] targetResetRun: OK (Target execution resumed)
```
