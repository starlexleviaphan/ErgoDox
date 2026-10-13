# Task 4 Report: Typing-Reactive Bongo Cat Mascot & WPM Speedometer Gauge

## Executive Summary
Successfully designed, implemented, and verified the interactive typing-reactive Bongo Cat mascot widget (`bongo_cat.h`/`.c`) and the WPM speedometer gauge widget (`wpm_gauge.h`/`.c`) for the ErgoDox Wireless Cyber-HUD on the central dongle display shield. Both widgets adhere to ZMK's display widget listener architecture, Zephyr delayed work queue scheduling, and LVGL v8 cyber styling. Verified clean compilation with strict compiler warnings (`-Wall -Wextra -Werror -std=c99`), passed full unit testing of key matrix partitioning and threshold logic, passed all 90 checks in the Firmware Critic validation audit, and committed the changes to `zmk-config` (commit `034b4ef`).

## Implemented Components

### 1. Interactive Bongo Cat Mascot Widget (`bongo_cat.h` & `bongo_cat.c`)
- **File Paths:**
  - `zmk-config/boards/shields/ergodox/src/widgets/bongo_cat.h`
  - `zmk-config/boards/shields/ergodox/src/widgets/bongo_cat.c`
- **Architecture & Interfaces:**
  - `struct zmk_widget_bongo_cat` containing `sys_snode_t node`, `lv_obj_t *obj`, `lv_obj_t *canvas`, `lv_obj_t *head_label`, `lv_obj_t *paw_l`, `lv_obj_t *paw_r`, and `lv_obj_t *desk`.
  - Exported functions: `zmk_widget_bongo_cat_init()`, `zmk_widget_bongo_cat_obj()`.
  - Subscribes to `zmk_position_state_changed` via `ZMK_LISTENER(bongo_cat_key_tracker, ...)`:
    - Left half keys (38 keys: `0..6, 14..20, 28..34, 42..47, 54..58, 64..65, 68..70, 74`): Drops left paw onto desk and schedules 150ms return timer.
    - Right half keys (38 keys: `7..13, 21..27, 35..41, 48..53, 59..63, 66..67, 71..73, 75`): Drops right paw onto desk and schedules 150ms return timer.
    - Timers use `k_work_reschedule_for_queue(zmk_display_work_q(), ...)` ensuring thread-safe LVGL redraw in the dedicated display workqueue.
  - Subscribes to `zmk_wpm_state_changed` via `ZMK_DISPLAY_WIDGET_LISTENER(widget_bongo_cat, ...)`:
    - **Low Speed (<30 WPM):** Normal cute face `( o.o )` in Cyber Cyan (`#00d2ff`).
    - **Mid Speed (30..70 WPM):** Excited happy eyes `( ^.^ )` in Lime Green (`#76ff03`).
    - **High Speed (>70 WPM):** Cool sunglasses `( B-) )` in Neon Amber (`#ff9100`).
- **Interactive Visuals:**
  - Paws dynamically alternate between resting `/` and `\` positions and hitting the desk `o` at desk coordinates.
  - Desk lights up in neon green (`#38ef7d`) when either paw strikes.
  - Cyber-card border with dynamic accent illumination.

### 2. WPM Speedometer Gauge Widget (`wpm_gauge.h` & `wpm_gauge.c`)
- **File Paths:**
  - `zmk-config/boards/shields/ergodox/src/widgets/wpm_gauge.h`
  - `zmk-config/boards/shields/ergodox/src/widgets/wpm_gauge.c`
- **Architecture & Interfaces:**
  - `struct zmk_widget_wpm_gauge` containing `sys_snode_t node`, `lv_obj_t *obj`, `lv_obj_t *arc`, and `lv_obj_t *label`.
  - Exported functions: `zmk_widget_wpm_gauge_init()`, `zmk_widget_wpm_gauge_obj()`, `zmk_widget_wpm_gauge_get_color()`.
  - Subscribes to `zmk_wpm_state_changed` via `ZMK_DISPLAY_WIDGET_LISTENER(widget_wpm_gauge, ...)`.
  - Range: 0 to 120 WPM (clamped at 120 for arc scale).
  - Formatted readout: `WPM: %u`.
- **Dynamic Speed Thresholds:**
  - **Low (<45 WPM):** Cyan (`#00d2ff`)
  - **Mid (45..80 WPM):** Lime Green (`#76ff03`)
  - **Fast (81..110 WPM):** Orange (`#ff9100`)
  - **Turbo (>110 WPM):** Crimson Red (`#ff1744`)
  - The arc indicator, numeric readout, and container border dynamically update their color to match the current speed tier.

### 3. Build Configuration
- `zmk-config/boards/shields/ergodox/CMakeLists.txt`:
  - Added `src/widgets/bongo_cat.c` and `src/widgets/wpm_gauge.c` to `zephyr_library_sources()`.
- `zmk-config/boards/shields/ergodox/ergodox_dongle_display.conf`:
  - Added `CONFIG_ZMK_WPM=y` to ensure ZMK's WPM calculation engine is enabled on the dongle display shield.

## Verification & Test Results
1. **Compilation Test (`-Wall -Wextra -Werror -std=c99`):**
   - Built with GCC C99 compiler test harness exercising initialization, state callbacks, paw reactivity, timer scheduling, and speed thresholds.
   - Result: 0 warnings, 0 errors, binary executed with return code 0.
2. **Key Matrix Partitioning Verification:**
   - 76 keys tested across rows 0..4 and thumb clusters:
   - Exactly 38 keys mapped to Left half.
   - Exactly 38 keys mapped to Right half.
   - All assertions passed.
3. **Firmware Critic Deep Audit:**
   - Command: `node .agents/skills/firmware-critic/scripts/audit.js`
   - Result: **90 checks passed, 0 warnings, 0 errors**.
   - 100% specification and architecture parity confirmed.

## Git Commits
- **Repository:** `zmk-config`
- **Initial Feature Commit:** `034b4ef14f287b50aa9ca1c16af0509cb96db8bb` (short: `034b4ef`)
  - `feat(widgets): add reactive bongo cat mascot and wpm speedometer gauge`
- **Review Hardening Commit:** `b2c422561c346cd6c4db580c2358f65e498cd588` (short: `b2c4225`)
  - `fix(widgets): statically define bongo cat work queues and guard boot/bounds events`

## Post-Review Hardening Fixes
1. **[CRITICAL] Kernel HardFault Prevention on Boot-Time Keypress:**
   - Switched from dynamic work initialization in `zmk_widget_bongo_cat_init()` to compile-time static definition using Zephyr macros:
     `K_WORK_DEFINE(update_paws_work, bongo_cat_update_paws_handler)`
     `K_WORK_DELAYABLE_DEFINE(lift_paw_l_work, bongo_cat_lift_l_handler)`
     `K_WORK_DELAYABLE_DEFINE(lift_paw_r_work, bongo_cat_lift_r_handler)`
   - Added guard in `position_state_changed_listener` checking `sys_slist_is_empty(&bongo_cat_widgets)` so events before widget creation safely return 0 without queueing unneeded work items.
2. **[IMPORTANT] Key Matrix Upper-Bound Guard:**
   - Guarded `ev->position >= 76` in `position_state_changed_listener` to prevent out-of-bounds positions from triggering left paw.
3. **Verification:**
   - C99 test harness verified boot-time keypress with empty list, out-of-bounds keypresses (`position = 76`, `999`), and valid matrix keypresses with zero warnings/errors.
   - Firmware Critic deep audit verified 90/90 checks passed.

## Status
**DONE**

