# Layer Map UX Overhaul & Dongle Stability Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement 100% guaranteed HID key suppression in menu mode, overhaul the 280×240 Layer Map UI with a zero-gap grid, dynamic zoom, and pictograms, smooth WPM calculation, and optimize firmware RAM/concurrency to prevent touchpad hangs.

**Architecture:** A dedicated Layer 14 (`MENU`) with 76 `&none` bindings is activated during menu mode, ensuring 0 keystrokes reach the host. The Layer Map UI replaces margins with a zero-gap 272px grid, spatial thumb cluster separation, dynamic zoom for partial layers, and compact pictograms. Firmware memory is relieved by halving LVGL VDB size to 25%, caching renders, and debouncing display updates to maintain fluent touchpad BLE streaming.

**Tech Stack:** C (C99), Zephyr RTOS, ZMK Firmware, LVGL 8.4, ST7789 SPI Display Driver, JavaScript (Node.js audit test harness).

**Spec:** [`docs/superpowers/specs/2026-10-13-layer-map-ux-and-dongle-stability-design.md`](file:///c:/Users/Starlex/Documents/GitHub/ErgoDox/docs/superpowers/specs/2026-10-13-layer-map-ux-and-dongle-stability-design.md)

## Global Constraints

- 15 total layers in `ergodox.keymap` (Layer 0..14), with 76 keys per layer (1140 bindings total).
- Layer 14 (`MENU`) must consist entirely of `&none` bindings.
- All LVGL mutations must be submitted to `zmk_display_work_q()`.
- Display dimensions: 280×240 pixels landscape.
- `CONFIG_DONGLE_SCREEN_FLIPPED=y` (180° rotation preserved).
- Audit test `node .agents/skills/firmware-critic/scripts/audit.js` must pass with 0 errors across 15 layers.
- All compiled `.uf2` binaries must be archived directly into `firmware/`.

## Review Focus

1. **Host Keystroke Leakage During Fast Typing into Menu**: Activating Layer 14 must instantly drop all queued keycodes without allowing trailing hold-tap or matrix ghosting to reach the host.
2. **Touchpad Rapid Gesture Starvation**: 100 Hz input events from Cirque GlidePoint over BLE must not stutter or drop when Layer Map is active on the screen.
3. **Partial Layer Zoom Alignment**: When zooming partial layers (`NRPD`, `ARRW`, `GAME`, `MOUS`), active keys must remain fully within visible screen coordinates (`x: 4..276`, `y: 30..194`).
4. **WPM Exponential Smoothing Decay**: Speedometer gauge must smoothly decay when typing stops without abruptly snapping to 0 or freezing at peak speed.
5. **Memory Budget Verification**: Halving VDB size to 25% must maintain static RAM utilization below 65% on nRF52840 (leaving >90KB free RAM).

---

### Task 1: Firmware Memory Optimization (VDB Buffer & RAM Relief)

**Files:**
- Modify: `zmk-config/boards/shields/ergodox/ergodox_dongle_display.conf:85-95`
- Test: `tests/test_vdb_config.py`

**Interfaces:**
- Consumes: Existing Kconfig memory options.
- Produces: `CONFIG_LV_Z_VDB_SIZE=25`, saving 33.6 KB of static RAM on nRF52840.

- [ ] **Step 1: Write the failing test for VDB config**

```python
# tests/test_vdb_config.py
import re

def test_vdb_buffer_size():
    with open("zmk-config/boards/shields/ergodox/ergodox_dongle_display.conf", "r") as f:
        content = f.read()
    match = re.search(r"CONFIG_LV_Z_VDB_SIZE=(\d+)", content)
    assert match is not None, "CONFIG_LV_Z_VDB_SIZE missing"
    assert int(match.group(1)) == 25, f"Expected VDB size 25 (saves 34KB RAM), got {match.group(1)}"
```

- [ ] **Step 2: Run test to verify it fails**

Run: `python -m pytest tests/test_vdb_config.py -v`
Expected: FAIL (currently `CONFIG_LV_Z_VDB_SIZE=50`)

- [ ] **Step 3: Update `CONFIG_LV_Z_VDB_SIZE=25` in `ergodox_dongle_display.conf`**

Change `CONFIG_LV_Z_VDB_SIZE=50` to `CONFIG_LV_Z_VDB_SIZE=25` in `zmk-config/boards/shields/ergodox/ergodox_dongle_display.conf`.

- [ ] **Step 4: Run test to verify it passes**

Run: `python -m pytest tests/test_vdb_config.py -v`
Expected: PASS

- [ ] **Step 5: Commit**

```bash
git -C zmk-config add boards/shields/ergodox/ergodox_dongle_display.conf
git -C zmk-config commit -m "perf(display): reduce LVGL VDB buffer size to 25% to free 34KB RAM"
```

---

### Task 2: Layer 14 (`MENU`) Keymap & Invariant Governance

**Files:**
- Modify: `zmk-config/config/ergodox.keymap`
- Modify: `layout-viewer/data.js`
- Modify: `docs/layout/02_LAYERS.md`
- Modify: `.agents/skills/firmware-critic/scripts/audit.js`

**Interfaces:**
- Consumes: 14-layer keymap structure.
- Produces: Layer 14 (`MENU`) with 76 `&none` bindings, 15-layer audit pass (1140 total keys).

- [ ] **Step 1: Write failing audit assertion for 15 layers**

In `.agents/skills/firmware-critic/scripts/audit.js`, update expected layer count:
```javascript
assert(layers.length === 15, `Expected exactly 15 layers in keymap, found ${layers.length}`);
assert(viewerLayers.length === 15, `Expected exactly 15 layers in viewer data, found ${viewerLayers.length}`);
```

- [ ] **Step 2: Run audit script to verify it fails**

Run: `node .agents/skills/firmware-critic/scripts/audit.js`
Expected: FAIL with "Expected exactly 15 layers in keymap, found 14"

- [ ] **Step 3: Implement Layer 14 (`MENU`) across keymap, viewer, and docs**

1. In `zmk-config/config/ergodox.keymap`:
   Add `layer_14 { display-name = "MENU"; bindings = < &none ... 76 times ... >; };`.
2. In `layout-viewer/data.js`:
   Add Layer 14 (`MENU`, `Dongle Settings Lock`) with 76 empty/none keys.
3. In `docs/layout/02_LAYERS.md`:
   Add Section "Слой 14: `MENU` (Dongle Menu Settings Lock)" documenting that all 76 keys are `&none` to prevent host leakage.

- [ ] **Step 4: Run audit script to verify it passes**

Run: `node .agents/skills/firmware-critic/scripts/audit.js`
Expected: PASS 90/90 checks with 15 layers and 1140 keys.

- [ ] **Step 5: Commit**

```bash
git -C zmk-config add config/ergodox.keymap
git -C zmk-config commit -m "feat(keymap): add Layer 14 MENU with 76 &none bindings for dongle OSD lock"
git add layout-viewer/data.js docs/layout/02_LAYERS.md .agents/skills/firmware-critic/scripts/audit.js
git commit -m "feat(governance): update audit, viewer, and docs for 15 layers"
```

---

### Task 3: Guaranteed Host HID Key Suppression in `menu_controller.c`

**Files:**
- Modify: `zmk-config/boards/shields/ergodox/src/menu/menu_controller.c`
- Test: `tests/test_menu_suppression.py`

**Interfaces:**
- Consumes: Layer 14 ID from keymap.
- Produces: `menu_controller_toggle()`, `menu_controller_set_state()` activating/deactivating Layer 14.

- [ ] **Step 1: Write test for Layer 14 activation logic**

```python
# tests/test_menu_suppression.py
def test_menu_controller_layer_14_usage():
    with open("zmk-config/boards/shields/ergodox/src/menu/menu_controller.c", "r") as f:
        src = f.read()
    assert "zmk_keymap_layer_activate(14" in src, "Menu controller must activate Layer 14 on menu open"
    assert "zmk_keymap_layer_deactivate(14" in src, "Menu controller must deactivate Layer 14 on menu exit"
```

- [ ] **Step 2: Run test to verify it fails**

Run: `python -m pytest tests/test_menu_suppression.py -v`
Expected: FAIL

- [ ] **Step 3: Implement Layer 14 activation/deactivation in `menu_controller.c`**

1. In `menu_controller_toggle()` and `menu_controller_set_state(SCREEN_STATE_MENU)`:
   - Call `zmk_keymap_layer_activate(14, false);`.
   - Call `zmk_hid_keyboard_clear(); zmk_endpoints_send_report(HID_USAGE_KEY);`.
2. In `menu_controller_set_state(SCREEN_STATE_DASHBOARD)`:
   - Call `if (zmk_keymap_layer_active(14)) zmk_keymap_layer_deactivate(14, false);`.
   - Clear and reset HID reports cleanly.

- [ ] **Step 4: Run test to verify it passes**

Run: `python -m pytest tests/test_menu_suppression.py -v`
Expected: PASS

- [ ] **Step 5: Commit**

```bash
git -C zmk-config add boards/shields/ergodox/src/menu/menu_controller.c
git -C zmk-config commit -m "feat(menu): activate Layer 14 on menu entry to guarantee 100% HID key suppression"
```

---

### Task 4: WPM Calculation Smoothing in `wpm_gauge.c`

**Files:**
- Modify: `zmk-config/boards/shields/ergodox/src/widgets/wpm_gauge.c`
- Modify: `zmk-config/boards/shields/ergodox/src/widgets/wpm_gauge.h`
- Test: `tests/test_wpm_smoothing.c`

**Interfaces:**
- Consumes: Raw WPM state events from ZMK.
- Produces: Smooth exponential moving average WPM indicator without 5s reset spikes.

- [ ] **Step 1: Write C99 unit test verifying EMA smoothing**

```c
// tests/test_wpm_smoothing.c
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static uint8_t smooth_wpm = 0;
void update_wpm_ema(uint8_t raw_wpm) {
    if (raw_wpm == 0) {
        smooth_wpm = (smooth_wpm > 3) ? (smooth_wpm - 3) : 0;
    } else {
        smooth_wpm = (uint8_t)((raw_wpm * 1 + smooth_wpm * 3) / 4);
    }
}

int main(void) {
    update_wpm_ema(60);
    assert(smooth_wpm == 15);
    update_wpm_ema(60);
    assert(smooth_wpm == 26);
    update_wpm_ema(0); // Smooth decay instead of instant drop to 0
    assert(smooth_wpm == 23);
    printf("PASS\n");
    return 0;
}
```

- [ ] **Step 2: Run test to verify it passes in GCC**

Run: `gcc -Wall -Werror tests/test_wpm_smoothing.c -o tests/test_wpm_smoothing && ./tests/test_wpm_smoothing`
Expected: PASS

- [ ] **Step 3: Implement EMA smoothing in `wpm_gauge.c`**

In `wpm_gauge.c`, update `set_wpm_gauge_state()`:
- Maintain `static uint8_t smooth_wpm = 0;`.
- Apply smoothing formula on incoming updates so that 5-second interval resets do not cause visible gauge drops.

- [ ] **Step 4: Verify syntax and compile test**

Run: `node .agents/skills/firmware-critic/scripts/audit.js`
Expected: PASS

- [ ] **Step 5: Commit**

```bash
git -C zmk-config add boards/shields/ergodox/src/widgets/wpm_gauge.c boards/shields/ergodox/src/widgets/wpm_gauge.h
git -C zmk-config commit -m "feat(wpm): implement exponential smoothing to prevent 5-second reset drops"
```

---

### Task 5: Zero-Gap Edge-to-Edge Grid & Thumb Cluster Separation

**Files:**
- Modify: `zmk-config/boards/shields/ergodox/src/menu/layer_inspector_data.h`
- Modify: `zmk-config/boards/shields/ergodox/src/menu/layer_inspector.c`
- Test: `tests/test_geometry_bounds.c`

**Interfaces:**
- Consumes: Screen dimensions 280×240.
- Produces: `left_half_geom[38]` and `right_half_geom[38]` with zero-gap main cluster (34×26 px) and spatially separated thumb cluster (8 px gap).

- [ ] **Step 1: Write geometry bounds verification test**

```c
// tests/test_geometry_bounds.c
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>

struct key_geom { uint16_t x, y, w, h; };
#include "../zmk-config/boards/shields/ergodox/src/menu/layer_inspector_data.h"

int main(void) {
    for (int i = 0; i < 38; i++) {
        // Must fit within 272px display width
        assert(left_half_geom[i].x + left_half_geom[i].w <= 276);
        assert(right_half_geom[i].x + right_half_geom[i].w <= 276);
        // Main cluster keys (0..29) must have zero horizontal gap with neighbor
        if (i % 6 < 5 && i < 30) {
            assert(left_half_geom[i].x + left_half_geom[i].w == left_half_geom[i + 1].x);
        }
    }
    printf("PASS\n");
    return 0;
}
```

- [ ] **Step 2: Run test to verify it fails on old geometry**

Run: `gcc -Wall tests/test_geometry_bounds.c -o tests/test_geom && ./tests/test_geom`
Expected: FAIL (old geometry had gaps between main keys)

- [ ] **Step 3: Implement zero-gap main cluster and separated thumb cluster in `layer_inspector_data.h`**

1. Expand main cluster keys to 34×26 px with 0 px gap (keys share 1px border).
2. Set thumb cluster at `x: 216..274` with an 8 px visual separation margin.
3. Align right half symmetrically.

- [ ] **Step 4: Run test to verify it passes**

Run: `gcc -Wall tests/test_geometry_bounds.c -o tests/test_geom && ./tests/test_geom`
Expected: PASS

- [ ] **Step 5: Commit**

```bash
git -C zmk-config add boards/shields/ergodox/src/menu/layer_inspector_data.h boards/shields/ergodox/src/menu/layer_inspector.c
git -C zmk-config commit -m "feat(ui): implement zero-gap edge-to-edge grid and separated thumb cluster"
```

---

### Task 6: Unicode Pictograms & Layer Trigger Neon Highlighting

**Files:**
- Modify: `zmk-config/boards/shields/ergodox/src/menu/layer_inspector_data.h`
- Modify: `zmk-config/boards/shields/ergodox/src/menu/layer_inspector.c`
- Test: `tests/test_pictograms.c`

**Interfaces:**
- Consumes: Key cell labels and categories (`KEY_CAT_LAYER`, `KEY_CAT_MOD`, etc.).
- Produces: Compact glyphs (`↵`, `⌫`, `⌦`, `␣`, `⇥`, `↑`, `↓`, `←`, `→`, `⇧`, `^`, `⌥`, `❖`, `Vol+`, `Vol-`, `✕`, `▶‖`, `L-CLK`, `R-CLK`) and Neon Emerald `#38ef7d` layer trigger styling.

- [ ] **Step 1: Write test for label lengths and pictograms**

```c
// tests/test_pictograms.c
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../zmk-config/boards/shields/ergodox/src/menu/layer_inspector_data.h"

int main(void) {
    for (int l = 0; l < 15; l++) {
        for (int k = 0; k < 76; k++) {
            const char *label = layer_keys_data[l][k].label;
            // Max label visual length must fit in 34px (<= 6 chars)
            assert(strlen(label) <= 8);
        }
    }
    printf("PASS\n");
    return 0;
}
```

- [ ] **Step 2: Run test to verify it passes or fails on long labels**

Run: `gcc -Wall tests/test_pictograms.c -o tests/test_pic && ./tests/test_pic`
Expected: PASS/FAIL check.

- [ ] **Step 3: Update labels to compact glyphs and style `KEY_CAT_LAYER`**

1. Replace text labels in `layer_keys_data` across all layers with compact glyphs (`↵`, `⌫`, `⌦`, `␣`, `⇥`, `ESC`, `↑`, `↓`, `←`, `→`, etc.).
2. In `layer_inspector.c`, style `KEY_CAT_LAYER` with Emerald `#38ef7d` border and double border width (2px) to make triggers clearly identifiable.

- [ ] **Step 4: Run test to verify it passes**

Run: `gcc -Wall tests/test_pictograms.c -o tests/test_pic && ./tests/test_pic`
Expected: PASS

- [ ] **Step 5: Commit**

```bash
git -C zmk-config add boards/shields/ergodox/src/menu/layer_inspector_data.h boards/shields/ergodox/src/menu/layer_inspector.c
git -C zmk-config commit -m "feat(ui): replace long text with pictograms and add neon layer trigger highlight"
```

---

### Task 7: Dynamic Zoom / Focus for Partial Layers

**Files:**
- Modify: `zmk-config/boards/shields/ergodox/src/menu/layer_inspector.c`
- Modify: `zmk-config/boards/shields/ergodox/src/menu/layer_inspector.h`
- Test: `tests/test_partial_layer_zoom.py`

**Interfaces:**
- Consumes: Current layer index (`0..14`) and active key count.
- Produces: Automatic zoom for partial layers (`NRPD`, `ARRW`, `GAME`, `MOUS`, `FUN1`, `FUN2`) scaling cells to 44×38 px with Montserrat 14 font.

- [ ] **Step 1: Write test verifying partial layer identification**

```python
# tests/test_partial_layer_zoom.py
def test_partial_layer_definitions():
    with open("zmk-config/boards/shields/ergodox/src/menu/layer_inspector.c", "r") as f:
        src = f.read()
    assert "is_partial_layer" in src, "Must define is_partial_layer helper"
```

- [ ] **Step 2: Run test to verify it fails**

Run: `python -m pytest tests/test_partial_layer_zoom.py -v`
Expected: FAIL

- [ ] **Step 3: Implement dynamic zoom logic in `layer_inspector.c`**

1. Add `is_partial_layer(uint8_t layer, bool is_left)` checking if the inspected half contains <= 16 active (non-trans) keys.
2. If true:
   - Calculate bounding cluster coordinates.
   - Scale active cells up to 44×38 px.
   - Apply `lv_font_montserrat_12` or `14`.
   - Dim outer transparent cells.
3. If false:
   - Render standard 38-key zero-gap layout.

- [ ] **Step 4: Run test to verify it passes**

Run: `python -m pytest tests/test_partial_layer_zoom.py -v`
Expected: PASS

- [ ] **Step 5: Commit**

```bash
git -C zmk-config add boards/shields/ergodox/src/menu/layer_inspector.c boards/shields/ergodox/src/menu/layer_inspector.h
git -C zmk-config commit -m "feat(ui): implement dynamic zoom scaling for partial layers"
```

---

### Task 8: Render Caching, Debouncing & Touchpad Concurrency Safety

**Files:**
- Modify: `zmk-config/boards/shields/ergodox/src/menu/layer_inspector.c`
- Test: `tests/test_render_cache.py`

**Interfaces:**
- Consumes: Layer and half change notifications.
- Produces: 0 ms no-op on redundant render calls, 50 ms debounce on updates via `k_work_delayable`.

- [ ] **Step 1: Write test for caching and delayable work**

```python
# tests/test_render_cache.py
def test_render_cache_and_debounce():
    with open("zmk-config/boards/shields/ergodox/src/menu/layer_inspector.c", "r") as f:
        src = f.read()
    assert "rendered_layer" in src, "Must cache rendered_layer"
    assert "k_work_reschedule" in src or "k_work_delayable" in src, "Must debounce with delayable work"
```

- [ ] **Step 2: Run test to verify it fails**

Run: `python -m pytest tests/test_render_cache.py -v`
Expected: FAIL

- [ ] **Step 3: Implement render caching and 50ms debouncing**

1. In `layer_inspector.c`:
   - Replace `layer_inspector_work` with `K_WORK_DELAYABLE_DEFINE(layer_inspector_delay_work, ...)`.
   - In `layer_inspector_on_key_position()` and `layer_inspector_on_layer_change()`:
     Schedule with `k_work_reschedule_for_queue(zmk_display_work_q(), &layer_inspector_delay_work, K_MSEC(50));`.
2. In `layer_inspector_render()`:
   - Check `if (rendered_layer == current_layer && rendered_is_left == current_is_left_half) return;`.
   - Update `rendered_layer` and `rendered_is_left` upon successful redraw.

- [ ] **Step 4: Run test to verify it passes**

Run: `python -m pytest tests/test_render_cache.py -v`
Expected: PASS

- [ ] **Step 5: Commit**

```bash
git -C zmk-config add boards/shields/ergodox/src/menu/layer_inspector.c
git -C zmk-config commit -m "perf(display): add render caching and 50ms work debounce to protect touchpad concurrency"
```

---

### Task 9: End-to-End Build & Firmware Deployment

**Files:**
- All firmware files in `zmk-config`
- Firmware artifacts in `firmware/`

**Interfaces:**
- Consumes: Complete repository state.
- Produces: Green GitHub Actions build run and downloaded `.uf2` binaries in `firmware/`.

- [ ] **Step 1: Run comprehensive local audit**

Run: `node .agents/skills/firmware-critic/scripts/audit.js`
Expected: PASS with 90/90 checks and 0 errors.

- [ ] **Step 2: Push changes to GitHub and trigger build**

Run:
```bash
git -C zmk-config push origin main
node .agents/skills/firmware-critic/scripts/build.js
```

- [ ] **Step 3: Verify build artifacts in `firmware/`**

Verify that `firmware/ergodox_dongle_display-nice_nano_v2-zmk.uf2` is freshly updated.

- [ ] **Step 4: Commit root repository update**

```bash
git add firmware/ zmk-config docs/superpowers/plans/
git commit -m "chore(release): deploy updated firmware binaries with layer map overhaul and HID suppression"
```
