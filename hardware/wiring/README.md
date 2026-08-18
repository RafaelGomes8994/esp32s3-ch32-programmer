# Hardware Wiring & Physical Connections

This document details the physical wiring between the **ESP32-S3 Programmer**, the **CH32V003A4M6 (SOP-16)** target microcontroller, and the **CP6208** DC motor driver application circuit.

---

## 1. Programmer $\rightarrow$ Target Interface (SWIO)

The CH32V003 utilizes a proprietary single-wire serial debug interface (SWIO) connected to pin `PD1`.

```
ESP32-S3                          CH32V003A4M6 (SOP-16)
+---------------+                 +--------------------+
|               |                 |                    |
|        GPIO10 +--------+------->+ Pin 7  (PD1/SWIO)  |
|               |        |        |                    |
|          3.3V +---+    [4.7k-   |                    |
|               |   |     10k]    |                    |
|               |   +----+--+---->+ Pin 15 (VDD)       |
|               |                 |                    |
|           GND +---------------->+ Pin 14 (VSS)       |
+---------------+                 +--------------------+
```

### Critical Requirements
* **External Pull-Up Resistor**: A **$4.7\text{k}\Omega$ – $10\text{k}\Omega$ pull-up resistor** to 3.3V on the SWIO line is mandatory for reliable 1-wire open-drain bi-directional communication.
* **Common Ground**: Ensure low-impedance ground connection between ESP32-S3 `GND` and CH32 `VSS` (Pin 14).

---

## 2. CH32V003 $\rightarrow$ CP6208 Motor Driver Circuit

```
CH32V003A4M6 (SOP-16)             CP6208 Motor Driver
+--------------------+            +-------------------+
|                    |            |                   |
| Pin 4  (PC4)       +----------->+ Pin 1 (AIN)       |
|                    |            |                   |
| Pin 14 (VSS/GND)   +----+------>+ Pin 2 (GND)       |
|                    |    |       |                   |
+--------------------+    +------>+ Pin 3 (BIN)       |
                                  |                   |
External Motor Power Supply       |                   |
+--------------------+            |                   |
| (+) Motor VCC (5V) +----------->+ Pin 6 (VCC)       |
|                    |            |                   |
| (-) Power GND      +------------+                   |
+--------------------+            |   Pin 4 (OUT1)    |
                                  |   Pin 5 (OUT2)    |
                                  +-----+-------+-----+
                                        |       |
                                     +--+-------+--+
                                     |   DC Motor  |
                                     +-------------+
```

---

## 3. Power Rail Isolation Architecture

To avoid electrical noise and brownouts, three separate power domains are used:

| Domain | Voltage | Source | Purpose |
| :--- | :--- | :--- | :--- |
| **Programmer Domain** | 5V / 3.3V | USB-C Port | Powers ESP32-S3 core, USB-UART bridge, and on-board LDO |
| **Target Logic Domain** | 3.3V | ESP32-S3 3.3V Out | Powers CH32V003 core logic and GPIOs (Pins 14 & 15) |
| **Motor Power Domain** | 4.5V – 12V | External DC Supply | Powers CP6208 $V_{CC}$ (Pin 6) and DC motor armature |

> [!CAUTION]
> Never power the CP6208 motor supply ($V_{CC}$) directly from the CH32V003 GPIO pins or the ESP32-S3 3.3V rail. High inductive current spikes can damage the microcontrollers. All grounds must remain tied together.
