# Task 1 Brief: Screen 180° Orientation Flip & Kconfig Configuration

## Objectives
1. In `zmk-config/boards/shields/ergodox/ergodox_dongle_display.conf`:
   - Set `CONFIG_DONGLE_SCREEN_FLIPPED=y` (flips display 180 degrees via `DISPLAY_ORIENTATION_ROTATED_90`)
   - Set `CONFIG_DONGLE_SCREEN_SYSTEM_ICON=2` (Windows ⊞ logo instead of Mac clover)
   - Ensure `CONFIG_ZMK_HID_INDICATORS=y` is set for Caps Lock indicator support
2. Verify that `node .agents/skills/firmware-critic/scripts/audit.js` passes with 0 errors.
3. Commit changes in `zmk-config` with message: `feat(dongle): enable 180 degree screen flip and windows system icon`

## Report File
Write full execution details to `.superpowers/sdd/2026-10-13-dongle-cyber-hud-menu/task-1-report.md`.
Return only: status (DONE), commit hash, test summary.
