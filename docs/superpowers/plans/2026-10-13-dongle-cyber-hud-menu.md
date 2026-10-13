# ErgoDox Dongle Cyber-HUD & Interactive Menu Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Transform the ErgoDox Wireless central receiver dongle firmware into an interactive, information-rich graphical Cyber-HUD featuring 180° flipped display, 14 color-coded layer badges, Caps Lock indicators, typing-reactive Bongo Cat mascot, WPM speedometer, LVGL cards layout, an in-firmware WASD-navigated settings menu with host HID suppression, an ErgoDox 38-key layer inspector, and persistent Flash storage.

**Architecture:** A modular C/LVGL graphics and input intercept engine hosted directly within `boards/shields/ergodox/src/` overriding the default YADS status screen. An input listener intercepts key positions for WASD menu navigation while suppressing HID transmission to the host during active menu mode, while Zephyr Settings persists user preferences across reboots.

**Tech Stack:** ZMK Firmware, Zephyr RTOS 3.5+, LVGL 8.x, Nordic nRF52840, ST7789V Display Driver over SPI, Node.js audit & build pipeline.

**Spec:** [docs/superpowers/specs/2026-10-13-dongle-cyber-hud-menu-design.md](file:///c:/Users/Starlex/Documents/GitHub/ErgoDox/docs/superpowers/specs/2026-10-13-dongle-cyber-hud-menu-design.md)

---

## Global Constraints

* Display resolution is 280×240 in landscape mode with ST7789V controller.
* 14 layers, strictly 76 keys per layer (1064 bindings).
* 100% mutual synchronization between `ergodox.keymap`, `layout-viewer/data.js`, and `docs/layout/02_LAYERS.md`.
* No software bootloader or system reset bindings (`&bootloader` / `&sys_reset`).
* All fresh `.uf2` binaries must be downloaded directly into `firmware/`.
* Validation script `node .agents/skills/firmware-critic/scripts/audit.js` must pass with 0 errors after any keymap or configuration change.

---

## Review Focus

1. **Host HID Leakage:** In Menu mode, ensure navigation keys (W, A, S, D, Space, Enter, Esc) never emit characters to the connected USB/BLE computer.
2. **Peripheral Role Deductions:** Ensure our 3-channel battery monitor (`L`, `R`, `TP`) continues to distinguish the Left half, Right half, and Synaptics Touchpad without collision.
3. **Display Orientation:** Verify that `DISPLAY_ORIENTATION_ROTATED_90` is correctly applied so the screen is right-side up.
4. **Memory Footprint:** Keep LVGL canvas buffers and widgets within nRF52840 SRAM headroom without causing stack overflow on the display work queue thread.
5. **Flash Endurance:** Settings must only be written to NVS when the user explicitly triggers "Save & Exit", never on intermediate slider movements.

---

## Task Structure

### Task 1: Screen Orientation Flip & Base Kconfig Configuration

**Files:**
- Modify: `zmk-config/boards/shields/ergodox/ergodox_dongle_display.conf:60-72`

**Interfaces:**
- Consumes: YADS Kconfig symbols `CONFIG_DONGLE_SCREEN_FLIPPED`, `CONFIG_DONGLE_SCREEN_SYSTEM_ICON`
- Produces: Base configuration flipping display 180° and setting Windows logo icon for GUI

- [ ] **Step 1: Update `ergodox_dongle_display.conf`**
  - Set `CONFIG_DONGLE_SCREEN_FLIPPED=y`
  - Set `CONFIG_DONGLE_SCREEN_SYSTEM_ICON=2` (Windows ⊞ logo)
  - Ensure `CONFIG_ZMK_HID_INDICATORS=y`

- [ ] **Step 2: Run validation audit**
  - Command: `node .agents/skills/firmware-critic/scripts/audit.js`
  - Expected: PASS with 0 errors

- [ ] **Step 3: Commit**
  - Command: `git add zmk-config/boards/shields/ergodox/ergodox_dongle_display.conf && git commit -m "feat(dongle): enable 180 degree screen flip and windows system icon"`

---

### Task 2: Keymap Menu Trigger Binding & 3-Way Sync

**Files:**
- Modify: `zmk-config/config/ergodox.keymap:205-220` (Layer 6 `MOUS`)
- Modify: `layout-viewer/data.js`
- Modify: `docs/layout/02_LAYERS.md`

**Interfaces:**
- Consumes: Custom behavior `&dongle_menu_toggle`
- Produces: Trigger key on Layer 6 (`MOUS`) Row 0 Key 1 (left side next to ESC) for activating/deactivating the on-screen dongle menu

- [ ] **Step 1: Declare menu toggle behavior and bind to Layer 6 (`MOUS`)**
  - Add behavior `dongle_menu_toggle` in `behaviors` block of `ergodox.keymap`
  - Replace position 1 on Layer 6 (currently `&trans`) with `&dongle_menu_toggle`
  - Update `layout-viewer/data.js` and `docs/layout/02_LAYERS.md` to reflect `&dongle_menu_toggle` on Layer 6

- [ ] **Step 2: Run validation audit**
  - Command: `node .agents/skills/firmware-critic/scripts/audit.js`
  - Expected: PASS with 1064 bindings matching 1:1 across keymap, viewer, and docs

- [ ] **Step 3: Commit**
  - Command: `git add zmk-config/config/ergodox.keymap layout-viewer/data.js docs/layout/02_LAYERS.md && git commit -m "feat(keymap): bind dongle menu toggle on MOUS layer with 100% sync"`

---

### Task 3: Caps Lock & 14-Layer Themed Badge Widgets

**Files:**
- Create: `zmk-config/boards/shields/ergodox/src/widgets/lock_status.h`
- Create: `zmk-config/boards/shields/ergodox/src/widgets/lock_status.c`
- Create: `zmk-config/boards/shields/ergodox/src/widgets/layer_badge.h`
- Create: `zmk-config/boards/shields/ergodox/src/widgets/layer_badge.c`

**Interfaces:**
- Consumes: `zmk_hid_indicators_changed`, `zmk_caps_word_state_changed`, `zmk_layer_state_changed`
- Produces: `zmk_widget_lock_status_init()`, `zmk_widget_layer_badge_init()`, 14-layer palette lookup

- [ ] **Step 1: Implement `lock_status.c` & `lock_status.h`**
  - Subscribes to `zmk_hid_indicators_changed` and `zmk_caps_word_state_changed`
  - Renders `[CAPS]` illuminated badge when Caps Lock or Caps Word is active

- [ ] **Step 2: Implement `layer_badge.c` & `layer_badge.h`**
  - Table of 14 layers with custom HEX colors: WORK (`#00d2ff`), SWAP (`#38ef7d`), LNPD (`#f9d423`), TYPE (`#00f2fe`), SYM1 (`#a18cd1`), NRPD (`#ff9a44`), MOUS (`#4facfe`), ARRW (`#43e97b`), GAME (`#ff0844`), GSWP (`#f857a6`), NPAD (`#f6d365`), FUN1 (`#b721ff`), FUN2 (`#6f86d6`), SYM2 (`#f355da`)
  - Renders high-visibility colored badge container with layer name and accent border

- [ ] **Step 3: Commit**
  - Command: `git add zmk-config/boards/shields/ergodox/src/widgets/lock_status.* zmk-config/boards/shields/ergodox/src/widgets/layer_badge.* && git commit -m "feat(widgets): add lock status indicator and 14-layer color badge"`

---

### Task 4: Typing-Reactive Bongo Cat Mascot & WPM Speedometer

**Files:**
- Create: `zmk-config/boards/shields/ergodox/src/widgets/bongo_cat.h`
- Create: `zmk-config/boards/shields/ergodox/src/widgets/bongo_cat.c`
- Create: `zmk-config/boards/shields/ergodox/src/widgets/wpm_gauge.h`
- Create: `zmk-config/boards/shields/ergodox/src/widgets/wpm_gauge.c`

**Interfaces:**
- Consumes: `zmk_position_state_changed` (detects Left half vs Right half key presses), `zmk_wpm_state_changed`
- Produces: `zmk_widget_bongo_cat_init()`, `zmk_widget_wpm_gauge_init()`

- [ ] **Step 1: Implement `bongo_cat.c` & `bongo_cat.h`**
  - LVGL canvas rendering for 2-paw Bongo Cat
  - Left paw taps on left-half key positions (0..6, 14..20, 28..34, 42..47, 54..58, 64..65, 68..70, 74)
  - Right paw taps on right-half key positions (7..13, 21..27, 35..41, 48..53, 59..63, 66..67, 71..73, 75)
  - Dynamic facial expression adapting to WPM (>30 animated eyes, >70 sunglasses / speed demon)

- [ ] **Step 2: Implement `wpm_gauge.c` & `wpm_gauge.h`**
  - LVGL arc/segmented meter indicating typing speed (0 to 120 WPM)
  - Color zones: Cyan (<45), Green (46-80), Orange (81-110), Red (>110)

- [ ] **Step 3: Commit**
  - Command: `git add zmk-config/boards/shields/ergodox/src/widgets/bongo_cat.* zmk-config/boards/shields/ergodox/src/widgets/wpm_gauge.* && git commit -m "feat(widgets): add reactive bongo cat mascot and wpm speedometer gauge"`

---

### Task 5: Modular LVGL Card-Based Dashboard ("Cyber-HUD")

**Files:**
- Create: `zmk-config/boards/shields/ergodox/src/custom_status_screen.c`
- Modify: `zmk-config/boards/shields/ergodox/CMakeLists.txt`

**Interfaces:**
- Consumes: Widgets from Tasks 3 & 4, custom `battery_status.c` (3 channels: L, R, TP), YADS `output_status` & `mod_status`
- Produces: `zmk_display_status_screen()` entry point rendering modular 280×240 LVGL cards

- [ ] **Step 1: Implement card-based container layout in `custom_status_screen.c`**
  - Header Card (280×28 px): Output status, Lock status badge (`[CAPS]`), and status icons
  - Center Left Card (145×140 px): Layer color badge + Speed bar gauge
  - Center Right Card (125×140 px): Bongo Cat mascot + WPM gauge
  - Footer Card (280×62 px): Modifier icons + 3-channel Battery monitor (`L`, `R`, `TP`)
  - Dark container styling (`#0a0d14`), rounded borders (`radius: 6px`, `border_color: #1f293d`, `border_width: 1px`)

- [ ] **Step 2: Update `CMakeLists.txt`**
  - Replace `${DONGLE_SCREEN_SRC_DIR}/src/custom_status_screen.c` with local `src/custom_status_screen.c`
  - Link new widget sources (`lock_status.c`, `layer_badge.c`, `bongo_cat.c`, `wpm_gauge.c`)

- [ ] **Step 3: Commit**
  - Command: `git add zmk-config/boards/shields/ergodox/src/custom_status_screen.c zmk-config/boards/shields/ergodox/CMakeLists.txt && git commit -m "feat(display): implement Cyber-HUD card-based status screen for 280x240 display"`

---

### Task 6: Interactive Menu Controller & Input Intercept Engine

**Files:**
- Create: `zmk-config/boards/shields/ergodox/src/menu/menu_controller.h`
- Create: `zmk-config/boards/shields/ergodox/src/menu/menu_controller.c`
- Modify: `zmk-config/boards/shields/ergodox/CMakeLists.txt`

**Interfaces:**
- Consumes: Key position events, `dongle_menu_toggle` signal
- Produces: WASD navigation menu with on-screen settings (Brightness, Timeout, Flip, Animation, Theme, Layer Map, Save & Exit) and host HID report suppression

- [ ] **Step 1: Implement `menu_controller.c` & `menu_controller.h`**
  - State machine: `SCREEN_STATE_DASHBOARD`, `SCREEN_STATE_MENU`, `SCREEN_STATE_LAYER_MAP`
  - Position listener: intercepts keys when `menu_active == true`
  - Suppresses HID transmission to host computer during menu mode
  - Navigation: `W` (Up), `S` (Down), `A` (Decrease), `D` (Increase), `Enter` (Select), `Esc` / Toggle (Exit)
  - Interactive menu list with highlighted cursor

- [ ] **Step 2: Link `menu_controller.c` in `CMakeLists.txt`**
  - Add `src/menu/menu_controller.c` to `zephyr_library_sources`

- [ ] **Step 3: Commit**
  - Command: `git add zmk-config/boards/shields/ergodox/src/menu/menu_controller.* zmk-config/boards/shields/ergodox/CMakeLists.txt && git commit -m "feat(menu): implement interactive WASD menu controller with HID suppression"`

---

### Task 7: ErgoDox 38-Key Layer Map Inspector

**Files:**
- Create: `zmk-config/boards/shields/ergodox/src/menu/layer_inspector.h`
- Create: `zmk-config/boards/shields/ergodox/src/menu/layer_inspector.c`
- Modify: `zmk-config/boards/shields/ergodox/CMakeLists.txt`

**Interfaces:**
- Consumes: Active layer index, ErgoDox key matrix geometry (38 keys per half)
- Produces: `layer_inspector_show(uint8_t layer, bool is_left_half)`, subscreen navigation

- [ ] **Step 1: Implement `layer_inspector.c` & `layer_inspector.h`**
  - Renders 5×6 main cluster + 6-key thumb cluster in ErgoDox layout geometry
  - Color-codes key cells by category (Alphas, Modifiers, Layer Switch, Media, Transparent)
  - `A` / `D` toggles between LEFT HALF and RIGHT HALF
  - `Esc` or Toggle returns to Settings Menu / Dashboard

- [ ] **Step 2: Link `layer_inspector.c` in `CMakeLists.txt`**
  - Add `src/menu/layer_inspector.c` to `zephyr_library_sources`

- [ ] **Step 3: Commit**
  - Command: `git add zmk-config/boards/shields/ergodox/src/menu/layer_inspector.* zmk-config/boards/shields/ergodox/CMakeLists.txt && git commit -m "feat(menu): implement ErgoDox 38-key layer map inspector"`

---

### Task 8: Flash Settings Persistence (Zephyr Settings)

**Files:**
- Modify: `zmk-config/boards/shields/ergodox/src/menu/menu_controller.c`
- Modify: `zmk-config/boards/shields/ergodox/ergodox_dongle_display.conf`

**Interfaces:**
- Consumes: Zephyr `settings_save_one()`, `SETTINGS_STATIC_HANDLER_DEFINE`
- Produces: Persistent retention of brightness, timeout, flip, anim_style, theme

- [ ] **Step 1: Implement settings handler in `menu_controller.c`**
  - Register `SETTINGS_STATIC_HANDLER_DEFINE(ergodox_display, "ergodox_display", NULL, display_settings_set, NULL, NULL)`
  - Load saved parameters on boot
  - Save parameters to Flash NVS only on "Save & Exit" action

- [ ] **Step 2: Ensure `CONFIG_SETTINGS=y` in `ergodox_dongle_display.conf`**

- [ ] **Step 3: Commit**
  - Command: `git add zmk-config/boards/shields/ergodox/src/menu/menu_controller.c zmk-config/boards/shields/ergodox/ergodox_dongle_display.conf && git commit -m "feat(settings): add persistent flash storage for dongle display parameters"`

---

### Task 9: End-to-End Validation & GitHub Actions Build

**Files:**
- Verify: `firmware/ergodox_dongle_display-nice_nano_v2-zmk.uf2`

**Interfaces:**
- Consumes: `node .agents/skills/firmware-critic/scripts/audit.js`, `node .agents/skills/firmware-critic/scripts/build.js`
- Produces: Compiled `.uf2` binaries downloaded into `firmware/`

- [ ] **Step 1: Run firmware validation audit**
  - Command: `node .agents/skills/firmware-critic/scripts/audit.js`
  - Expected: PASS with 0 errors

- [ ] **Step 2: Trigger GitHub Actions build & download pipeline**
  - Command: `node .agents/skills/firmware-critic/scripts/build.js "feat: add dongle cyber-hud, 180 flip, bongo cat, and wasd menu"`
  - Expected: Clean build on GitHub Actions, automatic download of UF2 files to `firmware/`

- [ ] **Step 3: Verify binary presence & sizes**
  - Check `firmware/ergodox_dongle_display-nice_nano_v2-zmk.uf2`
  - Verify size > 100 KB and git status clean
