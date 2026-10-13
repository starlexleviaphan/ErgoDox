# Task 3 Brief: Caps Lock & 14-Layer Themed Badge Widgets

## Objectives
1. Implement `lock_status.h` and `lock_status.c` in `zmk-config/boards/shields/ergodox/src/widgets/`:
   - Define `struct zmk_widget_lock_status` with `lv_obj_t *obj` and `lv_obj_t *label`.
   - Subscribe to `zmk_hid_indicators_changed` using `ZMK_DISPLAY_WIDGET_LISTENER`.
   - Render illuminated `[CAPS]` badge when Caps Lock (or Caps Word) is active. When inactive, display dim outline or hide.
   - Provide standard functions:
     - `int zmk_widget_lock_status_init(struct zmk_widget_lock_status *widget, lv_obj_t *parent);`
     - `lv_obj_t *zmk_widget_lock_status_obj(struct zmk_widget_lock_status *widget);`

2. Implement `layer_badge.h` and `layer_badge.c` in `zmk-config/boards/shields/ergodox/src/widgets/`:
   - Define `struct zmk_widget_layer_badge` with `lv_obj_t *obj` (container card), `lv_obj_t *label` (layer name), `lv_obj_t *desc_label`.
   - Subscribe to `zmk_layer_state_changed` using `ZMK_DISPLAY_WIDGET_LISTENER`.
   - Color lookup table for all 14 layers (indices 0..13):
     - 0 `WORK`: `#00d2ff`
     - 1 `SWAP`: `#38ef7d`
     - 2 `LNPD`: `#f9d423`
     - 3 `TYPE`: `#00f2fe`
     - 4 `SYM1`: `#a18cd1`
     - 5 `NRPD`: `#ff9a44`
     - 6 `MOUS`: `#4facfe`
     - 7 `ARRW`: `#43e97b`
     - 8 `GAME`: `#ff0844`
     - 9 `GSWP`: `#f857a6`
     - 10 `NPAD`: `#f6d365`
     - 11 `FUN1`: `#b721ff`
     - 12 `FUN2`: `#6f86d6`
     - 13 `SYM2`: `#f355da`
   - Update container background / border color and label text upon layer transition.
   - Provide standard functions:
     - `int zmk_widget_layer_badge_init(struct zmk_widget_layer_badge *widget, lv_obj_t *parent);`
     - `lv_obj_t *zmk_widget_layer_badge_obj(struct zmk_widget_layer_badge *widget);`

3. Update `zmk-config/boards/shields/ergodox/CMakeLists.txt` to include:
   - `src/widgets/lock_status.c`
   - `src/widgets/layer_badge.c`

4. Run validation audit:
   - `node .agents/skills/firmware-critic/scripts/audit.js`

5. Commit in `zmk-config`:
   - Message: `feat(widgets): add lock status indicator and 14-layer color badge`

## Report File
Write full execution details to `.superpowers/sdd/2026-10-13-dongle-cyber-hud-menu/task-3-report.md`.
Return only: status (DONE), commit hash, test summary.
