# Review Package: Task 5

## Commits
```
6f19b3d feat(display): implement Cyber-HUD card-based status screen for 280x240 display
```

## Summary
Implemented modular LVGL card-based Cyber-HUD status screen (`custom_status_screen.h`/`.c`) for 280×240 landscape display:
- Header Card (280×28 px): Output status (USB/BLE) + Lock status (`[CAPS]`/`[CW]`).
- Center Left Card (145×140 px): 14-layer palette badge + WPM speedometer gauge.
- Center Right Card (125×140 px): Typing-reactive Bongo Cat mascot.
- Footer Card (280×62 px): Modifiers status (Windows logo) + 3-channel Battery monitor (`L`, `R`, `TP`).
- Updated `CMakeLists.txt` to replace upstream YADS screen with local `custom_status_screen.c`.
- Exposes `zmk_display_get_dashboard_root()` for Task 6 menu toggling.
