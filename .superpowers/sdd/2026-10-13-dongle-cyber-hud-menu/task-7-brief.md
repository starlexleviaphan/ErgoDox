# Task 7 Brief: ErgoDox 38-Key Layer Map Inspector

## Objectives
1. Implement `layer_inspector.h` and `layer_inspector.c` in `zmk-config/boards/shields/ergodox/src/menu/`:
   - Subscreen view attached to screen root (280×240 px).
   - Ergonomic matrix geometry for ErgoDox half (38 keys):
     - Main finger cluster: 5 rows × 6 columns.
     - Thumb cluster: 6 keys arranged in physical ErgoDox thumb cluster geometry (2 inner keys + 3 bottom keys + 1 tall key).
   - Interactive Navigation:
     - `A` / `D` (or Left / Right): Toggle between `LEFT HALF` and `RIGHT HALF`.
     - `W` / `S` (or Up / Down): Cycle inspected layer (0..13).
     - `Esc` or Toggle: Calls `menu_controller_set_state(SCREEN_STATE_MENU)` and `layer_inspector_hide()`.
   - Visual Styling:
     - Header bar with Layer Title (`#%u %s`), color-coded using `zmk_widget_layer_badge_get_color(layer)`.
     - Active half banner (`< LEFT HALF >` / `< RIGHT HALF >`).
     - Key cells: 38 rounded rects with category-based colors (Alphas, Modifiers, Layer Switches, Media/Mouse, Transparent `▽`).
   - Exported Functions:
     - `int layer_inspector_init(lv_obj_t *screen);`
     - `int layer_inspector_show(uint8_t layer, bool is_left_half);`
     - `void layer_inspector_hide(void);`
     - `void layer_inspector_handle_nav(enum menu_nav_action action);`
     - `bool layer_inspector_is_active(void);`

2. Wire `layer_inspector` into `menu_controller.c`:
   - Initialize in `menu_controller_init()`.
   - Forward navigation actions to `layer_inspector_handle_nav()` when `current_screen_state == SCREEN_STATE_LAYER_MAP`.
   - Call `layer_inspector_hide()` when leaving `SCREEN_STATE_LAYER_MAP`.

3. Update `zmk-config/boards/shields/ergodox/CMakeLists.txt`:
   - Add `src/menu/layer_inspector.c` to `zephyr_library_sources()`.

4. Run validation audit:
   - `node .agents/skills/firmware-critic/scripts/audit.js`
   - Must pass with 90/90 checks and 0 errors.

5. Commit in `zmk-config`:
   - Message: `feat(menu): implement ErgoDox 38-key layer map inspector`

## Report File
Write full execution details to `.superpowers/sdd/2026-10-13-dongle-cyber-hud-menu/task-7-report.md`.
Return only: status (DONE), commit hash, test summary.
