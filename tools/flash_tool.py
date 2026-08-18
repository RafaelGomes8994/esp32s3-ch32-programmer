#!/usr/bin/env python3
"""
ESP32-S3 CH32V003 Binary Flasher Tool
--------------------------------------
Host workflow utility for programming and verifying binary firmware images
on the WCH CH32V003 microcontroller via the ESP32-S3 programmer.

Usage:
  python tools/flash_tool.py --port COM10 --bin firmware/ch32_blink/blink.bin --addr 0x08000000 [--reset]
"""

import argparse
import os
import sys
import time
import serial

CH32V003_FLASH_BASE = 0x08000000
CH32V003_FLASH_SIZE = 16 * 1024  # 16 KB

def main():
    parser = argparse.ArgumentParser(description="ESP32-S3 CH32V003 Binary Flasher Tool")
    parser.add_argument("--port", default="COM10", help="Serial port (default: COM10)")
    parser.add_argument("--bin", default=os.path.join("firmware", "ch32_blink", "blink.bin"), help="Path to target binary file (.bin) (default: firmware/ch32_blink/blink.bin)")
    parser.add_argument("--addr", default="0x08000000", help="Target flash memory address (default: 0x08000000)")
    parser.add_argument("--reset", action="store_true", help="Execute target reset/run after flashing and verification")
    args = parser.parse_args()

    # 1. File existence check
    if not os.path.exists(args.bin):
        print(f"Error: Binary file not found at '{args.bin}'", file=sys.stderr)
        sys.exit(1)

    try:
        with open(args.bin, "rb") as f:
            data = f.read()
    except Exception as e:
        print(f"Error: Failed to read binary file '{args.bin}': {e}", file=sys.stderr)
        sys.exit(1)

    file_size = len(data)
    if file_size == 0:
        print(f"Error: Binary file '{args.bin}' is empty (0 bytes).", file=sys.stderr)
        sys.exit(1)

    # 2. Validate binary size and address bounds against CH32V003 16KB flash
    try:
        addr_val = int(args.addr, 16)
    except ValueError:
        print(f"Error: Invalid hex address string '{args.addr}'", file=sys.stderr)
        sys.exit(1)

    print(f"Loaded binary '{args.bin}': {file_size} bytes")
    print(f"Target memory address: 0x{addr_val:08X}")

    if addr_val < CH32V003_FLASH_BASE or (addr_val + file_size) > (CH32V003_FLASH_BASE + CH32V003_FLASH_SIZE):
        print(
            f"Error: Binary bounds (0x{addr_val:08X} - 0x{(addr_val + file_size):08X}) "
            f"exceed CH32V003 16KB flash memory (0x{CH32V003_FLASH_BASE:08X} - 0x{(CH32V003_FLASH_BASE + CH32V003_FLASH_SIZE):08X}).",
            file=sys.stderr
        )
        sys.exit(1)

    hex_payload = data.hex()

    # 3. Connect to ESP32-S3 programmer over serial
    print(f"Connecting to ESP32-S3 programmer on port {args.port} (115200 baud)...")
    try:
        s = serial.Serial(args.port, 115200, timeout=3)
    except Exception as e:
        print(f"Error: Failed to open serial port {args.port}: {e}", file=sys.stderr)
        sys.exit(1)

    s.dtr = False
    s.rts = True
    time.sleep(0.1)
    s.rts = False
    time.sleep(0.5)

    startup_out = ""
    t_end = time.time() + 3
    while time.time() < t_end:
        if s.in_waiting:
            startup_out += s.read(s.in_waiting).decode("utf-8", errors="ignore")
        time.sleep(0.05)

    print("=== PROGRAMMER INITIALIZATION LOG ===")
    print(startup_out.strip())

    # 4. Target detection check
    if "Target detect: OK" not in startup_out:
        # Re-send explicit detect command if startup trace was truncated
        s.write(b"detect\n")
        detect_out = ""
        t_end = time.time() + 2
        while time.time() < t_end:
            if s.in_waiting:
                detect_out += s.read(s.in_waiting).decode("utf-8", errors="ignore")
            time.sleep(0.05)
        if "Target detect: OK" not in detect_out:
            print("Error: Target CH32V003 not detected on programmer!", file=sys.stderr)
            s.close()
            sys.exit(1)

    print("\n[STEP 1] Target Detection: OK")

    # 5. Program binary payload via targetProgramBinary() (erases required 64-byte pages only)
    flash_cmd = f"flash {addr_val:08X} {hex_payload}\n".encode("utf-8")
    print(f"\n[STEP 2] Programming target binary via targetProgramBinary() ({file_size} bytes)...")
    s.write(flash_cmd)

    flash_out = ""
    t_end = time.time() + 10
    while time.time() < t_end:
        if s.in_waiting:
            chunk = s.read(s.in_waiting).decode("utf-8", errors="ignore")
            flash_out += chunk
            sys.stdout.write(chunk)
            sys.stdout.flush()
        time.sleep(0.05)

    if "flash: OK" not in flash_out:
        print("\nError: Programming (targetProgramBinary) failed!", file=sys.stderr)
        s.close()
        sys.exit(1)

    print("\n[STEP 2 RESULT] targetProgramBinary: OK")

    # 6. Verify programmed binary via targetVerifyBinary()
    verify_cmd = f"verify {addr_val:08X} {hex_payload}\n".encode("utf-8")
    print(f"\n[STEP 3] Verifying programmed memory via targetVerifyBinary() ({file_size} bytes)...")
    s.write(verify_cmd)

    verify_out = ""
    t_end = time.time() + 10
    while time.time() < t_end:
        if s.in_waiting:
            chunk = s.read(s.in_waiting).decode("utf-8", errors="ignore")
            verify_out += chunk
            sys.stdout.write(chunk)
            sys.stdout.flush()
        time.sleep(0.05)

    if "verify: OK" not in verify_out:
        print("\nError: Verification (targetVerifyBinary) failed!", file=sys.stderr)
        s.close()
        sys.exit(1)

    print("\n[STEP 3 RESULT] targetVerifyBinary: OK")

    print("\n=== FLASHING & VERIFICATION SUMMARY ===")
    print("SUCCESS: Firmware successfully programmed and verified on CH32V003!")

    # 7. Optional explicit target reset/run
    if args.reset:
        print("\n[STEP 4] Executing explicit targetResetRun()...")
        s.write(b"reset\n")
        reset_out = ""
        t_end = time.time() + 3
        while time.time() < t_end:
            if s.in_waiting:
                chunk = s.read(s.in_waiting).decode("utf-8", errors="ignore")
                reset_out += chunk
                sys.stdout.write(chunk)
                sys.stdout.flush()
            time.sleep(0.05)
        if "Reset/run: OK" in reset_out:
            print("[STEP 4 RESULT] targetResetRun: OK (Target execution resumed)")
        else:
            print("Error: Target reset/run failed!", file=sys.stderr)
            s.close()
            sys.exit(1)
    else:
        print("Note: Target remains in debug halt state. (Pass --reset option to execute binary).")

    s.close()
    sys.exit(0)

if __name__ == "__main__":
    main()
