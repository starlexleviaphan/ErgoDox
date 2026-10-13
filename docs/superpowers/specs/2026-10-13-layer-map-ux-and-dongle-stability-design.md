# Design Specification: Layer Map UX Overhaul & Dongle Stability

- **Date**: 2026-10-13
- **Author**: Antigravity & User Pair Programming
- **Status**: Approved by User, Ready for Planning

---

## 1. Overview & Objectives

This design addresses four critical issues on the ErgoDox Wireless Dongle Cyber-HUD (280×240 ST7789 display):

1. **Guaranteed Host HID Suppression in Menu Mode**: Eliminate keystroke leakage to the host PC when navigating the dongle menu.
2. **Layer Map UX & Readability Overhaul**:
   - Zero-gap seamless grid on main cluster to maximize key label area.
   - Spatial visual separation of thumb cluster.
   - Edge-to-edge layout utilization (272 px width).
   - Dynamic Zoom / Focus for partial layers (Numpad, Navigation, WASD, Mouse, Function keys).
   - Intuitive pictograms/glyphs replacing long text (`↵`, `⌫`, `⌦`, `␣`, `⇥`, `↑`, `↓`, `←`, `→`, etc.).
   - Visual highlighting of Layer Trigger keys (`KEY_CAT_LAYER`).
3. **Accurate & Smooth WPM Calculation**: Fix rapid 5-second reset spikes by implementing a smooth rolling average based on standard typing metrics (5 strokes = 1 word).
4. **Dongle Stability & Touchpad Concurrency**: Prevent freezes during high-rate Cirque GlidePoint touchpad movements by halving LVGL VDB RAM consumption, adding a render cache, and debouncing display updates.

---

## 2. Architecture & Detailed Specifications

### 2.1. Dedicated Layer 14 (`MENU`) for 100% HID Key Suppression

#### Root Cause
In ZMK, core application listeners (`app/src/keymap.c`) run before shield listeners in the linker section. Keypresses are processed and sent to the host via USB HID before `menu_controller.c` can intercept them.

#### Architectural Solution
- In `zmk-config/config/ergodox.keymap`, add **Layer 14 (`MENU`)** containing 76 `&none` bindings.
- In `menu_controller.c`:
  - When opening Menu (`menu_controller_toggle` or F21):
    ```c
    zmk_keymap_layer_activate(14, false);
    ```
  - When closing Menu (`menu_controller_exit` / Save & Exit / Esc):
    ```c
    zmk_keymap_layer_deactivate(14, false);
    ```
- **Behavior**:
  - Because Layer 14 is the highest active layer, any keypress resolves to `&none` (opaque behavior).
  - `keymap.c` emits **0 HID packets**. Zero keys leak to the host PC.
  - `menu_controller.c` receives the raw matrix position event (`POS_W`, `POS_S`, `POS_A`, `POS_D`, `POS_ENTER`, `POS_ESC`) and executes menu navigation.
  - On reboot or disconnect, layers naturally reset to Layer 0. No flash corruption risk.

---

### 2.2. Screen Real Estate, Zero-Gap Grid & Thumb Cluster Separation

#### Geometry Specification on 280×240 Display
* Available display area: `x: 4..276` (272 px wide), `y: 30..194` (164 px high on Dashboard) or `y: 26..214` (188 px high in Fullscreen Menu).
* **Main Cluster (5 rows × 6 columns = 30 keys)**:
  - **Zero-gap seamless grid**: Keys touch each other with a crisp 1px shared border, eliminating wasted margins.
  - Main key size: **34 px width × 26 px height** (increased from 26×20 px, +40% label area!).
  - Main cluster bounding box: `x: 6..210`, `y: 32..162`.
* **Thumb Cluster (6 keys)**:
  - Visually separated from the main cluster with a distinct 8 px spatial gap and subtle accent backing.
  - 2 vertical 1.5u thumb keys (32×42 px) and 4 1u thumb keys (28×24 px).
  - Thumb cluster bounding box: `x: 216..274`, `y: 90..190`.

---

### 2.3. Dynamic Zoom / Focus for Partial Layers

Several layers define only a small subset of active keys, with all other keys being transparent (`&trans`):
- **Layer 5 (`NRPD`) Right Half**: 3×4 Numpad grid + operators.
- **Layer 5 (`NRPD`) Left Half**: 3×3 Arrow / Navigation & Editing cluster.
- **Layer 6 (`MOUS`)**: Mouse buttons & cursor cluster.
- **Layer 7 (`ARRW`)**: Directional navigation cluster.
- **Layer 8 (`GAME`) Left Half**: WASD movement cluster.
- **Layer 10 (`NPAD`)**: Dedicated numpad.
- **Layer 11 (`FUN1`) & Layer 12 (`FUN2`)**: 3×4 Function key blocks.

#### Zoom Mode Behavior
* When a partial layer is active on the displayed half:
  - Detect active bounding box of non-trans keys.
  - Scale cell dimensions up to **44 px × 38 px**!
  - Switch font to **Montserrat 14** (large, high-legibility).
  - Dim or hide outer transparent keys.
* When a full layout layer is active (e.g. `WORK`, `SWAP`, `TYPE`, `SYM1`, `SYM2`):
  - Display the full 38-key zero-gap layout.

---

### 2.4. Typography & Intuitive Pictograms

Long strings are replaced with standard, instantly recognizable compact symbols:

| Key Name | Label / Pictogram |
|---|---|
| Enter / Return | `↵` |
| Backspace | `⌫` |
| Delete | `⌦` |
| Space | `␣` |
| Tab | `⇥` |
| Escape | `ESC` |
| Arrow Up / Down / Left / Right | `↑`, `↓`, `←`, `→` |
| Shift / Ctrl / Alt / Win | `⇧`, `^`, `⌥`, `❖` |
| Volume Up / Down / Mute | `Vol+`, `Vol-`, `✕` |
| Media Play / Pause | `▶‖` |
| Mouse Left / Right Click | `L-CLK`, `R-CLK` |

#### Visual Highlighting for Layer Trigger Keys
- Keys categorized as `KEY_CAT_LAYER` receive:
  - Vibrant Emerald `#38ef7d` border.
  - Distinct layer badge marker (e.g. `▲ L4` or `▲ SYM`).

---

### 2.5. Accurate & Smooth WPM Calculation

#### Problem
ZMK's `wpm.c` divides key counts over a 5-second interval and resets the counter to zero every 5 seconds, causing jarring speed drops and spikes that look like raw CPM.

#### Solution
* In `wpm_gauge.c`:
  - Calculate typing speed using a rolling exponential moving average (EMA):
    $$\text{WPM}_{\text{smooth}} = \alpha \cdot \text{WPM}_{\text{current}} + (1 - \alpha) \cdot \text{WPM}_{\text{prev}} \quad (\alpha = 0.25)$$
  - Standard metric: 5 keystrokes per word.
  - Smooth animation on the arc indicator without jarring jumps to zero.

---

### 2.6. Dongle Stability & Touchpad Concurrency Optimization

#### Problem
The dongle hung during Cirque GlidePoint touchpad swipes due to RAM saturation (76.5% used, only 60KB remaining) and display workqueue starvation caused by repeated 38-object rendering over 8 MHz SPI.

#### Solutions
1. **Reduce VDB Framebuffer**:
   - Change `CONFIG_LV_Z_VDB_SIZE` from `50` to `25` in `ergodox_dongle_display.conf`.
   - Halves VDB buffer from 67.2 KB to 33.6 KB, immediately freeing **~34 KB of RAM**!
   - System RAM usage drops from 76.5% down to **63%**, leaving ample headroom for Bluetooth packets and touchpad DMA.
2. **Render Caching in `layer_inspector.c`**:
   - Track `static uint8_t rendered_layer` and `static bool rendered_is_left`.
   - If `layer == rendered_layer && is_left == rendered_is_left`, return immediately in 0 ms without touching LVGL.
3. **Debounced Work Queue Submission**:
   - Replace raw `k_work` with `struct k_work_delayable` using a 50 ms debounce for half-toggling and layer updates.
   - Prevents re-render storms during rapid typing or simultaneous touchpad gesturing.

---

## 3. Governance, Invariants & Verification

1. **Layer Count & Matrix Parity**:
   - `ergodox.keymap` updated to 15 layers (0..14).
   - `layout-viewer/data.js` updated to 15 layers.
   - `docs/layout/02_LAYERS.md` updated with Layer 14 (`MENU`).
   - `.agents/skills/firmware-critic/scripts/audit.js` updated to enforce 15 layers (1140 bindings total) with 100% parity across all 90+ checks.
2. **Firmware Compilation**:
   - Build on GitHub Actions via `node .agents/skills/firmware-critic/scripts/build.js`.
   - Download `.uf2` binaries directly into `firmware/`.
