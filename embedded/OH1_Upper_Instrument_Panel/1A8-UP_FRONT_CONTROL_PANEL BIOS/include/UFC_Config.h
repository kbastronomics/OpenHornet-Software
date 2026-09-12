/**************************************************************************************
 *        ____                   _    _                       _
 *       / __ \                 | |  | |                     | |
 *      | |  | |_ __   ___ _ __ | |__| | ___  _ __ _ __   ___| |_
 *      | |  | | '_ \ / _ \ '_ \|  __  |/ _ \| '__| '_ \ / _ \ __|
 *      | |__| | |_) |  __/ | | | |  | | (_) | |  | | | |  __/ |_
 *       \____/| .__/ \___|_| |_|_|  |_|\___/|_|  |_| |_|\___|\__|
 *             | |
 *             |_|
 *   ----------------------------------------------------------------------------------
 *   Copyright 2016-2026 OpenHornet
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *   ----------------------------------------------------------------------------------
 *   This Project uses Doxygen as a documentation generator.
 *   Please use Doxygen capable comments.
 **************************************************************************************/

/**
 * @file    ufc_config.h
 * @author  Sandra Carroll
 * @date    11.05.2026
 * @version 1.0.0
 * @brief   UFC runtime configuration menu using the five OP displays and CUE indicators.
 *
 * @details Presents a one-page settings menu on OP1–OP5:
 *
 *          | Display | Label | Controls          | CUE on = enabled |
 *          |---------|-------|-------------------|------------------|
 *          | OP1     | BOOT  | Boot splash       | CUE1             |
 *          | OP2     | IRQ   | IRQ vs polling    | CUE2             |
 *          | OP3     | HID   | USB HID mode      | CUE3             |
 *          | OP4     | OTA   | OTA update mode   | CUE4             |
 *          | OP5     | ELOG  | Elog remote log   | CUE5             |
 *
 *          - The OP display labels are written once by begin() and never change.
 *          - The CUE for each option tracks the current ON/OFF state in real time.
 *          - Pressing an OS button (OS1–OS5) toggles the corresponding option and
 *            immediately updates only the affected CUE.
 *          - Pressing ENT saves all current settings to NVS via UfcState::save()
 *            and signals the caller that the menu should close (update() returns true).
 *
 *          The class is fully decoupled from main.cpp globals.  Two function
 *          pointers are supplied at construction time:
 *          - @c WriteDisplayFn — writes a 4-character label to one OP display.
 *          - @c SetCueFn      — turns one CUE indicator on or off.
 *
 *          Typical wiring in main.cpp (when ready to activate):
 *          @code
 *          // Helper that bridges UfcConfig to the global display infrastructure
 *          void writeConfigLabel(int idx, const char* label4) {
 *              memcpy(op_display[idx].content, label4, 4);
 *              update_op_display(idx);
 *          }
 *
 *          UfcConfig cfgMenu(ufcState, writeConfigLabel, cue);
 *
 *          // In loop(), when config mode is active:
 *          bool done = cfgMenu.update(
 *              // OS1–OS5 key states (UFCkeys column 3, rows 0–4)
 *              { UFCkeys[0][3], UFCkeys[1][3], UFCkeys[2][3],
 *                UFCkeys[3][3], UFCkeys[4][3] },
 *              // ENT key state
 *              UFCkeys[4][2]
 *          );
 *          if (done) { cfgMenu.end(); }
 *          @endcode
 *
 * @note    Key states are active-HIGH (1 = pressed, 0 = released), matching
 *          the convention used by the TCA8418 UFCkeys matrix.
 *          Edge detection is handled internally — pass the raw key state
 *          every loop iteration; do not debounce before passing.
 *
 * @note    This header is self-contained.  Include it after UFC_State_Checker.h.
 *          The class is NOT yet wired into the main program.
 */

#pragma once

#ifndef UFC_CONFIG_H
#define UFC_CONFIG_H

#include <Arduino.h>
#include "UFC_State_Checker.h"

/**
 * @brief  UFC runtime configuration menu controller.
 *
 * @details Manages label display, CUE state indicators, button edge detection,
 *          option toggling, and NVS persistence for the five configurable UFC
 *          runtime options (BOOT, IRQ, HID, OTA, ELOG).
 *
 *          The class owns no display or hardware state directly.  All hardware
 *          interaction is delegated through the two function pointers supplied
 *          at construction so the class remains testable and decoupled.
 */
class UfcConfig {
public:

  // ── Callback types ────────────────────────────────────────────────────────

  /**
   * @brief  Callback type: write a 4-character label to one OP display.
   *
   * @details The implementation must copy @p label (exactly 4 chars, no null
   *          required) into the display content buffer and then push it to
   *          hardware.  In main.cpp this wraps memcpy + update_op_display().
   *
   * @param displayIdx  OP display index to update (0 = OP1 … 4 = OP5).
   * @param label4      Pointer to exactly 4 ASCII characters to display.
   */
  using WriteDisplayFn = void (*)(int displayIdx, const char* label4);

  /**
   * @brief  Callback type: turn one CUE indicator on or off.
   *
   * @details In main.cpp this is the global cue() function directly.
   *          CUE positions: 0 = CUE1 … 4 = CUE5.
   *
   * @param cuePos  CUE position (0–4).
   * @param on      true = illuminate; false = extinguish.
   */
  using SetCueFn = void (*)(uint8_t cuePos, bool on);

  // ── Menu option descriptor ────────────────────────────────────────────────

  /**
   * @brief  One entry in the config menu — the label shown on the OP display
   *         and the getter/setter pair that reads and writes the option value.
   */
  struct Option {
    const char* label;          ///< Exactly 4 characters shown on the OP display (never changes).
    bool (UfcState::*get)() const;  ///< Member function pointer: reads current option value.
    void (UfcState::*set)(bool);    ///< Member function pointer: writes toggled option value.
  };

  // ── Construction and initialisation ──────────────────────────────────────

  /**
   * @brief  Construct the config menu controller.
   *
   * @param state       Reference to the application's UfcState singleton.
   * @param writeDisplay  Callback that writes a 4-char label to one OP display.
   * @param setCue        Callback that turns one CUE indicator on or off.
   */
  UfcConfig(UfcState& state, WriteDisplayFn writeDisplay, SetCueFn setCue)
    : _state(state), _writeDisplay(writeDisplay), _setCue(setCue),
      _lastEntKey(false)
  {
    memset(_lastOsKeys, 0, sizeof(_lastOsKeys));
  }

  // ── Menu lifecycle ────────────────────────────────────────────────────────

  /**
   * @brief  Show the config menu: write all labels to OP1–OP5 and set CUEs.
   *
   * @details Iterates the five options, writes each label via the WriteDisplayFn
   *          callback (OP display content does not change again after this call),
   *          then calls _refreshAllCues() to reflect the current saved state.
   *          Resets internal edge-detection state so stale key history from
   *          before the menu opened cannot cause spurious toggles.
   */
  void begin() {
    memset(_lastOsKeys, 0, sizeof(_lastOsKeys));
    _lastEntKey = false;

    // Write static labels — these never change while the menu is open
    for (uint8_t i = 0; i < NUM_OPTIONS; i++) {
      _writeDisplay(i, _options[i].label);
    }
    _refreshAllCues();
  }

  /**
   * @brief  Poll key states, toggle options on press, save on ENT.
   *
   * @details Call once per loop() iteration while the config menu is active.
   *          Performs rising-edge detection on each of the five OS buttons
   *          (0→1 transition = just pressed) and on the ENT button.
   *
   *          On an OS button press, the corresponding option is toggled and
   *          only that option's CUE is updated — the other four CUEs are
   *          not disturbed.
   *
   *          On an ENT press, UfcState::save() is called to persist all
   *          current settings to NVS and the function returns @c true to
   *          signal that the menu should close.  The caller is responsible
   *          for calling end() after receiving @c true.
   *
   * @param osKeys  Array of five OS button states [OS1..OS5], active-HIGH.
   *                Pass raw UFCkeys values without pre-debouncing.
   * @param entKey  ENT button state, active-HIGH.
   *
   * @return @c true  — ENT was just pressed; caller should call end().
   * @return @c false — menu still active; call again next loop iteration.
   */
  bool update(const uint8_t osKeys[5], uint8_t entKey) {
    // ── OS buttons: rising edge → toggle option and refresh only that CUE ──
    for (uint8_t i = 0; i < NUM_OPTIONS; i++) {
      if (osKeys[i] && !_lastOsKeys[i]) {
        _toggleOption(i);
      }
      _lastOsKeys[i] = osKeys[i];
    }

    // ── ENT: rising edge → save and signal menu close ─────────────────────
    bool entPressed = entKey && !_lastEntKey;
    _lastEntKey = entKey;
    if (entPressed) {
      _state.save();
      return true;
    }
    return false;
  }

  /**
   * @brief  Dismiss the config menu: extinguish all five CUE indicators.
   *
   * @details Called by the application after update() returns @c true (ENT
   *          pressed) or when the menu is closed by any other means.  Leaves
   *          the OP display labels in place — the caller clears or overwrites
   *          the displays as needed for the next screen.
   */
  void end() {
    for (uint8_t i = 0; i < NUM_OPTIONS; i++) {
      _setCue(i, false);
    }
  }

private:

  // ── Internal helpers ──────────────────────────────────────────────────────

  /**
   * @brief  Refresh all five CUE indicators from the current option states.
   *
   * @details Reads each option via its getter member function pointer and
   *          drives the corresponding CUE on or off.  Called once by begin()
   *          and after any option toggle via _toggleOption().
   */
  void _refreshAllCues() {
    for (uint8_t i = 0; i < NUM_OPTIONS; i++) {
      _setCue(i, (_state.*_options[i].get)());
    }
  }

  /**
   * @brief  Toggle one option and update only its CUE indicator.
   *
   * @details Reads the current value via the getter, inverts it, writes it
   *          via the setter, then drives the CUE to match.  The other four
   *          CUE indicators are untouched.
   *
   * @param idx  Option index (0 = BOOT … 4 = ELOG).
   */
  void _toggleOption(uint8_t idx) {
    if (idx >= NUM_OPTIONS) return;
    bool current = (_state.*_options[idx].get)();
    (_state.*_options[idx].set)(!current);
    _setCue(idx, !current);
  }

  // ── Option table ──────────────────────────────────────────────────────────

  /** @brief Number of configurable options displayed in the menu. */
  static constexpr uint8_t NUM_OPTIONS = 5;

  /**
   * @brief  Static option descriptor table — defines the menu layout.
   *
   * @details Each entry maps one OP display (by index) to a label string and
   *          a get/set member function pair on UfcState.  The order determines
   *          which OS button controls which option:
   *          index 0 = OS1/OP1, index 1 = OS2/OP2, … index 4 = OS5/OP5.
   */
  static constexpr Option _options[NUM_OPTIONS] = {
    { "BOOT", &UfcState::getBootMode, &UfcState::setBootMode }, ///< OP1 / CUE1 / OS1
    { "IRQ ", &UfcState::getIrqMode,  &UfcState::setIrqMode  }, ///< OP2 / CUE2 / OS2
    { "HID ", &UfcState::getHidMode,  &UfcState::setHidMode  }, ///< OP3 / CUE3 / OS3
    { "OTA ", &UfcState::getOtaMode,  &UfcState::setOtaMode  }, ///< OP4 / CUE4 / OS4
    { "ELOG", &UfcState::getElogMode, &UfcState::setElogMode }  ///< OP5 / CUE5 / OS5
  };

  // ── Private members ───────────────────────────────────────────────────────

  /** @brief Reference to the application's UfcState singleton. */
  UfcState&      _state;
  /** @brief Callback: writes a 4-char label to an OP display. */
  WriteDisplayFn _writeDisplay;
  /** @brief Callback: drives one CUE indicator on or off. */
  SetCueFn       _setCue;
  /** @brief Previous OS button states for rising-edge detection (0 = not pressed). */
  uint8_t        _lastOsKeys[NUM_OPTIONS];
  /** @brief Previous ENT button state for rising-edge detection. */
  uint8_t        _lastEntKey;
};

// ── Out-of-class static member definition ────────────────────────────────────

/**
 * @brief  Out-of-class definition required for constexpr static member arrays in C++11/14.
 *
 * @details constexpr static data members with non-trivial types need an
 *          out-of-class definition to be ODR-usable (e.g. passed by reference).
 *          C++17 relaxes this for inline constexpr — if the build uses C++17
 *          this definition can be removed.
 */
constexpr UfcConfig::Option UfcConfig::_options[UfcConfig::NUM_OPTIONS];

#endif // UFC_CONFIG_H
