# Hardware Guide & Physical Pinout Reference

This document provides complete hardware pinouts, power rail architecture, and schematic details for interfacing the **ESP32-S3 Programmer**, the **CH32V003A4M6 (SOP-16)** microcontroller, and the **CP6208** motor driver test load.

---

## 1. ESP32-S3 Programmer Pinout

| Pin Name | Function | Connection Target | Electrical Characteristic |
| :--- | :--- | :--- | :--- |
| **GPIO10** *(or GPIO21 via `-DSWIO_PIN=21`)* | SWIO Data Line | CH32V003 Pin 7 (`PD1`) | Open-drain I/O (`IO_MUX` input enabled), requires $4.7\text{k}\Omega - 10\text{k}\Omega$ pull-up to 3.3V |
| **3V3** | 3.3V DC Out | CH32V003 Pin 15 (`VDD`) | Target core logic supply ($I_{max} \approx 500\text{ mA}$) |
| **GND** | Digital Ground | CH32V003 Pin 14 (`VSS`) | System common ground |

> [!WARNING]
> **Avoid ESP32-S3 GPIO19 and GPIO20 for SWIO**: When native USB CDC is enabled (`-DARDUINO_USB_CDC_ON_BOOT=1`, `-DARDUINO_USB_MODE=1`), GPIO19 (`USB D-`) and GPIO20 (`USB D+`) carry the host USB Serial connection. Mapping SWIO to GPIO19/20 will drop the serial link. Use **GPIO10** (default) or **GPIO21** (`-DSWIO_PIN=21`).

---

## 2. CH32V003A4M6 (SOP-16) Pinout

The **CH32V003A4M6** is an ultra-low-cost 32-bit RISC-V microcontroller in a standard 16-pin Small Outline Package (SOP-16).

```
                    CH32V003A4M6 (SOP-16)
                        +-----\_/-----+
          PC1 / A1  [ 1]|             |[16] VDD (3.3V)
          PC2 / A2  [ 2]|             |[15] VSS (GND)
               PC3  [ 3]|             |[14] PA2 / OSCO
     PC4 (Motor/LED)[ 4]|             |[13] PA1 / OSCI
          PC6 / A3  [ 5]|             |[12] PD7 / NRST
               PC7  [ 6]|             |[11] PD6 / RXD
    PD1 (SWIO Debug)[ 7]|             |[10] PD5 / TXD
               PD4  [ 8]|             |[ 9] PD3
                        +-------------+
```

### Pin Assignments for Programmer Setup
* **Pin 4 (`PC4`)**: General purpose push-pull output used to drive CP6208 motor driver input `AIN`.
* **Pin 7 (`PD1` / `SWIO`)**: 1-wire proprietary single-wire serial debug interface line connected to ESP32-S3 GPIO10.
* **Pin 14 (`VSS`)**: System Ground.
* **Pin 15 (`VDD`)**: 3.3V Power Supply.

---

## 3. SWIO Pull-Up Configuration

The single-wire debug protocol relies on an open-drain bidirectional signaling standard. Wire an external **$4.7\text{k}\Omega$ to $10\text{k}\Omega$ pull-up resistor between `PD1` (CH32 Pin 7) and `3.3V` (CH32 Pin 15)** — **do NOT place the resistor in series** on the SWIO data line. Both boards must also share common `3.3V` and `GND` reference rails so edge thresholds stay deterministic.

---

## 4. CP6208 DC Motor Driver Application Circuit

The **CP6208** is a monolithic bidirectional H-bridge motor driver IC capable of sourcing up to 100mA continuous / 500mA peak motor current.

### Driver Pinout (SOP-8 / SOT23-6)
* **Pin 1 (`AIN`)**: Motor Channel A Control Input (Connected to CH32 Pin 4 `PC4`).
* **Pin 2 (`GND`)**: Logic Ground (Connected to System Common GND).
* **Pin 3 (`BIN`)**: Motor Channel B Control Input (**Tied to GND** for uni-directional ON/OFF operation).
* **Pin 4 (`OUT1`)**: Motor Positive Terminal.
* **Pin 5 (`OUT2`)**: Motor Negative Terminal.
* **Pin 6 (`VCC`)**: Dedicated Motor Supply Voltage ($4.5\text{V} - 12\text{V}$).

### Control Logic Truth Table
| Input A (`AIN` / PC4) | Input B (`BIN` / GND) | Output 1 | Output 2 | Motor State |
| :---: | :---: | :---: | :---: | :---: |
| **LOW (0.0V)** | **LOW (0.0V)** | OFF / Hi-Z | OFF / Hi-Z | **Standby / Motor OFF** |
| **HIGH (3.3V)** | **LOW (0.0V)** | $V_{CC}$ | GND | **Forward Run (Motor ON)** |
| **LOW (0.0V)** | **HIGH (3.3V)** | GND | $V_{CC}$ | Reverse Run |
| **HIGH (3.3V)** | **HIGH (3.3V)** | GND | GND | Brake |

---

## 5. Power Domain Architecture & Grounding Rules

```
+--------------------------+       +--------------------------+       +--------------------------+
|  Programmer Domain       |       |  Target Logic Domain     |       |  Motor Power Domain      |
|  - USB-C Host (5V / 3.3V)|       |  - CH32 Core (3.3V)      |       |  - External DC (5V-12V)  |
|  - ESP32-S3 Core & GPIO  |       |  - Logic I/O Pins        |       |  - CP6208 VCC & Armature |
+------------+-------------+       +------------+-------------+       +------------+-------------+
             |                                  |                                  |
             +----------------------------------+----------------------------------+
                                        Common Ground (GND)
```

1. **Common Ground**: All grounds (ESP32-S3 `GND`, CH32V003 `VSS`, CP6208 `GND`, and Motor Power Supply negative terminal) **MUST** be connected together.
2. **Independent Motor Supply**: Motor power ($V_{CC}$) must always come from a dedicated external power supply to prevent inductive voltage spikes from interfering with or resetting the microcontrollers.

---

## 6. Troubleshooting & Shared-Bus Isolation

1. **Serial Port Drops Immediately at Boot (`GPIO19` / `GPIO20`)**: Verify `SWIO_PIN` does not collide with native USB CDC pins (`GPIO19`/`GPIO20`). Use `GPIO10` or pass `-DSWIO_PIN=21` in `platformio.ini`.
2. **SWIO Sync Succeeds, but Chip ID (`0x1FFFF704`) or `progbuf` Unlock Fails**:
   * Disconnect all shared-bus peripherals and any secondary CH32V003 devices sharing power or I/O rails with the target.
   * Keep only the ESP32-S3 programmer, the single target CH32V003, and the `PD1` $\leftrightarrow$ `3.3V` pull-up connected, then re-run `detect`.
3. **Reads Return `0x00000000` or `0xFFFFFFFF`**: Confirm the pull-up resistor is wired in parallel (`PD1` to `3.3V`) rather than in series on the SWIO wire, and verify `esp_cpu_get_cycle_count` / `IO_MUX` input buffer initialization under ESP-IDF 5.x (`espressif32 @ 6.13.0`).
