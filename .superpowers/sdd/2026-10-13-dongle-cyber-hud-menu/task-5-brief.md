# Task 5 Brief: Modular LVGL Card-Based Dashboard ("Cyber-HUD")

## Objectives
1. Implement `custom_status_screen.c` in `zmk-config/boards/shields/ergodox/src/`:
   - Implement `lv_obj_t *zmk_display_status_screen(void)` function.
   - Geometry: 280 px wide × 240 px high (landscape mode).
   - Layout modular LVGL card containers with dark cyber theme (`bg: #0a0d14`, `radius: 6`, `border: 1px #1f293d`):
     - **Header Card (280×28 px):**
       - Embeds Output status (`zmk_widget_output_status_init`) on left.
       - Embeds Lock status (`zmk_widget_lock_status_init`) centered/right.
     - **Center Left Card (145×140 px):**
       - Embeds Layer Badge (`zmk_widget_layer_badge_init`) with 14-layer color styling.
       - Embeds WPM Gauge (`zmk_widget_wpm_gauge_init`).
     - **Center Right Card (125×140 px):**
       - Embeds Bongo Cat Mascot (`zmk_widget_bongo_cat_init`) with reactive paw tapping.
     - **Footer Card (280×62 px):**
       - Embeds Modifier status (`zmk_widget_mod_status_init`) with Windows logo.
       - Embeds 3-channel battery monitor (`zmk_widget_dongle_battery_status_init`) showing `L`, `R`, `TP`.
   - Add function to access the active dashboard root object so menu mode (Task 6) can hide/show the dashboard.

2. Update `zmk-config/boards/shields/ergodox/CMakeLists.txt`:
   - Replace `${DONGLE_SCREEN_SRC_DIR}/src/custom_status_screen.c` with local `src/custom_status_screen.c`.

3. Run validation audit:
   - `node .agents/skills/firmware-critic/scripts/audit.js`
   - Must pass with 90/90 checks and 0 errors.

4. Commit in `zmk-config`:
   - Message: `feat(display): implement Cyber-HUD card-based status screen for 280x240 display`

## Report File
Write full execution details to `.superpowers/sdd/2026-10-13-dongle-cyber-hud-menu/task-5-report.md`.
Return only: status (DONE), commit hash, test summary.
