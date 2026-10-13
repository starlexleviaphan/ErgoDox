# Task 8 Brief: Flash Settings Persistence (Zephyr Settings)

## Objectives
Persist the dongle display preferences across power cycles, written to flash ONLY on the menu's "Save & Exit" action (never on intermediate A/D changes).

1. In `zmk-config/boards/shields/ergodox/src/menu/menu_controller.c`:
   - Persisted parameters (namespace `ergodox_display/`): `brightness` (uint8 10..100, default 80), `timeout_idx` or `timeout_s` (default 600), `flipped` (bool, default true), `anim_style` (uint8 0..2, default 0), `theme_id` (uint8 0..3, default 0).
   - Register `SETTINGS_STATIC_HANDLER_DEFINE(ergodox_display, "ergodox_display", NULL, display_settings_set, NULL, NULL)` following the pattern already used in `src/widgets/battery_status.c` (role settings): validate length and value ranges on load; discard invalid data instead of applying it.
   - On successful load, apply values to the in-memory state and schedule a UI refresh via `zmk_display_work_q()` (no direct LVGL calls from the settings thread).
   - "Save & Exit" menu item calls `settings_save_one()` for each parameter (or one packed struct) and logs failures with LOG_WRN; exiting without saving must leave flash untouched.
   - Guard everything with `#if IS_ENABLED(CONFIG_SETTINGS)` and provide a no-op fallback, as battery_status.c does.
2. In `zmk-config/boards/shields/ergodox/ergodox_dongle_display.conf`: ensure `CONFIG_SETTINGS=y` (and the NVS/flash settings backend ZMK already uses; do not duplicate existing options).
3. Run `node .agents/skills/firmware-critic/scripts/audit.js` (must pass 90/90).
4. Verify with a GCC C99 harness (-Wall -Wextra -Werror) with stubbed Zephyr settings: load valid values, load out-of-range values (rejected), wrong length (rejected), save only on Save & Exit.
5. Commit in `zmk-config`: `feat(settings): add persistent flash storage for dongle display parameters`

## Report File
`.superpowers/sdd/2026-10-13-dongle-cyber-hud-menu/task-8-report.md`. Return only: status, commit hash, test summary.
