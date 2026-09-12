# UFC Runtime Config Menu — Status

Tracks the in-progress feature that adds an on-device settings menu (driven by
the OP displays, CUE indicators, and OS/ENT buttons) for toggling UFC runtime
options and persisting them to NVS. See [UFC_CONFIG_MENU_TODO.md](UFC_CONFIG_MENU_TODO.md)
for the remaining work.

**Branch:** `UFCFixes#1` (1 commit ahead of `origin/UFCFixes#1`)
**Last updated:** 2026-09-12

## Working tree state (uncommitted)

| File | Status | Summary |
|------|--------|---------|
| [include/UFC_State_Checker.h](include/UFC_State_Checker.h) | Modified | Adds `hidmode` / `elogmode` state fields, getters/setters, and NVS persistence |
| [src/1A8-UP_FRONT_CONTROL_PANEL.cpp](src/1A8-UP_FRONT_CONTROL_PANEL.cpp) | Modified | Board-conditional pin defs (S2/S3 Mini), removed debug pragma, reordered ELOG init |
| [include/UFC_Config.h](include/UFC_Config.h) | **New, untracked** | `UfcConfig` class — the menu controller itself. Not yet wired into the `.cpp` |

## What's done

### `UFC_State_Checker.h`
- New `State` enum tokens: `HID` / `NOHID` and `ELOGON` / `ELOGOFF`.
- New `Snapshot` fields `hidmode` (default `NOHID`) and `elogmode` (default `ELOGOFF`).
- `setHidMode(bool)` / `getHidMode()` and `setElogMode(bool)` / `getElogMode()` added, mirroring the existing `otamode`/`irqmode` pattern. Both are marked `PLACEHOLDER` — they only flip the in-memory enum, no hardware behavior is wired yet.
- `save()` / `load()` extended from a 10-byte to a 12-byte NVS record (`"hid"` and `"elog"` keys added), both folded into the CRC-8/SMBUS check.
- `NVS_VERSION` changed from `3` → `1`, with the version-history comment rewritten to describe this as the "initial release schema" rather than an increment. **Flagged in the TODO — needs a decision**, since this was previously shipped as version 3.

### `UFC_Config.h` (new)
- `UfcConfig` class: a self-contained, hardware-decoupled menu controller.
  - Menu layout: OP1=BOOT, OP2=IRQ, OP3=HID, OP4=OTA, OP5=ELOG, each with a matching CUE1–5.
  - Takes `WriteDisplayFn` and `SetCueFn` callbacks at construction so it doesn't touch `op_display[]` or `cue()` directly.
  - `begin()` writes static labels and syncs all 5 CUEs to current state.
  - `update(osKeys[5], entKey)` does rising-edge detection per OS button (toggle + single-CUE refresh) and on ENT (calls `UfcState::save()`, returns `true` to signal close).
  - `end()` extinguishes all CUEs.
  - Verified the callback signatures match the real hardware functions already in `1A8-UP_FRONT_CONTROL_PANEL.cpp`: `cue(uint8_t pos, bool state)` ([src/1A8-UP_FRONT_CONTROL_PANEL.cpp:948](src/1A8-UP_FRONT_CONTROL_PANEL.cpp#L948)) and the `UFCkeys[row][col]` matrix used for `ufcos1`..`ufcos5`/`ufcent` DCS-BIOS bindings ([src/1A8-UP_FRONT_CONTROL_PANEL.cpp:1440-1445](src/1A8-UP_FRONT_CONTROL_PANEL.cpp#L1440-L1445)).
- Confirmed via grep: `UfcConfig`/`UFC_Config` has **zero references** in the `.cpp` file — the class exists but nothing constructs or calls it yet.

### `1A8-UP_FRONT_CONTROL_PANEL.cpp`
- Pin-assignment block is now split into `#if defined(ARDUINO_LOLIN_S2_MINI)` / `#if defined(ARDUINO_LOLIN_S3_MINI)`, in preparation for building the same firmware for both boards. The S3 block is currently a byte-for-byte copy of the S2 pinout (placeholder, not the real S3 wiring).
- Removed the `#pragma message "Board macro: ..."` build-log line.
- Moved the `#if defined(ENABLE_ELOG)` syslog init block in `setup()` to run after `ufcState.begin()` instead of before.

## Open questions / risks worth flagging
- **NVS_VERSION rollback (3→1):** if any board in the field already saved a v3 record, a v1 read on top of it could misinterpret bytes rather than cleanly falling back to defaults, since the version check would now accept a byte layout it didn't originally have. Needs a conscious decision (see TODO).
- **No entry point exists yet** for opening the config menu at runtime — there's no button-combo handler in `loop()` that would construct/drive a `UfcConfig` instance. Boot-time OTA selection (`ENC1PB` held at power-on) is the only current "mode selection" mechanism, and it's unrelated.
- **Display ownership conflict:** `op_display[]` is currently driven by DCS-BIOS callback updates during normal operation. Opening the config menu will need to suspend those writes (or they'll immediately overwrite the menu labels) — not yet designed.
