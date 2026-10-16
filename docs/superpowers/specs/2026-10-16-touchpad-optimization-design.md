# Architectural Design: Synaptics TM-P3125 Touchpad Firmware Optimization & Complete Multi-Finger Gesture Engine

- **Date:** 2026-10-16
- **Status:** Draft for Review
- **Target Platform:** ZMK Firmware / Zephyr RTOS (`nice!nano v2`, `nRF52840`)
- **Affected Components:**
  - `zmk-config/drivers/input/input_synaptics_tm_p3125.c`
  - `zmk-config/boards/shields/ergodox/ergodox_touchpad.conf`
  - `zmk-config/boards/shields/ergodox/ergodox_dongle.overlay`
  - `zmk-config/boards/shields/ergodox/ergodox_dongle_display.overlay`
  - `tests/test_touchpad_gestures.c`

---

## 1. Problem Statement & Root Cause Analysis

While the initial Synaptics TM-P3125 I2C-HID Precision Touchpad (PTP) driver enables basic wireless pointing and scrolling over ZMK's BLE split architecture, real-world usage exhibits periodic cursor hiccups, scroll stutters, unintended cursor jumps during multi-finger transitions, and incomplete 1-finger and 2-finger gesture coverage.

An audit of the firmware identified four root causes:

1. **Zephyr Workqueue Scheduling Conflict (`k_work_schedule` vs `k_work_reschedule`):**
   - `synaptics_work_handler` schedules `&data->work` at `K_MSEC(4)` whenever any finger is touching, even when the `INT` pin is de-asserted.
   - In Zephyr, calling `k_work_schedule(&data->work, K_NO_WAIT)` from `synaptics_gpio_callback` is a no-op if `data->work` is already scheduled (`K_WORK_DELAYED`). Hardware IRQ edges are ignored while the 4 ms delay timer is pending, and the CPU wastes cycles waking every 4 ms during stationary holds.
2. **BLE GATT Notification Flooding (Primary Bottleneck):**
   - The BLE connection interval between the touchpad peripheral and central dongle is `7.5 ms` (`133 Hz` max, shared across 3 peripherals).
   - `synaptics_work_handler` executes at `250–500 Hz` and emits unthrottled `input_report_rel` calls—and for 2-finger scrolling, emits `INPUT_REL_WHEEL` inside the 16-packet `for` loop without aggregation.
   - Every `input_report_*` call on `zmk,input-split` queues a BLE GATT notification. Bursting reports faster than the 7.5 ms radio interval exhausts the 10 BLE L2CAP TX buffers (`CONFIG_BT_L2CAP_TX_BUF_COUNT=10`) within ~30 ms, dropping packets and stalling the system workqueue.
3. **Oversized I2C Transfers on System Workqueue:**
   - Each loop iteration reads 64 bytes (`uint8_t buf[64]`) up to 16 times per work pass (~25 ms of blocking I2C time at 400 kHz), whereas the Synaptics PTP report (`0x03`) is 32 bytes (`buf[0..31]`).
4. **Incomplete Sensor Filtering, Gesture State Machine & Dongle Mappings:**
   - Hardware PTP `Confidence` bit (`status & 0x01`) is ignored, and `PALM_EDGE_LEFT` / `PALM_EDGE_RIGHT` constants are unused.
   - Transitioning between 1 and 2 fingers (`1F <-> 2F`) causes cursor twitches when the second finger lands slightly after the first or lifts slightly before the first.
   - 1-finger tracking lacks velocity ballistics, sub-pixel remainder accumulation, micro-jitter EMA smoothing, drag-lock continuation, and bottom-right clickpad Right-Click zones.
   - 2-finger gestures lack an anti-jitter tap deadzone, 2-finger physical clickpad Right-Click, and Pinch-to-Zoom (`Ctrl + Plus` / `Ctrl + Minus`).
   - `INPUT_BTN_MIDDLE` (`0x112`) is emitted by the driver for 3-finger taps but is missing from `&zip_button_behaviors` in `ergodox_dongle.overlay` and `ergodox_dongle_display.overlay`.

---

## 2. Architecture: I2C Acquisition & BLE Frame Coalescing Pipeline

### 2.1. Exact-Size I2C Reads & Bounded FIFO Drain
- Set `SYNAPTICS_PTP_REPORT_LEN = 32` (`buf[0..31]`), capturing report ID (`buf[2]`), Slots 0–2 (`buf[3..17]`), contact count (`buf[30]`), and physical clickpad button (`buf[31]`).
- Cap the drain loop in `synaptics_work_handler` to `SYNAPTICS_MAX_PACKETS_PER_PASS = 6` (~4.8 ms worst-case I2C transfer at 400 kHz).
- If `gpio_pin_get_dt(&config->irq_gpio) > 0` after 6 packets, call `k_work_reschedule(&data->work, K_MSEC(1))` to yield the system workqueue to BLE HCI/GATT processing before continuing.

### 2.2. Interrupt-Driven Wakeup & Liftoff Watchdog
- In `synaptics_gpio_callback`, call `k_work_schedule(&data->work, K_NO_WAIT)` (and if `data->work` is delayed on the liftoff watchdog timer, use `k_work_reschedule(&data->work, K_NO_WAIT)` so hardware interrupts always preempt waiting timers immediately).
- When `irq_gpio` is de-asserted (`0`) at the end of `synaptics_work_handler`:
  - If any gesture state is active (`contacts > 0` or `pend_dx != 0` / `pend_dy != 0` / `pend_wheel != 0` / `pend_hwheel != 0`), schedule `&data->work` at:
    - `K_MSEC(8)` if accumulated relative deltas are waiting to be flushed to BLE, or
    - `K_MSEC(35)` liftoff watchdog if no deltas are pending, to detect missed liftoff after `80 ms` of sensor silence.
  - If no fingers are active and no deltas are pending, do not schedule `data->work` (0 Hz idle polling).

### 2.3. 8 ms BLE Report Coalescer (`SYNAPTICS_BLE_FRAME_MS = 8`)
- Instead of calling `input_report_rel` inside the packet loop or on every 2 ms work pass, accumulate:
  - `pend_dx`, `pend_dy` (cursor deltas)
  - `pend_wheel`, `pend_hwheel` (scroll wheel ticks)
- `synaptics_flush_reports(data, now, force)` emits aggregated `INPUT_REL_X`, `INPUT_REL_Y`, `INPUT_REL_WHEEL`, and `INPUT_REL_HWHEEL` only when `(now - data->last_ble_report_time) >= 8` ms or when `force == true` (e.g., immediately prior to button press/release or liftoff).
- In `ergodox_touchpad.conf`, increase BLE buffer counts for extra burst headroom:
  - `CONFIG_BT_L2CAP_TX_BUF_COUNT=16`
  - `CONFIG_BT_BUF_ACL_TX_COUNT=16`
  - `CONFIG_BT_BUF_ACL_RX_COUNT=16`

---

## 3. Sensor Filtering, Palm Rejection & Transition Guards

1. **Hardware Confidence & Palm Edge Rejection:**
   - A finger slot $i \in \{0, 1, 2\}$ is valid only when both `Tip Switch` (`status & 0x02`) and `Confidence` (`status & 0x01`) are asserted: `(status & 0x03) == 0x03`.
   - For a new 1-finger touch-down, if $x_0 < \text{PALM\_EDGE\_LEFT}\ (70)$ or $x_0 > \text{PALM\_EDGE\_RIGHT}\ (1150)$, mark the touch as palm-suppressed until it moves into the active interior ($100 \le x_0 \le 1120$) or lifts.
2. **Packet Gap Re-Anchoring (`SYNAPTICS_GAP_RESET_MS = 45`):**
   - If `(now - data->prev_x0_time) > 45` ms during active 1-finger tracking, re-anchor `prev_x0 = x0`, `prev_y0 = y0` with zero delta for that sample to prevent cursor teleportation after brief hovers or packet drops.
3. **1F -> 2F Landing Grace Window (`SYNAPTICS_2F_GRACE_MS = 20`):**
   - During the first `20 ms` of a new 1-finger touch while cumulative displacement is `< 35` units, buffer cursor movement in `grace_dx` / `grace_dy`.
   - If a second finger lands within `20 ms`, discard `grace_dx` / `grace_dy` and enter 2-finger mode cleanly with zero cursor twitch.
   - Once `20 ms` elapses or displacement reaches `>= 35` units with 1 finger, flush `grace_dx` / `grace_dy` into `pend_dx` / `pend_dy` and track normally.
4. **2F/3F -> 1F Liftoff Guard:**
   - Once `gesture_max_fingers >= 2`, lifting down to 1 finger (`contacts == 1`) does **not** resume 1-finger cursor movement until all fingers lift (`contacts == 0`), preventing end-of-scroll or end-of-right-click cursor jumps.

---

## 4. Gesture Engine & Velocity Ballistics Specification

### 4.1. 1-Finger Engine
- **2-Tap EMA Micro-Jitter Smoothing & Q4 Sub-Pixel Accumulator:**
  - For raw delta $(\Delta x, \Delta y)$, when $|\Delta x| + |\Delta y| \le 6$:
    $$\Delta x_{\text{smooth}} = \frac{3 \cdot \Delta x + \Delta x_{\text{prev}}}{4}, \quad \Delta y_{\text{smooth}} = \frac{3 \cdot \Delta y + \Delta y_{\text{prev}}}{4}$$
  - At higher speeds ($|\Delta x| + |\Delta y| > 6$), use unsmoothed $\Delta x, \Delta y$ for zero lag.
- **Piecewise Velocity Ballistics (Q4 Fixed-Point Gain `/ 16`):**
  - Speed metric $v = |\Delta x| + |\Delta y|$:
    - $v \le 6$: gain $= 12$ ($0.75\times$, precision targeting)
    - $6 < v \le 22$: gain $= 16$ ($1.00\times$, 1:1 linear)
    - $22 < v \le 50$: gain $= 24$ ($1.50\times$, fast travel)
    - $v > 50$: gain $= 32$ ($2.00\times$, full-screen swipe)
  - Scaled values add to `subpixel_rem_x` and `subpixel_rem_y`; integer quotient (`/ 16`) is added to `pend_dx` / `pend_dy`, and remainder (`% 16`) is retained across packets.
- **1-Finger Tap (Left Click):**
  - Recognizes on liftoff when `gesture_max_fingers == 1`, duration $< 280$ ms, and `touch_max_disp < 90` units.
  - Emits `INPUT_BTN_LEFT = 1` and schedules release after `40 ms`.
- **1-Finger Double-Tap & Drag + 180 ms Drag-Lock Continuation:**
  - If a new 1F touch-down occurs within `400 ms` (`TAP_DRAG_TIMEOUT_MS`) and within `140` units (`TAP_DRAG_MAX_DISTANCE`) of the previous 1F tap, enter `is_tap_dragging = true` and hold `INPUT_BTN_LEFT = 1`.
  - On finger liftoff while `is_tap_dragging == true`, do not release `INPUT_BTN_LEFT` immediately; schedule `tap_release_work` for `180 ms` (`DRAG_LOCK_CONT_MS = 180`).
  - If 1 finger touches down again within `180 ms`, cancel `tap_release_work` and continue dragging seamlessly (`INPUT_BTN_LEFT` stays held). If `180 ms` expires without re-touch, `tap_release_work` releases `INPUT_BTN_LEFT = 0` and clears `is_tap_dragging`.

### 4.2. Physical Clickpad Engine (1F Zones, 2F, 3F)
- When hardware clickpad switch (`buf[31] & 0x01`) transitions from `0 -> 1`:
  - Flush any pending relative motion first.
  - Determine target button code `active_phys_btn`:
    - `contacts >= 3` or `gesture_max_fingers >= 3` $\to$ `INPUT_BTN_MIDDLE`
    - `contacts == 2` or `gesture_max_fingers == 2` $\to$ `INPUT_BTN_RIGHT`
    - `contacts == 1` and $x_0 > 650$ and $y_0 > 650$ (Bottom-Right Clickpad Zone) $\to$ `INPUT_BTN_RIGHT`
    - Otherwise $\to$ `INPUT_BTN_LEFT`
  - Emit `input_report_key(dev, active_phys_btn, 1, true, K_NO_WAIT)` and suppress tap-to-click for the current touch cycle.
- When hardware clickpad switch transitions from `1 -> 0`:
  - Emit `input_report_key(dev, active_phys_btn, 0, true, K_NO_WAIT)` and reset `active_phys_btn = 0`.

### 4.3. 2-Finger Engine (Tap, Natural V/H Scroll, Pinch-to-Zoom)
- **2-Finger Tap (Right Click with Anti-Jitter Deadzone):**
  - Track cumulative centroid travel `two_finger_travel` and span change `two_finger_span_delta` from initial 2F anchor.
  - While `two_finger_travel < 45` and `|two_finger_span_delta| < 55`, remain in `TWO_FINGER_IDLE` (anti-jitter deadzone).
  - If all fingers lift while in `TWO_FINGER_IDLE` and duration $< 280$ ms (and physical click was not used), emit `INPUT_BTN_RIGHT = 1` + `40 ms` delayed release.
- **2-Finger Mode Lock (Scroll vs. Pinch-to-Zoom):**
  - Once movement exceeds the deadzone:
    - **Pinch-to-Zoom Lock (`TWO_FINGER_PINCH`):** Entered if initial finger span $S_0 = |x_0 - x_1| + |y_0 - y_1| \ge 160$ and $|\Delta S_{\text{total}}| > 55$ with $|\Delta S_{\text{total}}| > (|\Delta c_x| + |\Delta c_y|) \times 3 / 2$.
      - Accumulate frame-by-frame $\Delta S$ in `pinch_accum`.
      - For every `+85` units (`PINCH_STEP = 85`), emit a tap on `INPUT_BTN_5` (mapped to `LC(EQUAL)` Zoom In on dongle) with `35 ms` release.
      - For every `-85` units, emit a tap on `INPUT_BTN_6` (mapped to `LC(MINUS)` Zoom Out on dongle) with `35 ms` release.
    - **2-Finger Natural Scroll Lock (`TWO_FINGER_SCROLL`):** Entered otherwise.
      - **Natural Vertical Scroll:** When fingers move up ($\Delta c_y < 0$), page scrolls down; when fingers move down ($\Delta c_y > 0$), page scrolls up. Accumulate `scroll_accum_y += dy` with `SCROLL_STEP = 26` and add resulting ticks to `pend_wheel`.
      - **Horizontal Scroll:** When $|\Delta c_x| > |\Delta c_y| \times 2$, accumulate `scroll_accum_x += dx` with `SCROLL_STEP = 26` and add resulting ticks to `pend_hwheel`.

### 4.4. 3-Finger Engine & Dongle Input Processor Mappings
- **3-Finger Tap:** Duration $< 280$ ms without swipe $\to$ `INPUT_BTN_MIDDLE = 1` + `40 ms` release.
- **3-Finger Swipe Up / Down:** Centroid vertical displacement beyond $\pm 120$ units $\to$ `INPUT_BTN_3` (`LG(TAB)` Task View) / `INPUT_BTN_4` (`LG(D)` Show Desktop) + `40 ms` release.
- **Dongle Overlay Mapping (`ergodox_dongle.overlay` & `ergodox_dongle_display.overlay`):**
  ```dts
  &zip_button_behaviors {
      codes = <INPUT_BTN_0 INPUT_BTN_1 INPUT_BTN_2 INPUT_BTN_3 INPUT_BTN_4 INPUT_BTN_5 INPUT_BTN_6 INPUT_BTN_LEFT INPUT_BTN_RIGHT INPUT_BTN_MIDDLE>;
      bindings = <&mkp LCLK &mkp RCLK &mkp MCLK &kp LG(TAB) &kp LG(D) &kp LC(EQUAL) &kp LC(MINUS) &mkp LCLK &mkp RCLK &mkp MCLK>;
  };
  ```

---

## 5. Verification & Testing Plan

1. **Deterministic Host Unit Tests (`tests/test_touchpad_gestures.c`):**
   - Compile and run a standalone C test suite mocking Zephyr's I2C/GPIO/Input APIs to verify:
     - 32-byte I2C reads and max 6 packets per work pass.
     - 8 ms BLE report coalescing (multiple packets within 8 ms produce a single `INPUT_REL_X` / `INPUT_REL_Y` / `INPUT_REL_WHEEL` emission).
     - 1F velocity ballistics (0.75x slow, 1.0x normal, 1.5x fast, 2.0x swipe) and Q4 sub-pixel remainder preservation.
     - 1F Tap, Double-Tap & Drag, and 180 ms Drag-Lock continuation across lift/re-touch.
     - Palm rejection on left/right edges and hardware `Confidence == 0` rejection.
     - Physical Clickpad zones (Bottom-Left = `INPUT_BTN_LEFT`, Bottom-Right = `INPUT_BTN_RIGHT`, 2F = `INPUT_BTN_RIGHT`, 3F = `INPUT_BTN_MIDDLE`).
     - 2F Tap with anti-jitter deadzone, 2F Natural V/H Scroll, 2F Pinch-to-Zoom (`INPUT_BTN_5` / `INPUT_BTN_6`), 1F->2F grace window, and 2F->1F liftoff guard.
     - 3F Tap (`INPUT_BTN_MIDDLE`) and 3F Swipe Up/Down (`INPUT_BTN_3` / `INPUT_BTN_4`).
2. **Firmware Build & Critic Verification:**
   - Run `node .agents/skills/firmware-critic/scripts/audit.js`.
   - Build all firmware targets via `node .agents/skills/firmware-critic/scripts/build.js` and verify fresh `.uf2` binaries in `firmware/`.
