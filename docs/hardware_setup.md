# Hardware Setup & Pinout Guide

This document describes the physical connections between the ESP32-S3 programmer, the CH32V003 target MCU, and the CP6208 motor driver.

---

## Hardware Components

1. **Programmer Board**: YD-ESP32-S3 (N8R2) / ESP32-S3 DevKit
2. **Target MCU**: CH32V003A4M6 (SOP-16 Package)
3. **Motor Driver**: CP6208 H-Bridge Motor Driver IC
4. **Motor**: Mini 3V–6V DC Motor

---

## Pin Mapping Table

### 1. ESP32-S3 Programmer $\rightarrow$ CH32V003 (SOP-16)

| ESP32-S3 Signal | ESP32-S3 Pin | CH32V003 SOP-16 Pin | CH32 Function | Connection Notes |
| :--- | :--- | :--- | :--- | :--- |
| **SWIO** | GPIO10 | Pin 7 | `PD1` / `SWIO` | Single-wire debug data line (requires 3.3V pull-up) |
| **3.3V** | 3V3 | Pin 15 | `VDD` | Target main power supply |
| **GND** | GND | Pin 14 | `VSS` | Common System Ground |

---

### 2. CH32V003 Target $\rightarrow$ CP6208 Motor Driver $\rightarrow$ DC Motor

| Signal Source | Source Pin | Destination Pin | Destination Signal | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **CH32 PC4** | Pin 4 (`PC4`) | CP6208 Pin 1 (`AIN`) | Input A | 3.3V Push-Pull Output Control Signal |
| **Common GND** | Pin 14 (`VSS`) | CP6208 Pin 2 (`GND`) | Logic Ground | Must share common GND with ESP32 & Motor Power |
| **Common GND** | Pin 14 (`VSS`) | CP6208 Pin 3 (`BIN`) | Input B | **TIED TO GND** for uni-directional ON/OFF control |
| **External Power** | +5V Supply | CP6208 Pin 6 (`VCC`) | Motor Power | Dedicated Motor Power Supply (NOT driven by GPIO) |
| **CP6208 Output** | Driver OUT1/OUT2 | Motor (+) / (-) | DC Motor Terminals | H-Bridge Motor Drive Outputs |

---

## CP6208 Control Logic Summary

| Input A (`AIN` / PC4) | Input B (`BIN`) | Motor State |
| :---: | :---: | :---: |
| **LOW (0.0V)** | **LOW (0.0V)** | **Motor OFF** (Standby) |
| **HIGH (3.3V)** | **LOW (0.0V)** | **Motor ON** (Forward Run) |
| **LOW (0.0V)** | **HIGH (3.3V)** | Reverse Run |
| **HIGH (3.3V)** | **HIGH (3.3V)** | Brake |
