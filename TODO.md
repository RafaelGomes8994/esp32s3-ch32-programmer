# Project Roadmap & Milestones

## Phase 1: Core Physical Layer (Completed)
- [x] Precision cycle-accurate timing loop (`PrecDelay(10)`)
- [x] Open-drain drive high, drive low, and line release
- [x] SWIO single-wire bit transmit and receive

## Phase 2: DMI Protocol & Target Detection (Completed)
- [x] 32-bit DMI register read/write framing
- [x] `0x5AA50400` debug module initialization handshake
- [x] Read CH32 Chip ID (`0x0713BB91`) and `DMHARTINFO` (`0x002120F4`)
- [x] Abstract command memory word read at `0x1FFFF704`

## Phase 3: Flash Programming Engine (Completed)
- [x] 3-stage flash unlock key sequence (`FLASH_KEYR`, `FLASH_OBKEYR`, `FLASH_MODEKEYR`)
- [x] 64-byte fast page erase
- [x] 64-byte fast page buffer programming (`targetProgramBinary()`)
- [x] Bit-for-bit memory readback verification (`targetVerifyBinary()`)
- [x] Explicit target reset/run (`targetResetRun()`)

## Phase 4: Host Tooling & Real Application Verification (Completed)
- [x] Python host CLI tool (`tools/flash_tool.py`)
- [x] Non-destructive regression test suite
- [x] Bare-metal CH32V003 application test driving CP6208 motor load
- [x] Repository modularization and documentation suite

## Future Enhancements
- [ ] Support for multi-chip targets in daisy-chained configurations
- [ ] Option Byte (OB) configuration CLI commands
- [ ] Web-based WebUSB / WebSerial flashing frontend
