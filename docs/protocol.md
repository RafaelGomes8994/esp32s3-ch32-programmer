# Protocol Reference: SWIO, DMI & Serial CLI

This document outlines the protocol layers used by the ESP32-S3 programmer to interface with the CH32V003 microcontroller.

---

## 1. Single-Wire I/O (SWIO) Physical Layer

The CH32V003 debug system uses a 1-wire bidirectional interface. Timing is cycle-accurate based on an upstream delay multiplier loop (`upstreamPrecDelay(10)` measured at ~48 CPU cycles on 240 MHz Xtensa LX7):

### Bit Encoding
* **Logic 1 (Write)**: Pull line LOW for $1 \times t_1$, drive line HIGH for $1 \times t_1$.
* **Logic 0 (Write)**: Pull line LOW for $4 \times t_1$, drive line HIGH for $1 \times t_1$.
* **Bit Read (Sample)**: Pull line LOW for $1 \times t_1$, release line to pull-up, sample line after $2 \times t_1$, then restore driven high.

---

## 2. Debug Module Interface (DMI) Frame Layer

DMI register transactions are encoded into 32-bit frames:

* **Write Frame**:
  - `[Start Bit: 1]`
  - `[7-bit Register Address (MSB first)]`
  - `[Write Selector: 1]`
  - `[32-bit Data (MSB first)]`

* **Read Frame**:
  - `[Start Bit: 1]`
  - `[7-bit Register Address (MSB first)]`
  - `[Read Selector: 0]`
  - `[32-bit Data readback from target (MSB first)]`

### Key DMI Registers
| Register Address | Name | Description |
| :--- | :--- | :--- |
| `0x04` | `DMI_DATA0` | Abstract command data register 0 |
| `0x10` | `DMI_DMCONTROL` | Debug module control (Halt, Resume, Reset) |
| `0x11` | `DMI_DMSTATUS` | Debug module status register |
| `0x12` | `DMI_HARTINFO` | Hart information (offset `0xF4` on CH32V003) |
| `0x16` | `DMI_ABSTRACTCS` | Abstract command control & status |
| `0x17` | `DMI_COMMAND` | Abstract command execution register |
| `0x20` | `DMI_PROGBUF0` | Program buffer instruction register 0 |
| `0x7D` | `DMI_CONFIG` | Debug interface configuration register |
| `0x7E` | `DMI_SHADOW_CONFIG` | Shadow configuration register |

---

## 3. Flash Controller Register Map (Memory Mapped)

| Address | Register | Description |
| :--- | :--- | :--- |
| `0x40022004` | `FLASH_KEYR` | Flash unlock key register (Key 1: `0x45670123`, Key 2: `0xCDEF89AB`) |
| `0x40022008` | `FLASH_OBKEYR` | Option bytes key register |
| `0x4002200C` | `FLASH_STATR` | Flash status register (Bit 0 = Busy, Bit 4 = Write Protect Error) |
| `0x40022010` | `FLASH_CTLR` | Flash control register (Page Erase, Page Program, Buffer Reset) |
| `0x40022014` | `FLASH_ADDR` | Flash address register |
| `0x40022024` | `FLASH_MODEKEYR` | Flash fast-mode key register |

---

## 4. Interactive Serial CLI Protocol

The programmer listens on USB Serial at **115200 baud, 8N1**:

| Command | Arguments | Description |
| :--- | :--- | :--- |
| `detect` | None | Runs SWIO/DMI sync, reads Chip ID and flash header |
| `unlock` | None | Executes 3-stage flash controller unlock sequence |
| `erase` | `confirm` | Erases 64-byte test page at `0x08000000` |
| `testprogram` | `confirm` | Programs and verifies 64-byte deterministic test pattern |
| `flash` | `<addr> <hex>` | Erases affected 64-byte pages and fast-programs hex payload |
| `verify` | `<addr> <hex>` | Reads back and compares memory against hex payload |
| `reset` | None | Executes `targetResetRun()` to start target application |
