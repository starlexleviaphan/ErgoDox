# Review Package: Task 6

## Commits
```
898bdf3 feat(menu): implement interactive WASD menu controller with HID suppression
```

## Summary
Implemented interactive WASD on-screen menu controller and input intercept engine (`menu_controller.h`/`.c`) for the ErgoDox Wireless Cyber-HUD:
- State Machine: `SCREEN_STATE_DASHBOARD`, `SCREEN_STATE_MENU`, `SCREEN_STATE_LAYER_MAP`.
- Menu Activation: Triggered via `F21` or matrix position 1 on Layer 6 (`MOUS`).
- Host HID Input Suppression: Calls `zmk_hid_keyboard_clear()` while `menu_active == true` on navigation key events (W, A, S, D, Space, Enter, Esc), preventing any keystrokes from leaking into host OS windows.
- Navigation: W/S (Up/Down cursor), A/D (Dec/Inc values), Space/Enter (Select), Esc/Toggle (Exit).
- 7 Configurable Settings Items: Brightness, Sleep Timeout, Orientation (Normal/180°), Animation, Theme, Layer Map Inspector, Save & Exit.
- Weak link hook for `layer_inspector_show` (Task 7).
- Updated `CMakeLists.txt` and `custom_status_screen.c` to initialize menu controller on screen root.
