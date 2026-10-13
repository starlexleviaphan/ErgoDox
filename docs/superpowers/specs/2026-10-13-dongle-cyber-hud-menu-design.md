# Design Specification: ErgoDox Wireless Dongle Cyber-HUD & Interactive Menu System

**Date:** 2026-10-13  
**Status:** Approved  
**Target:** ZMK Firmware (`zmk-config`), Zephyr RTOS, LVGL 8.x, ST7789V Display, nice!nano v2 (nRF52840)  

---

## 1. Executive Summary & Goal

Upgrade the ErgoDox Wireless central receiver dongle firmware with an interactive, information-rich graphical user interface ("Cyber-HUD") driven by LVGL and Zephyr RTOS on an ST7789V 240×280 color IPS display.

Key deliverables:
1. **180° Screen Orientation Flip:** Fix upside-down display mounting (`CONFIG_DONGLE_SCREEN_FLIPPED=y` by default, configurable at runtime).
2. **Interactive On-Screen Menu System:** Controlled directly from keyboard keys (`W`/`A`/`S`/`D`, `Enter`, `Esc`) with host HID input suppression during menu navigation to prevent accidental typing on the computer.
3. **ErgoDox Layer Map Inspector:** Full-screen visual preview of the ErgoDox 38-key matrix (Left / Right halves) showing active key assignments for the current layer.
4. **Typing-Reactive Animation & Telemetry:** Integrated Bongo Cat mascot (paws tapping in sync with left/right typing events, dynamic mood based on WPM), WPM gauge, Caps Lock badge, colored layer indicators, and 3-channel battery telemetry (`L`, `R`, `TP` - Touchpad).
5. **Persistent Flash Storage:** Retain user display preferences (brightness, sleep timeout, orientation, theme) across power cycles using Zephyr Settings (`nvs`).

---

## 2. Hardware & Firmware Constraints

* **MCU:** Nordic Semiconductor nRF52840 on nice!nano v2.
* **Display:** ST7789V 240×280 IPS LCD over SPI3 (8 MHz), CMD/DATA on P1.04, RST on P0.11, CS on P1.06, PWM Backlight on P0.10.
* **Layout Orientation:** Landscape 280×240 (`CONFIG_DONGLE_SCREEN_HORIZONTAL=y`, with 180° rotation).
* **Peripherals:** 3 wireless splits connected via BLE Central role:
  - Left half (38 keys)
  - Right half (38 keys)
  - Synaptics Touchpad (`TM-P3125`)
* **Firmware Invariants:**
  - 14 layers, strictly 76 keys per layer (1064 bindings).
  - No software bootloaders/system reset keys in keymaps.
  - Strict synchronization between `ergodox.keymap`, `layout-viewer/data.js`, and `docs/layout/02_LAYERS.md`.

---

## 3. Architecture & Subsystems

```
                                  +-----------------------------+
                                  |   BLE Key Events (L/R/TP)   |
                                  +--------------+--------------+
                                                 |
                                                 v
                               +---------------------------------+
                               |    Input Intercept Controller   |
                               +----------------+----------------+
                                                |
                       +------------------------+------------------------+
                       | menu_active == false                            | menu_active == true
                       v                                                 v
           +-----------------------+                         +-----------------------+
           | Standard ZMK HID Pipe |                         | Menu Navigation Engine|
           |   (Sent to PC Host)   |                         |  (WASD / Enter / Esc) |
           +-----------------------+                         +-----------+-----------+
                                                                         |
                                                                         v
                                                       +---------------------------------+
                                                       |   LVGL Screen State Machine     |
                                                       |  - DASHBOARD                    |
                                                       |  - SETTINGS_MENU                |
                                                       |  - LAYER_MAP_INSPECTOR          |
                                                       +----------------+----------------+
                                                                        |
                                                                        v
                                                       +---------------------------------+
                                                       |   Zephyr Settings (Flash NVS)   |
                                                       +---------------------------------+
```

### 3.1 Subsystem 1: Input Intercept & Menu Activation

1. **Trigger Key:**
   - Dedicated key binding `&dongle_menu_toggle` assigned on Layer 6 (`MOUS`) at row 0, position 1 (top-left, next to ESC).
   - Pressing this key toggles the internal state `menu_active`.
2. **Keystroke Interception:**
   - When `menu_active == true`:
     - Keys corresponding to navigation are consumed by the dongle runtime and **suppressed from USB/BLE HID reports**:
       - `W` (Position / Keycode `W` or `UP`) -> Menu cursor UP
       - `S` (Position / Keycode `S` or `DOWN`) -> Menu cursor DOWN
       - `A` (Position / Keycode `A` or `LEFT`) -> Value DECREASE / Previous half
       - `D` (Position / Keycode `D` or `RIGHT`) -> Value INCREASE / Next half
       - `Space` / `Enter` -> Item SELECT / ENTER subscreen
       - `Esc` or `&dongle_menu_toggle` -> CANCEL / EXIT menu back to Dashboard
     - All other keypresses in menu mode are ignored (or suppressed) so no extraneous characters leak to the operating system.

### 3.2 Subsystem 2: Graphical Screen State Machine (LVGL)

The display controller manages three distinct LVGL view states:

#### State A: `DASHBOARD` (Main View)
* **Header Bar (280×28 px):**
  - Left: Connection indicator (`USB` green/white, `BLE` blue/green with profile index 1..5).
  - Center: Lock indicators (`[CAPS]` in bright cyan/amber when Caps Lock or Caps Word active; `[NUM]`).
  - Right: Quick brightness & status glyphs.
* **Hero Section (280×160 px):**
  - **Left Half (Layer Badge & Speed):**
    - High-visibility layer badge displaying layer name (e.g. `WORK`, `TYPE`, `GAME`, `SYM1`, `FUN1`, etc.).
    - Dynamic color coding per layer class (Cyan for WORK/TYPE, Crimson Red for GAME, Violet for SYM, Amber for FUN, Deep Blue for MOUS/ARRW).
    - Numeric WPM readout with dynamic color thresholds (Green >100, Yellow >50, Red <50).
  - **Right Half (Bongo Cat Animation):**
    - Animated 2-paw mascot: Left paw taps down on Left-half key events; Right paw taps down on Right-half key events.
    - Mood expressions: Relaxed (<30 WPM), Energetic (30-70 WPM), Sunglasses / Fire mode (>70 WPM).
* **Footer Bar (280×44 px):**
  - Modifiers row: `Ctrl`, `Shift`, `Alt`, and Windows logo (``) using NerdFonts.
  - 3-Channel Battery Gauges: Horizontal bars and percentage text for `L: XX%`, `R: XX%`, `TP: XX%` with low-battery warning highlighting.

#### State B: `SETTINGS_MENU` (Interactive Config)
* Rendered as a high-contrast styled list with a highlighted selection cursor:
  1. **Brightness:** Slider/stepper (10% to 100%, step 10%).
  2. **Idle Timeout:** Stepper (Off, 1m, 3m, 5m, 10m, 15m).
  3. **Screen Flip:** Toggle (Normal / Inverted 180°).
  4. **Animation:** Selector (Bongo Cat / WPM Gauge / Off).
  5. **Color Theme:** Selector (Cyberpunk / Monokai / Matrix / Clean).
  6. **Layer Map Inspector:** Action -> Navigates to State C.
  7. **Save & Exit:** Action -> Persists settings to flash and returns to Dashboard.

#### State C: `LAYER_MAP_INSPECTOR` (Visual Matrix)
* Full-screen 280×240 schematic of the ErgoDox half (38 keys):
  - Main cluster: 5 rows × 6 columns matrix.
  - Thumb cluster: 6 keys arranged in ErgoDox geometry.
  - Header: Active layer name + active half indicator (`LEFT HALF` ↔ `RIGHT HALF`, switchable via `A`/`D`).
  - Key cells: Rendered with rounded rects colored by key category (Alphas, Modifiers, Layer Transitions, Special/Media, Transparent).
  - Pressing `Esc` or `dongle_menu_toggle` returns to Settings Menu or Dashboard.

### 3.3 Subsystem 3: Persistent Flash Settings

* Subsystem handles persistence using Zephyr Settings API (`CONFIG_SETTINGS=y`).
* Setting key prefix: `ergodox_display/`.
* Managed parameters:
  - `brightness` (uint8_t: 10..100, default 80)
  - `timeout_s` (uint16_t: 0..900, default 600)
  - `flipped` (bool: default true / 180°)
  - `anim_style` (uint8_t: 0=BongoCat, 1=Gauge, 2=None)
  - `theme_id` (uint8_t: 0..3)
* Save triggers on "Save & Exit" menu action to minimize flash write cycles.

---

## 4. Integration & File Layout

* `zmk-config/boards/shields/ergodox/`:
  - `CMakeLists.txt`: Links local source files for custom display, menu, widgets, and fonts.
  - `ergodox_dongle_display.conf`: Kconfig flags (`CONFIG_DONGLE_SCREEN_FLIPPED=y`, `CONFIG_ZMK_DISPLAY=y`, etc.).
  - `src/`:
    - `custom_status_screen.c`: Master screen controller and state machine.
    - `menu/menu_controller.c` & `.h`: Menu state, WASD input routing, and item renderers.
    - `menu/layer_inspector.c` & `.h`: 38-key ErgoDox matrix renderer.
    - `widgets/battery_status.c`: 3-channel battery monitor (`L`, `R`, `TP`).
    - `widgets/bongo_cat.c` & `.h`: Reactive animation engine.
    - `widgets/layer_badge.c`: Color-themed layer indicator.
    - `widgets/lock_status.c`: Caps Lock / Num Lock indicator.
* `zmk-config/config/ergodox.keymap`:
  - Layer 6 (`MOUS`): Assign `&dongle_menu_toggle` to left-hand row 0 position 1.
* `layout-viewer/data.js` & `docs/layout/02_LAYERS.md`:
  - Synchronize metadata for Layer 6 update.

---

## 5. Verification & Testing Strategy

1. **Static Validation Audit:**
   - Execute `node .agents/skills/firmware-critic/scripts/audit.js` to ensure 1064 key bindings, 14 layers, and sync invariants are preserved.
2. **Compilation & Build:**
   - Build using GitHub Actions pipeline script `node .agents/skills/firmware-critic/scripts/build.js`.
   - Verify clean build without warnings on `nice_nano_v2` target with shield `ergodox_dongle_display`.
3. **Functional Testing Checklist:**
   - Display orientation renders right-side up (180° flipped).
   - Pressing Menu Toggle key enters menu; pressing WASD navigates menu and does NOT type letters into PC.
   - Adjusting brightness changes screen backlight immediately.
   - Bongo Cat paws tap reactively upon typing on Left and Right halves.
   - Battery indicators accurately show Left, Right, and Touchpad battery percentages.
   - Layer Map Inspector accurately displays ErgoDox matrix layout.
   - Settings persist after unplugging and replugging USB dongle.
