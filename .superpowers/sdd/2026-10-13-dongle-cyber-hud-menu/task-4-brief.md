# Task 4 Brief: Typing-Reactive Bongo Cat Mascot & WPM Speedometer

## Objectives
1. Implement `bongo_cat.h` and `bongo_cat.c` in `zmk-config/boards/shields/ergodox/src/widgets/`:
   - Define `struct zmk_widget_bongo_cat` with `sys_snode_t node`, `lv_obj_t *obj`, `lv_obj_t *canvas` (or object components for head, paws, desk).
   - Subscribe to `zmk_position_state_changed`:
     - Detect Left half keys: `0..6, 14..20, 28..34, 42..47, 54..58, 64..65, 68..70, 74` -> trigger Left paw tap.
     - Detect Right half keys: `7..13, 21..27, 35..41, 48..53, 59..63, 66..67, 71..73, 75` -> trigger Right paw tap.
     - Paw returns to resting position after ~150ms timer (`k_work_delayable` or `k_timer`).
   - Subscribe to `zmk_wpm_state_changed`:
     - Low speed (<30 WPM): Normal cute face `( o.o )`
     - Mid speed (30..70 WPM): Happy excited eyes `( ^.^ )`
     - High speed (>70 WPM): Sunglasses / cool face `( B-) )`
   - Standard widget API:
     - `int zmk_widget_bongo_cat_init(struct zmk_widget_bongo_cat *widget, lv_obj_t *parent);`
     - `lv_obj_t *zmk_widget_bongo_cat_obj(struct zmk_widget_bongo_cat *widget);`

2. Implement `wpm_gauge.h` and `wpm_gauge.c` in `zmk-config/boards/shields/ergodox/src/widgets/`:
   - Define `struct zmk_widget_wpm_gauge` with `sys_snode_t node`, `lv_obj_t *obj`, `lv_obj_t *arc` (or segmented bar), `lv_obj_t *label`.
   - Subscribe to `zmk_wpm_state_changed`:
     - Range: 0 to 120 WPM (clamped or scaled).
     - Color thresholds:
       - Low (<45 WPM): Cyan `#00d2ff`
       - Mid (45..80 WPM): Lime Green `#76ff03`
       - Fast (81..110 WPM): Orange `#ff9100`
       - Turbo (>110 WPM): Crimson Red `#ff1744`
     - Display formatted text like `WPM: %d`.
   - Standard widget API:
     - `int zmk_widget_wpm_gauge_init(struct zmk_widget_wpm_gauge *widget, lv_obj_t *parent);`
     - `lv_obj_t *zmk_widget_wpm_gauge_obj(struct zmk_widget_wpm_gauge *widget);`

3. Update `zmk-config/boards/shields/ergodox/CMakeLists.txt` to include:
   - `src/widgets/bongo_cat.c`
   - `src/widgets/wpm_gauge.c`

4. Run validation audit:
   - `node .agents/skills/firmware-critic/scripts/audit.js`

5. Commit in `zmk-config`:
   - Message: `feat(widgets): add reactive bongo cat mascot and wpm speedometer gauge`

## Report File
Write full execution details to `.superpowers/sdd/2026-10-13-dongle-cyber-hud-menu/task-4-report.md`.
Return only: status (DONE), commit hash, test summary.
