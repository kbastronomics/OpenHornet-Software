# UFC Runtime Config Menu — TODO

Companion to [UFC_CONFIG_MENU_STATUS.md](UFC_CONFIG_MENU_STATUS.md). Ordered
roughly by what blocks what.

## Decisions needed
- [ ] **Resolve `NVS_VERSION` 3 → 1 change** in [UFC_State_Checker.h](include/UFC_State_Checker.h#L661). Confirm whether this is intentional (e.g. deliberately re-baselining before first real release) or an accidental edit. If boards already have v3 data saved, decide whether to bump to `4` instead so `load()` cleanly rejects the old record and falls back to defaults, rather than reusing `1`.
- [ ] **Define the menu entry point.** Nothing in `loop()`/`setup()` currently constructs a `UfcConfig` or decides when to show it. Pick the trigger (e.g. hold ENT + an OS button, a long-press on an encoder, a boot-time combo like the existing `ENC1PB`-held-for-OTA pattern) and where in `loop()` it's checked.
- [ ] **Decide how the menu coexists with DCS-BIOS display updates.** `op_display[]` is written by DCS-BIOS callbacks continuously; opening the config menu needs some way to pause/bypass those writes while active (e.g. a global `configMenuActive` flag checked before `update_op_display()` calls elsewhere), and to restore normal display content on `end()`.

## Wiring (once the above is decided)
- [ ] Add `#include "UFC_Config.h"` to [1A8-UP_FRONT_CONTROL_PANEL.cpp](src/1A8-UP_FRONT_CONTROL_PANEL.cpp).
- [ ] Write the bridge function for `WriteDisplayFn` (wraps `memcpy` into `op_display[i].content` + `update_op_display(i)`, per the example already documented in [UFC_Config.h](include/UFC_Config.h#L59-L77)).
- [ ] Instantiate `UfcConfig cfgMenu(ufcState, writeConfigLabel, cue);` at file scope.
- [ ] Call `cfgMenu.begin()` on menu entry, `cfgMenu.update(...)` each `loop()` iteration while active (feeding it `UFCkeys[0..4][3]` for OS1–OS5 and `UFCkeys[4][2]` for ENT), and `cfgMenu.end()` when `update()` returns `true`.
- [ ] Restore/refresh the normal DCS-BIOS-driven display content after `end()`.

## Placeholder features to actually implement
- [ ] **HID mode** ([UFC_State_Checker.h](include/UFC_State_Checker.h#L302-L316) `setHidMode()`): wire to real TinyUSB HID descriptor init/teardown so toggling it actually changes USB enumeration. Currently just flips the enum.
- [ ] **Elog mode** ([UFC_State_Checker.h](include/UFC_State_Checker.h#L318-L334) `setElogMode()`): wire to `Logger.configureSyslog()` / a disable call so toggling it actually starts/stops remote syslog. Currently just flips the enum.
- [ ] **S3 Mini pinout** ([1A8-UP_FRONT_CONTROL_PANEL.cpp](src/1A8-UP_FRONT_CONTROL_PANEL.cpp#L184-L216) `#if defined(ARDUINO_LOLIN_S3_MINI)` block): currently a straight copy of the S2 Mini pin assignments — needs the actual S3 Mini wiring once that hardware variant is defined.

## Testing / validation
- [ ] Build both PlatformIO environments (`lolin_s2_mini`, `lolin_s3_mini`) to confirm the new `#if defined(ARDUINO_LOLIN_S2_MINI)` / `ARDUINO_LOLIN_S3_MINI` split compiles cleanly on each and that exactly one branch is active per board (no macro left permanently defining pins for the wrong board).
- [ ] Bench-test NVS `save()`/`load()` round-trip with the new 12-byte record (power-cycle test: set HID/ELOG via debugger or temporary test hook, reboot, confirm values persist).
- [ ] Once wired, bench-test the actual menu: enter menu, toggle each of the 5 options, confirm only the touched CUE changes, press ENT, confirm NVS save + menu close + display restore.
- [ ] Re-run existing test suite in [test/](test/) if it covers `UfcState`.

## Housekeeping
- [ ] `git add` the new [include/UFC_Config.h](include/UFC_Config.h) once the above is far enough along to commit.
- [ ] Update [UFC_CONFIG_MENU_STATUS.md](UFC_CONFIG_MENU_STATUS.md) as items here are completed.
