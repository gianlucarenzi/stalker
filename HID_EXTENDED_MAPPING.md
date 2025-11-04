# HID Extended Mapping – USB Consumer/System → Amiga Combos

## 1) Purpose
This document describes how non-boot (extended) USB HID events—especially from Usage Page `0x0C` (Consumer) and `0x01` (System Control)—are converted into generic `KEY_*` scancodes delivered to the Amiga task as key combinations. The goal is to let an Amiga-side helper consume those combos (e.g., for volume/mute/playback) without changing the USB→Amiga lookup table.

## 2) Design principles
- **Report Protocol**: The firmware operates in Report Protocol when possible (fallback to Boot if not supported), parses the report descriptor, and decodes INPUT fields per ReportID/Usage.
- **Extended events**: Decoded Consumer/System events are enqueued into `extended_input_queue` and logged by `extended_logger_task` (ANSI colors). A bridge task forwards them to the Amiga pipeline as generic `KEY_*` scancodes.
- **Generic scancodes only**: The bridge sends only generic `KEY_*` (USB HID keyboard codes) to the Amiga task. The conversion to Amiga scancodes remains entirely handled by `scancode_to_amiga()` and `scancodeamiga[]`.
- **Combos using CTRL + ALT + Fn**: All extended functions are mapped to `CTRL+ALT+F1…F9`. `SHIFT` (L/R) is reserved for future features.
- **Consistent press/release ordering**: For `pressed=1`, the bridge sends `[CTRL, ALT, Fn]` press; for `pressed=0`, it sends `[Fn, ALT, CTRL]` release. It tracks state to avoid duplicate press/release.

## 3) Current mapping
> Note: Amiga does not have F11/F12; functions above F10 are not used.

### Consumer (Usage Page `0x0C`)
- **Mute (`0xE2`)** → `CTRL + ALT + F1`
- **Volume Down (`0xEA`)** → `CTRL + ALT + F2`
- **Volume Up (`0xE9`)** → `CTRL + ALT + F3`
- **Play/Pause (`0xCD`)** → `CTRL + ALT + F4`
- **Next Track (`0xB5`)** → `CTRL + ALT + F5`
- **Previous Track (`0xB6`)** → `CTRL + ALT + F6`
- **Stop (`0xB7`)** → `CTRL + ALT + F7`

### System / Extended Keyboard
- **PrintScreen/SysReq** → `CTRL + ALT + F8`
- **Application/Menu** → `CTRL + ALT + F9`

## 4) Generic `KEY_*` used (not Amiga scancodes)
- **Modifiers**:
  - `KEY_LEFTCONTROL` or `KEY_RIGHTCONTROL` (either can be used; default LEFT)
  - `KEY_LEFTALT` or `KEY_RIGHTALT` (either can be used; default LEFT)
- **Function keys**:
  - `KEY_F1 … KEY_F10`

The bridge uses `KEY_LEFTCONTROL` and `KEY_LEFTALT` by default, but RIGHT variants are equivalent and can be substituted later without impact.

## 5) Press/release semantics
- On `evt.pressed == 1`:
  1. Send `KEY_LEFTCONTROL` down
  2. Send `KEY_LEFTALT` down
  3. Send `KEY_Fn` down
- On `evt.pressed == 0`:
  1. Send `KEY_Fn` up
  2. Send `KEY_LEFTALT` up
  3. Send `KEY_LEFTCONTROL` up

The bridge keeps a small state per usage to avoid repeated press without release and to ignore release with no prior press.

## 6) Amiga pipeline interaction
- The Amiga task receives generic `KEY_*` scancodes (same path as base keyboard) and converts them via `scancode_to_amiga()` + `scancodeamiga[]` to actual Amiga scancodes.
- The USB→Amiga lookup table remains unchanged and is the single source of truth.

## 7) Logging and debugging
- `extended_logger_task` logs extended events with colors:
  - `HID_EVT_KEYBOARD` (extended): light green
  - `HID_EVT_CONSUMER`: light cyan
  - `HID_EVT_SYSTEM`: light magenta

Example:
```
[HID-EXT CONS] usage=0x00E9 (VolumeUp) pressed=1 ts=...
```

## 8) Future enhancements
- **Symbolic names**: The `0x0C` (Consumer) usage mapping can be expanded (e.g., Calculator, Eject, Browser).
- **Multi-bit release**: Currently press-only for multi-bit fields (`value != 0`). A stateful release can be added if needed.
- **Alternate combos**: `SHIFT` (L/R) or `GUI` (L/R) can be used later for richer macros. For now, only `CTRL+ALT` are used.

## 9) Notes and compatibility
- **Boot vs Report Protocol**: If the device does not support Report Protocol, the firmware remains in Boot Protocol. Many extended functions (Consumer/System) require Report Protocol.
- **Multi-interface devices**: The firmware is configured to handle multiple interfaces (`USBH_MAX_NUM_INTERFACES=3`) and larger buffers (`USBH_MAX_DATA_BUFFER=1024`).
- **Stability**: The original Amiga pipeline is unmodified; this mapping is additive.

## 10) References
- USB HID Usage Tables, Consumer Page (`0x0C`)
- Amiga Keymap Library (conversion/scancodes)
- USB→Amiga scancode mapping: `scancode_to_amiga()`, `scancodeamiga[]` in `src/amiga.c`
