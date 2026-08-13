# Serial CLI & Host Communication Protocol

The ESP32-S3 CH32 Programmer communicates with host scripts (`tools/flash_tool.py`) or interactive serial terminals over USB Serial at **115200 baud**.

---

## Serial Command Format

Commands are sent as ASCII strings terminated by `\n` (newline).

### 1. `detect`
* **Description**: Queries the single-wire SWIO interface and reads target chip ID.
* **Success Output**:
  ```text
  Target detect: OK (CH32 ID = 0x0713BB91, DMHARTINFO = 0x002120F4)
  Target memory read: OK (0x1FFFF704 = 0x0713BB91)
  ```
* **Failure Output**:
  ```text
  Target detect: FAILED (unexpected target identity)
  ```

---

### 2. `unlock`
* **Description**: Unlocks CH32V003 flash controller using 3-stage KEYR, OBKEYR, MODEKEYR sequences.
* **Output**:
  ```text
  FLASH_CTLR before unlock: 0x00000080
  FLASH_CTLR after unlock:  0x00000000
  Unlock: OK
  ```

---

### 3. `erase confirm`
* **Description**: Erases 64-byte test page at `0x08000000` and verifies all bytes read back `0xFFFFFFFF`.
* **Output**:
  ```text
  Final Erase Verification: OK (All 64 bytes verified 0xFFFFFFFF)
  ```

---

### 4. `flash <hex-address> <hex-data-string>`
* **Description**: Erases affected 64-byte pages, fast-programs data block, and verifies readback via `targetProgramBinary()`.
* **Parameters**:
  * `<hex-address>`: Flash memory address (e.g. `08000000`)
  * `<hex-data-string>`: Hex-encoded byte payload (even number of characters)
* **Output**:
  ```text
  Progress: 64/92 bytes (at 0x08000000)
  flash: OK (92 bytes processed at 0x08000000)
  ```

---

### 5. `verify <hex-address> <hex-data-string>`
* **Description**: Reads flash memory byte-by-byte via abstract commands and compares against expected payload via `targetVerifyBinary()`.
* **Output**:
  ```text
  Progress: 64/92 bytes (at 0x0800003F)
  verify: OK (92 bytes processed at 0x08000000)
  ```

---

### 6. `reset`
* **Description**: Triggers explicit RISC-V debug module target reset/run sequence via `DMI_DMCONTROL` register write (`targetResetRun()`).
* **Output**:
  ```text
  Reset/run: OK
  ```

---

## Python Tool Usage Example

```bash
python tools/flash_tool.py --port COM10 --bin firmware/ch32_blink/blink.bin --addr 0x08000000 --reset
```
