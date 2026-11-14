# Verification & Test Procedures

This directory contains test notes and regression procedures for the ESP32-S3 CH32V003 programmer.

---

## 1. Non-Destructive Target Detection Test

Connect the programmer to the target and issue the `detect` command over USB Serial (115200 baud):

```text
Target detect: OK (CH32 ID = 0x0713BB91, DMHARTINFO = 0x002120F4)
Target memory read: OK (0x1FFFF704 = 0x0713BB91)
```

**Expected Verifications**:
* SWIO sync returns `OK` (`DMCFGR 0x7D` reads `0x5AA50401` or `0x5AA50FFF`).
* DMI communication returns `OK` (`DMSTATUS 0x11` reads `0x004C0382` or `0x004C0C82`).
* Device identification word at `0x1FFFF704` matches `0x0713BB91`.

---

## 2. End-to-End Flash Programming Test

To test binary flashing, memory readback verification, and application run:

```bash
python tools/flash_tool.py --port COM10 --bin firmware/ch32_blink/blink.bin --addr 0x08000000 --reset
```

**Expected Steps**:
1. `[STEP 1]` Target Detection: `OK`
2. `[STEP 2]` Programming: `targetProgramBinary` erases affected 64-byte pages and writes data block.
3. `[STEP 3]` Verification: `targetVerifyBinary` verifies all programmed bytes match payload.
4. `[STEP 4]` Target Reset: `targetResetRun` resumes target execution.
5. Exit code: `0`.

---

## 3. Shared-Bus Isolation & Pin Verification Checklist

Before flashing on multi-peripheral breadboards or custom PCBs:

1. **Pin Conflict Check**: Ensure `SWIO_PIN` is set to a non-USB-CDC pin (`GPIO10` default, or `GPIO21` via `-DSWIO_PIN=21`). Never assign `GPIO19`/`GPIO20`.
2. **Pull-Up Topology**: Confirm the $4.7\text{k}\Omega - 10\text{k}\Omega$ resistor is connected **between `PD1` (`SWIO`) and `3.3V`** (not in series) with common `3.3V`/`GND`.
3. **Target Isolation on `CH32 ID` / `progbuf` Failure**: If SWIO detects line transitions (`DMCFGR` responds) but fails `0x1FFFF704` readback or `unlock`, temporarily isolate the target MCU by disconnecting other shared peripherals or sibling CH32 nodes, verify `detect` returns `OK`, then flash with `tools/flash_tool.py`.
