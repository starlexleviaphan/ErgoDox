# Task 8 Report

Status: DONE. Commit 24e3c71 in zmk-config.

## Changes
- `menu_controller.c`:
  - Added settings handler `ergodox_display` with keys `brightness` (u8 10..100), `timeout_s` (u16, must match a table value), `flipped` (u8 0/1), `anim_style` (u8 <3) and `theme_id` (u8 <4).
  - Length and range are validated, and invalid data is discarded with LOG_WRN.
  - A successful load updates the in-memory state, then submits `settings_apply_work` to `zmk_display_work_q()`. That handler applies the rotation and refreshes the UI, so no LVGL call is made from the settings thread.
  - `save_display_settings()` calls `settings_save_one` for each of the 5 keys and logs failures with LOG_WRN. It runs only in the Save & Exit branch, so exit via ESC and intermediate A/D changes never touch flash.
  - Everything is guarded by `IS_ENABLED(CONFIG_SETTINGS)`, with a no-op fallback.
- `ergodox_dongle_display.conf`: added `CONFIG_SETTINGS=y`. ZMK already provides the NVS backend, so no extra options.

## Verification
- GCC C99 harness (`-std=gnu99 -Wall -Wextra -Werror`) with stubbed Zephyr, LVGL and ZMK headers. It lives in `.superpowers/sdd/.../t8harness/` and includes the real `menu_controller.c`.
- Result: ALL PASS.
  - Valid loads of all 5 keys were applied.
  - Out-of-range values were rejected for all 5 keys, with no UI work queued.
  - Wrong lengths were rejected.
  - Zero saves happened on A/D changes or ESC exit.
  - Save & Exit made exactly 5 saves with the right key names.
- `audit.js`: 90/90 checks passed.
- Not verified: a real firmware build (no GitHub Actions run).

## Fix: orientation mechanism (review HIGH)
- Removed all lv_disp_set_rotation/LV_DISP_ROT_* use. set_screen_orientation() now only updates state and submits orientation_apply_work to zmk_display_work_q().
- The work handler calls display_set_orientation(DEVICE_DT_GET(DT_CHOSEN(zephyr_display)), flipped ? ROTATED_90 : ROTATED_270), with device_is_ready check and LOG_WRN on error, then lv_obj_invalidate(lv_scr_act()).
- settings_apply_handler reuses that handler, so a loaded flipped value is applied the same way.
- Harness: LVGL rotation symbols removed from the stub header, so any use would fail to compile. Added display_set_orientation stub. Asserts: flipped=true -> ROTATED_90, false -> ROTATED_270, and no display call from the caller thread until the work handler runs. ALL PASS (-Wall -Wextra -Werror).

