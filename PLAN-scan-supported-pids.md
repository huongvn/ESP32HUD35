# PLAN: Scan OBD-II supported PIDs (Kia Picanto)

Status: accepted
Date: 2026-08-16
Branch: dev_improve_ui

## Goal

Determine exactly which OBD-II Mode-01 PIDs the ECU in the user's Kia Picanto
supports, and log the result over serial. No UI changes in this task.

Rationale: previous PIDs (MAF 0x10, Fuel 0x2F, Oil 0x5C) returned 0 because the
ECU does not support them. Instead of guessing, we query the standard "supported
PID" bitmaps (SAE J1979) to get a definitive list.

## Safety

- Only read-only services are used: Mode 01 (current data) and Mode 03 (read
  DTCs). No write/control services are ever sent (no 0x04, 0x2E, 0x27, 0x11...).
- We only probe the four standard PID-range queries (0x00/0x20/0x40/0x60) and a
  fixed table of known PIDs. No "rainbow attack" / random PID scanning.
- Query rate stays low and separate from the display PID loop.

## Background: how "supported PID" works

- PID 0x00 response = 4-byte bitmap, bit set => PID 0x01..0x20 supported.
- PID 0x20 response = bitmap for 0x21..0x40.
- PID 0x40 response = bitmap for 0x41..0x60.
- PID 0x60 response = bitmap for 0x61..0x80.

Bit order (SAE J1979): first byte A, bit7 (MSB) = lowest PID of the range.
For PID `p` in range `[r+1 .. r+32]` (r = 0x00/0x20/0x40/0x60):
  byte_index = (p - 1) % 8
  bit_mask   = 1 << (7 - ((p - 1) % 8))
  supported  = (bitmap[(p - 1) / 8] & bit_mask) != 0

## Implementation changes (src/can_module.cpp only)

1. Storage
   - `static uint8_t g_supported[4][4];`  // groups 0x00,0x20,0x40,0x60
   - `static bool g_sup_got[4];`
   - `static bool g_sup_logged;`

2. RX handling in `can_handle_rx()`
   - Before the existing Mode-01 switch, add a branch: if `msg->data[1] == 0x41`
     and `msg->data[2]` is one of {0x00, 0x20, 0x40, 0x60} and
     `msg->data_length_code >= 7`:
     - group = data[2] / 32
     - copy `msg->data[3..6]` into `g_supported[group]`
     - set `g_sup_got[group] = true`
     - do NOT touch `g_can.last_ms`/`fresh` (not display data)
     - if all groups received and not yet logged -> call `can_log_supported()`

3. Helper `pid_supported(uint8_t pid)`
   - returns false if pid==0 or group not yet received
   - computes byte/bit per formula above

4. Helper `can_log_supported()`
   - prints "CAN: supported PIDs:" and lists each supported PID as hex + name
     using a small static lookup table for 0x01..0x60 (name or "?").

5. Scheduler in `can_task()`
   - Add `last_sup` and `sup_idx` (0..3).
   - Every 2 s send one query: pid of group = sup_idx * 32
     (0x00, 0x20, 0x40, 0x60), increment sup_idx (mod 4).
   - If group 0x00 has been received, log what we have even if other groups are
     missing (0x00 is always supported), to be robust against partial responses.

## Definition of done

- Firmware builds and uploads (env esp32-3248S035C).
- On the car, serial prints a list of supported PIDs.
- Display loop / HUD behaviour unchanged.

## Out of scope

- Adding new PIDs to the HUD (follow-up task after reviewing scan output).
- Sniffing proprietary Hyundai/Kia broadcast frames.
- Writing a PLAN file in repo root: this file.

## Verification

- `pio run -e esp32-3248S035C` (build)
- `pio run -e esp32-3248S035C -t upload` (upload)
- Ask user to plug into car and share serial output.
