# Task 6 Brief: Interactive Menu Controller & Input Intercept Engine

## Objectives
1. Implement `menu_controller.h` and `menu_controller.c` in `zmk-config/boards/shields/ergodox/src/menu/`:
   - Define state machine:
     ```c
     enum screen_state {
         SCREEN_STATE_DASHBOARD = 0,
         SCREEN_STATE_MENU,
         SCREEN_STATE_LAYER_MAP,
     };
     ```
   - Menu Activation & Toggle:
     - Listen for `F21` keycode (or key position 1 on Layer 6) to toggle menu active state.
     - When entering menu:
       - Set `menu_active = true`.
       - Hide Dashboard (`lv_obj_add_flag(zmk_display_get_dashboard_root(), LV_OBJ_FLAG_HIDDEN)`).
       - Show Menu root container (`lv_obj_clear_flag(menu_root, LV_OBJ_FLAG_HIDDEN)`).
     - When exiting menu:
       - Set `menu_active = false`.
       - Hide Menu root, show Dashboard root.
   - Host HID Input Suppression:
     - When `menu_active == true`, intercept keypresses (W, A, S, D, Space, Enter, Esc) for menu navigation.
     - Call `zmk_hid_keyboard_clear()` so intercepted navigation keystrokes are suppressed and never transmitted over USB/BLE to the host computer.
   - Menu Navigation & UI:
     - `W` (Up) / `S` (Down): Move cursor between menu items.
     - `A` (Left/Dec) / `D` (Right/Inc): Adjust active item value.
     - `Space` / `Enter`: Select item / enter subscreen.
     - `Esc` / Toggle: Exit menu back to Dashboard.
     - Menu Items:
       1. `[ Яркость ]`: 10%..100% (calls `set_screen_brightness()`)
       2. `[ Таймаут сна ]`: 0 (Off), 60s, 180s, 300s, 600s, 900s
       3. `[ Ориентация ]`: Normal / Flipped (180°)
       4. `[ Анимация ]`: Bongo Cat / WPM Gauge / Off
       5. `[ Тема ]`: Cyberpunk / Monokai / Matrix / Clean
       6. `[ Карта слоев ]`: Calls `layer_inspector_show()` (Task 7 stub if not yet linked)
       7. `[ Сохранить и выйти ]`: Saves settings (Task 8) and returns to Dashboard
   - Standard initialization:
     - `int menu_controller_init(lv_obj_t *screen);`
     - Registered with ZMK display lifecycle.

2. Update `zmk-config/boards/shields/ergodox/CMakeLists.txt`:
   - Add `src/menu/menu_controller.c` to `zephyr_library_sources()`.
   - Add `src/menu` to `zephyr_library_include_directories()`.

3. Run validation audit:
   - `node .agents/skills/firmware-critic/scripts/audit.js`

4. Commit in `zmk-config`:
   - Message: `feat(menu): implement interactive WASD menu controller with HID suppression`

## Report File
Write full execution details to `.superpowers/sdd/2026-10-13-dongle-cyber-hud-menu/task-6-report.md`.
Return only: status (DONE), commit hash, test summary.
