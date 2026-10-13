# Design Specification: ErgoDox Wireless Dongle Cyber-HUD & Interactive Menu System

**Date:** 2026-10-13  
**Status:** Approved  
**Target:** ZMK Firmware (`zmk-config`), Zephyr RTOS, LVGL 8.x, ST7789V Display, nice!nano v2 (nRF52840)  

---

## 1. Executive Summary & Goal

Upgrade the ErgoDox Wireless central receiver dongle firmware with an interactive, information-rich graphical user interface ("Cyber-HUD") driven by LVGL and Zephyr RTOS on an ST7789V 240×280 color IPS display.

Key deliverables:
1. **180° Screen Orientation Flip:** Fix upside-down display mounting (`CONFIG_DONGLE_SCREEN_FLIPPED=y` by default, configurable at runtime via on-screen menu).
2. **Interactive On-Screen Menu System:** Controlled directly from keyboard keys (`W`/`A`/`S`/`D`, `Enter`, `Esc`) with host HID input suppression during menu navigation to prevent accidental typing on the computer.
3. **Caps Lock & Caps Word Indicators:** Live visual badge for HID lock states.
4. **Color-Themed Layer Badges for All 14 Layers:** Distinct color identities for every layer in the WORK-TYPE-GAME system.
5. **Typing-Reactive Bongo Cat Mascot & WPM Speedometer:** Live 2-paw animation reacting to Left/Right half keystrokes, paired with a dynamic speed gauge replacing plain numeric labels.
6. **Card-Based UI Layout (LVGL Cards):** Structured modular cards with rounded corners and subtle cyber-borders optimized for 280×240 resolution.
7. **ErgoDox Layer Map Inspector:** Full-screen visual preview of the ErgoDox 38-key matrix (Left / Right halves) showing active key assignments for the current layer.
8. **Persistent Flash Storage:** Retain user display preferences (brightness, sleep timeout, orientation, theme) across power cycles using Zephyr Settings (`nvs`).

---

## 2. Hardware & Firmware Constraints

* **MCU:** Nordic Semiconductor nRF52840 on nice!nano v2.
* **Display:** ST7789V 240×280 IPS LCD over SPI3 (8 MHz), CMD/DATA on P1.04, RST on P0.11, CS on P1.06, PWM Backlight on P0.10.
* **Layout Orientation:** Landscape 280×240 (`CONFIG_DONGLE_SCREEN_HORIZONTAL=y`, with 180° rotation via `DISPLAY_ORIENTATION_ROTATED_90`).
* **Peripherals:** 3 wireless splits connected via BLE Central role:
  - Left half (38 keys)
  - Right half (38 keys)
  - Synaptics Touchpad (`TM-P3125`)
* **Firmware Invariants:**
  - 14 layers, strictly 76 keys per layer (1064 bindings).
  - No software bootloaders/system reset keys in keymaps.
  - Strict synchronization between `ergodox.keymap`, `layout-viewer/data.js`, and `docs/layout/02_LAYERS.md`.

---

## 3. UI Layout & Visual Design ("Cyber-HUD")

The ST7789 display runs in horizontal landscape mode (280 px wide × 240 px high). The interface is constructed using LVGL card containers with dark backgrounds (`#0a0d14`), subtle rounded borders (`radius: 6px`, `border_color: #1f293d`, `border_width: 1px`), and high-contrast typography.

```
+-------------------------------------------------------------------------+
| [HEADER CARD]                                                   280x28  |
|  [USB/BLE] Host Status     [CAPS] [NUM] Lock State     [BRI] Brightness |
+-------------------------------------------------------------------------+
| [CENTER-LEFT: LAYER & SPEED]              | [CENTER-RIGHT: BONGO CAT]   |
|  +-------------------------------------+  |  +-----------------------+  |
|  | [LAYER BADGE]                       |  |  |   /\_/\    [WPM ARC]  |  |
|  |  #0 WORK (Cyan Accent)              |  |  |  ( o.o )    84 WPM    |  |
|  |-------------------------------------|  |  |  /|   |\   Speedometer|  |
|  |  SPEED: 84 WPM (Speed Bar Gauge)    |  |  |  ~  ~  ~   (Paws Tap) |  |
|  +-------------------------------------+  |  +-----------------------+  |
|                                145x140 px |                  125x140 px |
+-------------------------------------------------------------------------+
| [FOOTER CARD: MODIFIERS & 3-CHANNEL BATTERY]                    280x62  |
|  MODS: [Ctrl] [Shift] [Alt] [Win ⊞]                                     |
|  BATT:  L [====  ] 78%    R [======] 95%    TP [===   ] 60%             |
+-------------------------------------------------------------------------+
```

### 3.1 180° Screen Orientation Flip
* **Default state:** `CONFIG_DONGLE_SCREEN_FLIPPED=y` sets `display_set_orientation(display, DISPLAY_ORIENTATION_ROTATED_90)` (compared to `ROTATED_270`), rotating the image 180 degrees so the display is right-side up.
* **Runtime flipping:** The Settings Menu includes a "Screen Flip" toggle which can invert the orientation at runtime and persist the value to Flash.

### 3.2 Caps Lock & Caps Word Indicator Badge
* Subscribes to ZMK HID indicators (`zmk_hid_indicators_process_report` / `zmk_hid_indicators_get()`).
* Also subscribes to `zmk_caps_word_state_changed` event.
* **Rendering:**
  - Inactive: Dim outline badge or hidden.
  - Active: Bold illuminated badge `[CAPS]` with neon-amber or bright cyan glow (`#00e5ff` / `#ffb300`).

### 3.3 Color-Themed Layer Badges for All 14 Layers
Each of the 14 layers in the ErgoDox firmware receives a dedicated visual identity with a color-coded badge, accent border, and description:

| Index | Name | Function / Purpose | Accent Color (Hex) | Color Style |
|:---:|:---:|:---|:---:|:---|
| **0** | `WORK` | Main programming layer with Home Row Mods | `#00d2ff` | Cyber Cyan |
| **1** | `SWAP` | Left-hand mirrored alphas | `#38ef7d` | Mint Emerald |
| **2** | `LNPD` | Left numpad cluster | `#f9d423` | Warm Gold |
| **3** | `TYPE` | Typing mode (no HRM, raw typing speed) | `#00f2fe` | Electric Aqua |
| **4** | `SYM1` | Primary symbols & language hotkeys | `#a18cd1` | Soft Lavender |
| **5** | `NRPD` | Right numpad cluster | `#ff9a44` | Amber Orange |
| **6** | `MOUS` | Mouse keys & dongle menu trigger | `#4facfe` | Sky Blue |
| **7** | `ARRW` | Navigation arrows & editing cluster | `#43e97b` | Neon Green |
| **8** | `GAME` | WASD gaming mode (0ms latency, no HRM) | `#ff0844` | Crimson Red |
| **9** | `GSWP` | Gaming swap layer | `#f857a6` | Deep Pink |
| **10** | `NPAD` | Standard 3x4 numeric keypad | `#f6d365` | Sunny Yellow |
| **11** | `FUN1` | Function keys F1–F12 | `#b721ff` | Purple Violet |
| **12** | `FUN2` | Extended function keys F13–F24 | `#6f86d6` | Royal Indigo |
| **13** | `SYM2` | Wireless dongle & Bluetooth management | `#f355da` | Neon Magenta |

### 3.4 Bongo Cat Mascot & Dynamic WPM Speedometer
* **Bongo Cat Mascot Widget:**
  - 2-paw animated pixel art / vector drawing rendered via LVGL canvas/line primitives.
  - **Left Paw:** Listens to `zmk_position_state_changed`. If the pressed key position belongs to the Left half (`0..6, 14..20, 28..34, 42..47, 54..58, 64..65, 68..70, 74`), the left paw drops onto the desk.
  - **Right Paw:** If the pressed key position belongs to the Right half (`7..13, 21..27, 35..41, 48..53, 59..63, 66..67, 71..73, 75`), the right paw drops.
  - **Typing Expressions:**
    - `WPM < 30`: Idle / gentle blinking face.
    - `30 <= WPM < 70`: Excited typing eyes.
    - `WPM >= 70`: Sunglasses / flame particles ("Speed Demon" mode).
* **WPM Speedometer Gauge:**
  - Replaces the plain text number with a dynamic arc gauge / segmented bar.
  - Dynamic color thresholds:
    - Low (`0 - 45 WPM`): Blue/Cyan `#00d2ff`
    - Medium (`46 - 80 WPM`): Green/Yellow `#76ff03`
    - High (`81 - 110 WPM`): Orange `#ff9100`
    - Ultra (`111+ WPM`): Blazing Red `#ff1744`

### 3.5 3-Channel Split Battery Gauges
* Monitors all 3 wireless splits via our custom `battery_status.c`:
  - `L` — Left Half
  - `R` — Right Half
  - `TP` — Synaptics Touchpad
* Each battery bar features anti-aliased terminal nubs, percentage text, and color thresholding (Green >50%, Yellow 15-50%, Red <15%).

---

## 4. Interactive Menu System (WASD Navigation)

```
                                +-----------------------------+
                                |  Press Menu Toggle on MOUS  |
                                +--------------+--------------+
                                               |
                                               v
                        +---------------------------------------------+
                        |           SETTINGS MENU ACTIVE              |
                        |      (Host HID Input Suppressed)            |
                        |                                             |
                        |   > 1. Brightness:      [ 80% ]             |
                        |     2. Idle Timeout:    [ 10m ]             |
                        |     3. Orientation:     [ Flipped 180° ]    |
                        |     4. Animation:       [ Bongo Cat ]       |
                        |     5. Color Theme:     [ Cyberpunk ]       |
                        |     6. Layer Map:       [ View Matrix -> ]  |
                        |     7. Save & Exit                          |
                        +---------------------------------------------+
                                    |              |
                      W/S (Navigate)|              | A/D (Adjust)
                                    v              v
                        Select Subscreen / Change Values
```

### 4.1 Menu Intercept & Host Typing Suppression
1. **Toggle Key:**
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
     - All other keypresses in menu mode are suppressed so no characters leak into the active computer window.

### 4.2 Layer Map Inspector
* Full-screen 280×240 schematic of the ErgoDox half (38 keys):
  - Main cluster: 5 rows × 6 columns matrix.
  - Thumb cluster: 6 keys arranged in ErgoDox geometry.
  - Header: Active layer name + active half indicator (`LEFT HALF` ↔ `RIGHT HALF`, switchable via `A`/`D`).
  - Key cells: Rendered with rounded rects colored by key category (Alphas, Modifiers, Layer Transitions, Special/Media, Transparent).
  - Pressing `Esc` or `dongle_menu_toggle` returns to Settings Menu or Dashboard.

---

## 5. Persistent Flash Settings

* Managed parameters via Zephyr Settings (`ergodox_display/`):
  - `brightness` (uint8_t: 10..100, default 80)
  - `timeout_s` (uint16_t: 0..900, default 600)
  - `flipped` (bool: default true / 180°)
  - `anim_style` (uint8_t: 0=BongoCat, 1=Gauge, 2=None)
  - `theme_id` (uint8_t: 0..3)
* Save triggers on "Save & Exit" menu action to protect flash write endurance.

---

## 6. Integration & File Layout

* `zmk-config/boards/shields/ergodox/`:
  - `CMakeLists.txt`: Links local source files for custom display, menu, widgets, and fonts.
  - `ergodox_dongle_display.conf`: Kconfig flags (`CONFIG_DONGLE_SCREEN_FLIPPED=y`, `CONFIG_ZMK_DISPLAY=y`, etc.).
  - `src/`:
    - `custom_status_screen.c`: Master screen controller, LVGL cards layout, and state machine.
    - `menu/menu_controller.c` & `.h`: Menu state, WASD input routing, and item renderers.
    - `menu/layer_inspector.c` & `.h`: 38-key ErgoDox matrix renderer.
    - `widgets/battery_status.c`: 3-channel battery monitor (`L`, `R`, `TP`).
    - `widgets/bongo_cat.c` & `.h`: Reactive animation engine with paw tapping.
    - `widgets/wpm_gauge.c` & `.h`: Color-shifting WPM speedometer arc.
    - `widgets/layer_badge.c`: Color-themed badge for all 14 layers.
    - `widgets/lock_status.c`: Caps Lock / Caps Word badge.
* `zmk-config/config/ergodox.keymap`:
  - Layer 6 (`MOUS`): Assign `&dongle_menu_toggle` to left-hand row 0 position 1.
* `layout-viewer/data.js` & `docs/layout/02_LAYERS.md`:
  - Synchronize metadata for Layer 6 update.

---

## 7. Verification & Testing Strategy

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
