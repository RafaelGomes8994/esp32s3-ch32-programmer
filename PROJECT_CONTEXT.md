# Project Context & Verification Status

## Hardware Configuration
- **Programmer**: ESP32-S3 (YD-ESP32-S3 N8R2)
- **Target**: CH32V003A4M6 (SOP-16 Package)
- **SWIO Connection**: ESP32-S3 GPIO10 $\rightarrow$ CH32 Pin 7 (`SWIO`)

## Feature Verification Checklist
- [x] GPIO Timing & Single-Wire SWIO Protocol Driver
- [x] Open-Drain Line Drive & Bit Sampling
- [x] Chip Detection (`0x0713BB91`) & `DMHARTINFO` (`0x002120F4`)
- [x] Flash Unlock Sequence (`FLASH_KEYR`, `FLASH_OBKEYR`, `FLASH_MODEKEYR`)
- [x] 64-Byte Fast Page Erase
- [x] 64-Byte Fast Page Programming (`targetProgramBinary()`)
- [x] Memory Readback Verification (`targetVerifyBinary()`)
- [x] Target Reset/Run Execution (`targetResetRun()`)
- [x] Host Python CLI Flashing Tool (`tools/flash_tool.py`)
- [x] Modular Architecture & Complete Documentation
