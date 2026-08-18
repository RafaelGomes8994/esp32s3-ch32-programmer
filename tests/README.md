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
