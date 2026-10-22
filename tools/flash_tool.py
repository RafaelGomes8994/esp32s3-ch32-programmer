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
import serial.tools.list_ports

CH32V003_FLASH_BASE = 0x08000000
CH32V003_FLASH_SIZE = 16 * 1024  # 16 KB
MAX_CHUNK_BYTES = 64  # Exact 64-byte hardware page boundary for robust SWIO timing


def get_candidate_ports():
    ports = list(serial.tools.list_ports.comports())
    usb_ports = []
    other_ports = []

    for p in ports:
        dev = p.device.lower()
        # On Linux/macOS, USB serial devices are ttyUSB*, ttyACM*, or cu.usb*
        # On Windows, COM ports with USB VID/PID
        is_usb = (
            any(tok in dev for tok in ["ttyusb", "ttyacm", "cu.usb", "tty.usb"])
            or (p.hwid and "usb" in p.hwid.lower())
            or (p.vid is not None)
        )
        if is_usb:
            usb_ports.append(p.device)
        elif sys.platform.startswith("win") and dev.startswith("com"):
            usb_ports.append(p.device)
        elif not dev.startswith("/dev/ttys"):  # filter out legacy Linux serial8250 dummy ports
            other_ports.append(p.device)

    # Sort USB ports: prefer ttyUSB over ttyACM
    usb_ports.sort(key=lambda d: (0 if "ttyusb" in d.lower() else 1))
    return usb_ports if usb_ports else other_ports


def probe_port_for_programmer(port_name, timeout_sec=2.0):
    try:
        s = serial.Serial()
        s.port = port_name
        s.baudrate = 115200
        s.timeout = 1.0
        s.dtr = False
        s.rts = False
        s.open()
    except Exception:
        return None, ""

    s.dtr = False
    s.rts = False
    time.sleep(0.1)

    initial_buf = ""
    t_end = time.time() + 1.0
    while time.time() < t_end:
        if s.in_waiting:
            initial_buf += s.read(s.in_waiting).decode("utf-8", errors="ignore")
        time.sleep(0.04)

    if "Target detect: OK" in initial_buf or "ESP32-S3 CH32 Programmer" in initial_buf:
        return s, initial_buf

    # Probe actively with detect command
    s.reset_input_buffer()
    s.write(b"detect\n")
    s.flush()
    detect_buf = ""
    t_end = time.time() + timeout_sec
    while time.time() < t_end:
        if s.in_waiting:
            detect_buf += s.read(s.in_waiting).decode("utf-8", errors="ignore")
            if "Target detect: OK" in detect_buf or "CH32 ID" in detect_buf or "SWIO sync: OK" in detect_buf:
                break
        time.sleep(0.04)

    combined = (initial_buf + "\n" + detect_buf).strip()
    if "Target detect: OK" in combined or "CH32 ID" in combined or "SWIO sync: OK" in combined:
        return s, combined

    s.close()
    return None, ""


def resolve_default_bin():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    coproc_bin = os.path.normpath(
        os.path.join(script_dir, "..", "firmware", "ch32_i2c_coprocessor", "i2c_coproc.bin")
    )
    if os.path.exists(coproc_bin):
        return coproc_bin
    return os.path.join("firmware", "ch32_blink", "blink.bin")


def send_and_wait(s, cmd_bytes, success_token, timeout_sec=10.0, echo=True):
    s.reset_input_buffer()
    s.write(cmd_bytes)
    s.flush()
    out = ""
    t_end = time.time() + timeout_sec
    while time.time() < t_end:
        if s.in_waiting:
            chunk = s.read(s.in_waiting).decode("utf-8", errors="ignore")
            out += chunk
            if echo:
                sys.stdout.write(chunk)
                sys.stdout.flush()
            if success_token in out or "FAILED" in out or "Invalid" in out or "Command too long" in out:
                break
        time.sleep(0.02)
    return out


def main():
    default_bin = resolve_default_bin()

    parser = argparse.ArgumentParser(description="ESP32-S3 CH32V003 Binary Flasher Tool")
    parser.add_argument(
        "--port",
        default="auto",
        help="Serial port (default: auto - scans all USB/serial ports for programmer)",
    )
    parser.add_argument("--bin", default=default_bin, help=f"Path to target binary file (.bin) (default: {default_bin})")
    parser.add_argument("--addr", default="0x08000000", help="Target flash memory address (default: 0x08000000)")
    parser.add_argument("--chunk-size", type=int, default=MAX_CHUNK_BYTES, help="Chunk size in bytes (multiple of 64, max 512, default: 64)")
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
    target_port = args.port
    s = None
    startup_out = ""

    if target_port == "auto":
        candidates = get_candidate_ports()
        if not candidates:
            print("Error: No serial/USB ports found on the system! Check USB connection.", file=sys.stderr)
            sys.exit(1)

        if len(candidates) == 1:
            target_port = candidates[0]
            print(f"Auto-detected single USB/serial port: {target_port}")
        else:
            print(f"Scanning {len(candidates)} USB/serial port(s) for ESP32-S3 programmer + CH32 target: {candidates}...")
            for port_candidate in candidates:
                print(f"  Probing {port_candidate}...", end=" ", flush=True)
                p_conn, p_out = probe_port_for_programmer(port_candidate, timeout_sec=2.0)
                if p_conn is not None:
                    print("FOUND!")
                    s = p_conn
                    target_port = port_candidate
                    startup_out = p_out
                    break
                else:
                    print("no response")

            if s is None:
                fallback_port = candidates[0]
                print(f"Note: Auto-probe did not receive instant handshake; connecting directly to {fallback_port}...")
                target_port = fallback_port

    if s is None:
        print(f"Connecting to ESP32-S3 programmer on port {target_port} (115200 baud)...")
        try:
            s = serial.Serial()
            s.port = target_port
            s.baudrate = 115200
            s.timeout = 3
            s.dtr = False
            s.rts = False
            s.open()
        except Exception as e:
            print(f"Error: Failed to open serial port {target_port}: {e}", file=sys.stderr)
            sys.exit(1)

        s.dtr = False
        s.rts = False
        time.sleep(0.1)

        t_end = time.time() + 2.5
        while time.time() < t_end:
            if s.in_waiting:
                startup_out += s.read(s.in_waiting).decode("utf-8", errors="ignore")
            time.sleep(0.05)

    print(f"\nConnected to programmer on {target_port}!")
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
            print(f"Error: Target CH32V003 not detected on programmer ({target_port})!", file=sys.stderr)
            print("Please verify:")
            print("  1. GPIO 10 (ESP32) is connected to PD1 (CH32V003 SWIO).")
            print("  2. External 4.7k - 10k ohm pull-up resistor between 3.3V and SWIO line.")
            print("  3. 3.3V and GND are common between ESP32 and CH32V003.")
            s.close()
            sys.exit(1)

    print("\n[STEP 1] Target Detection: OK")

    # 5. Program binary payload via targetProgramBinary() in page-aligned chunks
    chunk_size = max(64, min(512, (args.chunk_size // 64) * 64))
    print(f"\n[STEP 2] Programming target binary via targetProgramBinary() ({file_size} bytes in {chunk_size}-byte chunks)...")

    for offset in range(0, file_size, chunk_size):
        chunk_data = data[offset : offset + chunk_size]
        chunk_hex = chunk_data.hex()
        cur_addr = addr_val + offset
        flash_cmd = f"flash {cur_addr:08X} {chunk_hex}\n".encode("utf-8")

        success = False
        for attempt in range(1, 4):
            flash_out = send_and_wait(s, flash_cmd, "flash: OK", timeout_sec=10.0, echo=True)
            if "flash: OK" in flash_out:
                success = True
                break
            print(f"\n[WARN] Chunk at 0x{cur_addr:08X} attempt {attempt}/3 failed, re-synchronizing SWIO...")
            send_and_wait(s, b"detect\n", "Target detect: OK", timeout_sec=3.0, echo=False)
            send_and_wait(s, b"unlock\n", "Unlock: OK", timeout_sec=3.0, echo=False)

        if not success:
            print(f"\nError: Programming (targetProgramBinary) failed at 0x{cur_addr:08X}!", file=sys.stderr)
            s.close()
            sys.exit(1)

    print("\n[STEP 2 RESULT] targetProgramBinary: OK")

    # 6. Verify programmed binary via targetVerifyBinary() in chunks
    print(f"\n[STEP 3] Verifying programmed memory via targetVerifyBinary() ({file_size} bytes)...")

    for offset in range(0, file_size, chunk_size):
        chunk_data = data[offset : offset + chunk_size]
        chunk_hex = chunk_data.hex()
        cur_addr = addr_val + offset
        verify_cmd = f"verify {cur_addr:08X} {chunk_hex}\n".encode("utf-8")

        success = False
        for attempt in range(1, 4):
            verify_out = send_and_wait(s, verify_cmd, "verify: OK", timeout_sec=10.0, echo=True)
            if "verify: OK" in verify_out:
                success = True
                break
            print(f"\n[WARN] Verify at 0x{cur_addr:08X} attempt {attempt}/3 failed, re-synchronizing SWIO...")
            send_and_wait(s, b"detect\n", "Target detect: OK", timeout_sec=3.0, echo=False)

        if not success:
            print(f"\nError: Verification (targetVerifyBinary) failed at 0x{cur_addr:08X}!", file=sys.stderr)
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
