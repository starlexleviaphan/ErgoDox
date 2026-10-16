# Touchpad Firmware Optimization & Multi-Finger Gesture Engine Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Eliminate wireless touchpad hiccups/stutters via 32-byte bounded I2C reads and 8 ms BLE report coalescing while implementing the full 1-finger, 2-finger (Tap, Natural V/H Scroll, Pinch-to-Zoom, Physical Click), and 3-finger gesture engine.

**Architecture:** Refactor `zmk-config/drivers/input/input_synaptics_tm_p3125.c` into an interrupt-driven, 32-byte bounded FIFO reader (max 6 packets/pass) paired with an 8 ms (125 Hz) BLE frame coalescer that aggregates all relative axes (`INPUT_REL_X`, `INPUT_REL_Y`, `INPUT_REL_WHEEL`, `INPUT_REL_HWHEEL`) so L2CAP TX buffers never saturate. Layer a deterministic multi-finger finite state machine on top with hardware `Confidence` + palm edge filtering, 1F->2F landing grace window, 2F->1F liftoff guard, Q4 sub-pixel velocity ballistics, 180 ms drag-lock continuation, physical clickpad zones, and 2F Scroll vs. Pinch-to-Zoom classification mapped through `&zip_button_behaviors` on the central dongle.

**Tech Stack:** C11 (Zephyr RTOS / ZMK Firmware input subsystem, Devicetree overlays, Kconfig), GCC + Pytest for host-side unit testing (`-DZMK_TOUCHPAD_UNIT_TEST`), GitHub Actions ZMK build pipeline.

**Spec:** `docs/superpowers/specs/2026-10-16-touchpad-optimization-design.md`

## Global Constraints

- `SYNAPTICS_PTP_REPORT_LEN = 32` bytes per `i2c_read` call (`buf[0..31]`), never 64 bytes.
- `SYNAPTICS_MAX_PACKETS_PER_PASS = 6` maximum I2C packets drained per workqueue invocation.
- `SYNAPTICS_BLE_FRAME_MS = 8` ms minimum interval between non-forced relative report bursts (`INPUT_REL_X`, `INPUT_REL_Y`, `INPUT_REL_WHEEL`, `INPUT_REL_HWHEEL`).
- Hardware slot validity requires both `Tip Switch` (`0x02`) and `Confidence` (`0x01`): `(status & 0x03) == 0x03`.
- Palm edge rejection bounds for new 1-finger touches: `PALM_EDGE_LEFT = 70`, `PALM_EDGE_RIGHT = 1150` (unlock interior `100 <= x0 <= 1120`).
- Dongle `&zip_button_behaviors` in both `ergodox_dongle.overlay` and `ergodox_dongle_display.overlay` must map `INPUT_BTN_5` -> `&kp LC(EQUAL)`, `INPUT_BTN_6` -> `&kp LC(MINUS)`, and `INPUT_BTN_MIDDLE` -> `&mkp MCLK`.
- All source files in `zmk-config/` must remain pure ASCII (`tests/test_no_non_ascii.py`).

## Review Focus

- **Rapid Pinch-to-Zoom bursts faster than 35 ms release work:** If a second pinch step triggers in the same direction while `pinch_in_release_work` or `pinch_out_release_work` is still pending, the driver must force-release (`0`) before re-asserting (`1`) so the host OS sees every distinct zoom step instead of coalescing them into a single held key.
- **Physical clickpad release after finger count changes:** If the user presses the physical clickpad with 2 fingers (`INPUT_BTN_RIGHT` asserted) and lifts the fingers *before* releasing the physical clickpad switch, releasing the switch (`buf[31] & 0x01 == 0`) must release `active_phys_btn` (`INPUT_BTN_RIGHT`), not `INPUT_BTN_LEFT`.
- **Pending relative motion stranded when `irq_gpio` de-asserts mid-frame:** If packets are read at $t = 3\text{ ms}$ (after a previous BLE flush at $t = 0\text{ ms}$) and `irq_gpio` immediately goes idle (`0`) with no further interrupts, the work handler must schedule a follow-up flush at `K_MSEC(8)` so `pend_dx`/`pend_dy`/`pend_wheel` are never stranded in memory.
- **1F->2F grace window expiration when finger lifts early:** If a 1-finger touch moves `< 35` units (buffered in `grace_dx`/`grace_dy`) and lifts before `20 ms`, it must be evaluated as a 1-finger tap (Left Click) and clear `grace_dx`/`grace_dy` without emitting a stale cursor jump on the next touch.
- **Corrupted or out-of-range I2C frame (`dx` or `dy` > 250 units):** When a single frame has an abnormal coordinate spike (`|dx| >= 250` or `|dy| >= 250`), `prev_x0`/`prev_y0` must still update to the new anchor while ignoring the spike delta so tracking does not get permanently stuck rejecting subsequent frames.

---

### Task 1: BLE Buffer Headroom & Dongle Input Processor Mappings

**Files:**
- Modify: `zmk-config/boards/shields/ergodox/ergodox_touchpad.conf:49-53`
- Modify: `zmk-config/boards/shields/ergodox/ergodox_dongle.overlay:37-41`
- Modify: `zmk-config/boards/shields/ergodox/ergodox_dongle_display.overlay:49-53`
- Modify: `zmk-config/boards/shields/ergodox/ergodox_dongle.conf:48-52`
- Modify: `zmk-config/boards/shields/ergodox/ergodox_dongle_display.conf:48-52`
- Create: `tests/test_touchpad_config.py`

**Interfaces:**
- Consumes: ZMK `&zip_button_behaviors` Devicetree node and `<zephyr/dt-bindings/input/input-event-codes.h>` (`INPUT_BTN_0..6`, `INPUT_BTN_LEFT`, `INPUT_BTN_RIGHT`, `INPUT_BTN_MIDDLE`).
- Produces: Dongle button mappings for `INPUT_BTN_5` (`&kp LC(EQUAL)`), `INPUT_BTN_6` (`&kp LC(MINUS)`), `INPUT_BTN_MIDDLE` (`&mkp MCLK`), and 16-buffer BLE L2CAP/ACL configuration on touchpad and dongles.

- [ ] **Step 1: Write the failing configuration test in `tests/test_touchpad_config.py`**

```python
def test_touchpad_ble_buffers():
    with open("zmk-config/boards/shields/ergodox/ergodox_touchpad.conf", "r", encoding="utf-8") as f:
        conf = f.read()
    assert "CONFIG_BT_L2CAP_TX_BUF_COUNT=16" in conf
    assert "CONFIG_BT_BUF_ACL_TX_COUNT=16" in conf
    assert "CONFIG_BT_BUF_ACL_RX_COUNT=16" in conf


def test_dongle_zip_button_behaviors():
    for path in [
        "zmk-config/boards/shields/ergodox/ergodox_dongle.overlay",
        "zmk-config/boards/shields/ergodox/ergodox_dongle_display.overlay",
    ]:
        with open(path, "r", encoding="utf-8") as f:
            overlay = f.read()
        assert "INPUT_BTN_5" in overlay, f"Missing INPUT_BTN_5 in {path}"
        assert "INPUT_BTN_6" in overlay, f"Missing INPUT_BTN_6 in {path}"
        assert "INPUT_BTN_MIDDLE" in overlay, f"Missing INPUT_BTN_MIDDLE in {path}"
        assert "&kp LC(EQUAL)" in overlay, f"Missing Zoom In binding in {path}"
        assert "&kp LC(MINUS)" in overlay, f"Missing Zoom Out binding in {path}"
```

- [ ] **Step 2: Run `pytest` to verify the test fails**

Run: `pytest tests/test_touchpad_config.py -v`
Expected: FAIL (`CONFIG_BT_L2CAP_TX_BUF_COUNT=16` and `INPUT_BTN_5` not found)

- [ ] **Step 3: Update `ergodox_touchpad.conf`, `ergodox_dongle.conf`, `ergodox_dongle_display.conf`, and both dongle overlays**

In `ergodox_touchpad.conf`, `ergodox_dongle.conf`, and `ergodox_dongle_display.conf`, set:
```ini
CONFIG_BT_L2CAP_TX_BUF_COUNT=16
CONFIG_BT_BUF_ACL_TX_COUNT=16
CONFIG_BT_BUF_ACL_RX_COUNT=16
```
In both `ergodox_dongle.overlay` and `ergodox_dongle_display.overlay`, update `&zip_button_behaviors` to:
```dts
&zip_button_behaviors {
    codes = <INPUT_BTN_0 INPUT_BTN_1 INPUT_BTN_2 INPUT_BTN_3 INPUT_BTN_4 INPUT_BTN_5 INPUT_BTN_6 INPUT_BTN_LEFT INPUT_BTN_RIGHT INPUT_BTN_MIDDLE>;
    bindings = <&mkp LCLK &mkp RCLK &mkp MCLK &kp LG(TAB) &kp LG(D) &kp LC(EQUAL) &kp LC(MINUS) &mkp LCLK &mkp RCLK &mkp MCLK>;
};
```

- [ ] **Step 4: Run `pytest` to verify configuration and ASCII tests pass**

Run: `pytest tests/test_touchpad_config.py tests/test_no_non_ascii.py -v`
Expected: PASS

- [ ] **Step 5: Commit Task 1**

```bash
git add tests/test_touchpad_config.py
git commit -m "test(touchpad): add config and dongle button mapping tests"
git -C zmk-config add boards/shields/ergodox/ergodox_touchpad.conf boards/shields/ergodox/ergodox_dongle.conf boards/shields/ergodox/ergodox_dongle_display.conf boards/shields/ergodox/ergodox_dongle.overlay boards/shields/ergodox/ergodox_dongle_display.overlay
git -C zmk-config commit -m "feat(touchpad): add pinch-to-zoom and middle-click dongle bindings and expand BLE buffers"
```

---

### Task 2: Event-Driven 32-Byte I2C Pipeline, 8 ms BLE Report Coalescer & Sensor Guards

**Files:**
- Modify: `zmk-config/drivers/input/input_synaptics_tm_p3125.c`
- Create: `tests/test_touchpad_gestures.c`
- Create: `tests/test_touchpad_gestures.py`

**Interfaces:**
- Consumes: Zephyr I2C/GPIO/Workqueue/Input APIs (or host test mocks when `ZMK_TOUCHPAD_UNIT_TEST` is defined).
- Produces:
  - Constants: `SYNAPTICS_PTP_REPORT_LEN (32)`, `SYNAPTICS_MAX_PACKETS_PER_PASS (6)`, `SYNAPTICS_BLE_FRAME_MS (8)`, `SYNAPTICS_GAP_RESET_MS (45)`, `SYNAPTICS_LIFTOFF_TIMEOUT_MS (85)`, `PALM_EDGE_LEFT (70)`, `PALM_EDGE_RIGHT (1150)`.
  - Functions: `synaptics_flush_reports(struct synaptics_data *data, int64_t now, bool force)`, `synaptics_slot_valid(uint8_t status)`.
  - Unit test harness in `tests/test_touchpad_gestures.c` that `#include`s `../zmk-config/drivers/input/input_synaptics_tm_p3125.c` under `-DZMK_TOUCHPAD_UNIT_TEST`.

- [ ] **Step 1: Write the failing host C unit test for I2C 32-byte drain, 8 ms BLE coalescing, confidence bit, palm edge rejection, and gap re-anchoring in `tests/test_touchpad_gestures.c` and runner `tests/test_touchpad_gestures.py`**

Create `tests/test_touchpad_gestures.c` with Zephyr API mocks (`mock_enqueue_ptp_packet`, `mock_step_work`, `mock_rel_events_count`, `mock_last_i2c_read_len`) and test functions:
- `test_i2c_32_byte_read_and_6_packet_cap()`: Enqueue 10 PTP packets while `irq_gpio == 1`; run `synaptics_work_handler` once and assert `mock_i2c_read_calls == 6`, `mock_last_i2c_read_len == 32`, and `data.work` is rescheduled for `1 ms`.
- `test_ble_8ms_coalescing_and_stranded_flush()`: Feed 4 packets at $t = 1, 2, 3, 4\text{ ms}$ (after a flush at $t = 0\text{ ms}$) with `irq_gpio` dropping to `0` after packet 4; assert zero `INPUT_REL_X`/`Y` emitted during $t = 1..4\text{ ms}$, assert `data.work` is scheduled for `<= 8 ms`, and when executed at $t = 8\text{ ms}$ emits exactly 1 aggregated `INPUT_REL_X` and 1 `INPUT_REL_Y`.
- `test_confidence_and_palm_edge_rejection()`: Feed 1F touch with `status0 = 0x02` (`Confidence == 0`) -> assert ignored; feed 1F touch at `x0 = 40` (`< PALM_EDGE_LEFT`) with `status0 = 0x03` -> assert palm-suppressed until `x0` enters `[100, 1120]`.
- `test_gap_reanchoring_and_spike_recovery()`: Feed 1F at `(300, 300)` at $t = 0$, then `(600, 600)` at $t = 60\text{ ms}$ (`> 45 ms` gap) -> assert re-anchored with 0 delta; then feed `(900, 300)` at $t = 64\text{ ms}$ (`dx = 300 >= 250` spike) -> assert spike rejected AND `prev_x0` updated to `900` so next frame at `(910, 300)` produces normal movement.

- [ ] **Step 2: Run `pytest tests/test_touchpad_gestures.py` to verify it fails**

Run: `pytest tests/test_touchpad_gestures.py -v`
Expected: FAIL (64-byte reads, 16 packet loop, missing `ZMK_TOUCHPAD_UNIT_TEST` guard and coalescer)

- [ ] **Step 3: Implement `#ifndef ZMK_TOUCHPAD_UNIT_TEST` header guards, 32-byte bounded I2C drain, `k_work_reschedule` IRQ wakeup, 8 ms BLE report coalescer, and confidence/palm/gap guards in `zmk-config/drivers/input/input_synaptics_tm_p3125.c`**

Key implementation details in `input_synaptics_tm_p3125.c`:
- Wrap `<zephyr/...>` includes in `#ifndef ZMK_TOUCHPAD_UNIT_TEST` so host GCC can compile the file directly.
- Replace `uint8_t buf[64]` and `for (int iter = 0; iter < 16; iter++)` with `uint8_t buf[SYNAPTICS_PTP_REPORT_LEN]` (`32`) and `for (int iter = 0; iter < SYNAPTICS_MAX_PACKETS_PER_PASS; iter++)` (`6`).
- Implement `synaptics_flush_reports(struct synaptics_data *data, int64_t now, bool force)` that flushes `pend_dx`, `pend_dy`, `pend_wheel`, `pend_hwheel` when `force || (now - data->last_ble_report_time) >= SYNAPTICS_BLE_FRAME_MS`.
- In `synaptics_gpio_callback` and `synaptics_heartbeat_handler`, call `k_work_reschedule(&data->work, K_NO_WAIT)` so hardware interrupts immediately preempt any pending watchdog delay.
- At the end of `synaptics_work_handler`:
  - If `gpio_pin_get_dt(&config->irq_gpio) > 0`: `k_work_reschedule(&data->work, K_MSEC(1))`.
  - Else if `data->pend_dx || data->pend_dy || data->pend_wheel || data->pend_hwheel`: `k_work_reschedule(&data->work, K_MSEC(SYNAPTICS_BLE_FRAME_MS))`.
  - Else if active touch state exists: `k_work_reschedule(&data->work, K_MSEC(35))`.

- [ ] **Step 4: Run `pytest tests/test_touchpad_gestures.py` to verify Task 2 tests pass**

Run: `pytest tests/test_touchpad_gestures.py -v`
Expected: PASS

- [ ] **Step 5: Commit Task 2**

```bash
git add tests/test_touchpad_gestures.c tests/test_touchpad_gestures.py
git commit -m "test(touchpad): add host C unit test harness for I2C drain and BLE coalescer"
git -C zmk-config add drivers/input/input_synaptics_tm_p3125.c
git -C zmk-config commit -m "perf(touchpad): implement 32-byte bounded I2C drain, 8ms BLE coalescer, and palm/confidence guards"
```

---

### Task 3: 1-Finger Engine — Velocity Ballistics, Sub-Pixel EMA, 1F->2F Grace Window, Drag-Lock & Clickpad Zones

**Files:**
- Modify: `zmk-config/drivers/input/input_synaptics_tm_p3125.c`
- Modify: `tests/test_touchpad_gestures.c`

**Interfaces:**
- Consumes: `synaptics_flush_reports`, `struct synaptics_data` state fields.
- Produces:
  - `synaptics_apply_ballistics(struct synaptics_data *data, int16_t raw_dx, int16_t raw_dy, int16_t *out_dx, int16_t *out_dy)` using 2-tap EMA when `|dx|+|dy| <= 6`, Q4 fixed-point gains (`12`, `16`, `24`, `32` `/ 16`), and `subpixel_rem_x` / `subpixel_rem_y`.
  - 1F->2F landing grace window (`SYNAPTICS_2F_GRACE_MS = 20`, `< 35` units).
  - 1F Single Tap (`< 280 ms`, `< 90` units), Double-Tap & Drag (`< 400 ms`, `< 140` units), and `DRAG_LOCK_CONT_MS = 180` continuation on liftoff.
  - Physical clickpad handler (`synaptics_process_clickpad`) tracking `active_phys_btn` across press and release (`INPUT_BTN_LEFT`, `INPUT_BTN_RIGHT` for 1F bottom-right `x0 > 650 && y0 > 650` or 2F, `INPUT_BTN_MIDDLE` for 3F).

- [ ] **Step 1: Add failing C unit tests for 1F ballistics, sub-pixel accumulation, 1F->2F grace window, 180 ms drag-lock, and clickpad zones in `tests/test_touchpad_gestures.c`**

Add tests:
- `test_1f_ballistics_and_subpixel()`: Verify slow step `(1, 0)` with Q4 gain `12/16` accumulates across 2 packets to emit `+1` without losing fractional remainder; verify fast step `(30, 0)` (`v=30`, gain `24/16`) emits `45`; verify swipe step `(60, 0)` (`v=60`, gain `32/16`) emits `120`.
- `test_1f_tap_and_grace_window_early_lift()`: Touch 1F at `(400, 400)` at $t=0$, move to `(410, 405)` at $t=8\text{ ms}$ (inside `20 ms` / `< 35` unit grace window), lift at $t=15\text{ ms}$ -> assert 0 cursor movement emitted and `INPUT_BTN_LEFT` tap click emitted cleanly.
- `test_1f_double_tap_drag_and_180ms_drag_lock()`: Perform 1F tap at $t=0..50\text{ ms}$, re-touch at $t=150\text{ ms}$ and drag -> assert `INPUT_BTN_LEFT` stays `1`; lift finger at $t=300\text{ ms}$ -> assert `INPUT_BTN_LEFT` is still held at $t=400\text{ ms}$ (`< 180 ms`), re-touch at $t=420\text{ ms}$ -> assert drag continues without releasing `INPUT_BTN_LEFT`; lift at $t=500\text{ ms}$ and wait until $t=700\text{ ms}$ (`> 180 ms`) -> assert `INPUT_BTN_LEFT` released (`0`).
- `test_physical_clickpad_zones_and_liftoff_before_release()`: Press physical click (`buf[31]=1`) with 1F at `(800, 750)` (bottom-right) -> assert `INPUT_BTN_RIGHT=1`; lift finger (`contacts=0`, `buf[31]=1`), then release physical click (`buf[31]=0`) -> assert `INPUT_BTN_RIGHT=0` (not `INPUT_BTN_LEFT`).

- [ ] **Step 2: Run `pytest tests/test_touchpad_gestures.py` to verify new tests fail**

Run: `pytest tests/test_touchpad_gestures.py -v`
Expected: FAIL

- [ ] **Step 3: Implement `synaptics_apply_ballistics`, 1F->2F grace window, 180 ms drag-lock continuation, and `synaptics_process_clickpad` in `zmk-config/drivers/input/input_synaptics_tm_p3125.c`**

Implement the exact thresholds from Section 4.1 & 4.2 of the spec in `input_synaptics_tm_p3125.c`.

- [ ] **Step 4: Run `pytest tests/test_touchpad_gestures.py` to verify all Task 2 & Task 3 tests pass**

Run: `pytest tests/test_touchpad_gestures.py -v`
Expected: PASS

- [ ] **Step 5: Commit Task 3**

```bash
git add tests/test_touchpad_gestures.c
git commit -m "test(touchpad): add unit tests for 1F ballistics, drag-lock, grace window, and clickpad zones"
git -C zmk-config add drivers/input/input_synaptics_tm_p3125.c
git -C zmk-config commit -m "feat(touchpad): implement 1F velocity ballistics, subpixel EMA, 180ms drag-lock, and clickpad zones"
```

---

### Task 4: 2-Finger & 3-Finger Engine — Anti-Jitter 2F Tap, Natural V/H Scroll, Pinch-to-Zoom & 3F Gestures

**Files:**
- Modify: `zmk-config/drivers/input/input_synaptics_tm_p3125.c`
- Modify: `tests/test_touchpad_gestures.c`

**Interfaces:**
- Consumes: `synaptics_flush_reports`, `struct synaptics_data`.
- Produces:
  - `enum synaptics_2f_mode { TWO_FINGER_IDLE = 0, TWO_FINGER_SCROLL, TWO_FINGER_PINCH }`.
  - Delayed release works `pinch_in_release_work` (`INPUT_BTN_5`) and `pinch_out_release_work` (`INPUT_BTN_6`).
  - 2F Tap (`< 280 ms` in `TWO_FINGER_IDLE` with `45`-unit centroid / `55`-unit span anti-jitter deadzone).
  - 2F Natural Scroll (`SCROLL_STEP = 26`, aggregated into `pend_wheel` and `pend_hwheel`, flushed via 8 ms BLE coalescer).
  - 2F Pinch-to-Zoom (`PINCH_STEP = 85`, emitting `INPUT_BTN_5` for spread > 0 and `INPUT_BTN_6` for spread < 0, with forced `0` release before re-triggering if already pending).
  - 2F/3F -> 1F liftoff guard (suppressing 1F cursor jumps when lifting one finger before the other).
  - 3F Tap (`INPUT_BTN_MIDDLE`) and 3F Swipe Up/Down (`INPUT_BTN_3` / `INPUT_BTN_4`).

- [ ] **Step 1: Add failing C unit tests for 2F anti-jitter tap, Natural V/H scroll, Pinch-to-Zoom (including rapid consecutive zoom steps), 1F->2F transition, 2F->1F liftoff guard, and 3F gestures in `tests/test_touchpad_gestures.c`**

Add tests:
- `test_2f_tap_with_settling_jitter()`: Touch 2 fingers, jitter centroid by 30 units (`< 45` deadzone), lift after `120 ms` -> assert `INPUT_BTN_RIGHT` click emitted and zero scroll ticks.
- `test_2f_natural_scroll_aggregated_in_8ms_frame()`: Touch 2 fingers and move parallel up (`dy = -30` per packet for 3 packets within 6 ms) -> assert `INPUT_REL_WHEEL` is not emitted inside the packet loop, and is emitted once as a single aggregated tick count on the 8 ms BLE flush.
- `test_2f_pinch_to_zoom_in_and_out_rapid()`: Touch 2 fingers at `(300, 400)` and `(600, 400)` (span `300 >= 160`); expand span by `+180` units (`2 * PINCH_STEP`) within 10 ms -> assert `INPUT_BTN_5` is pulsed twice with an explicit `0` release between pulses so neither pulse is lost; then contract span by `-90` units -> assert `INPUT_BTN_6` pulsed.
- `test_1f_to_2f_grace_and_2f_to_1f_liftoff_guard()`: Touch 1F at $t=0$, move `(15, 15)` at $t=8\text{ ms}$, land 2nd finger at $t=12\text{ ms}$ -> assert zero `INPUT_REL_X`/`Y` emitted; scroll with 2F, then lift 2nd finger leaving 1F moving `(50, 50)` -> assert zero `INPUT_REL_X`/`Y` emitted until all fingers lift.
- `test_3f_tap_and_swipes()`: Verify 3F tap emits `INPUT_BTN_MIDDLE`; 3F swipe up emits `INPUT_BTN_3`; 3F swipe down emits `INPUT_BTN_4`.

- [ ] **Step 2: Run `pytest tests/test_touchpad_gestures.py` to verify new tests fail**

Run: `pytest tests/test_touchpad_gestures.py -v`
Expected: FAIL

- [ ] **Step 3: Implement 2F mode classifier (Tap deadzone, Natural V/H Scroll into `pend_wheel`/`pend_hwheel`, Pinch-to-Zoom `INPUT_BTN_5`/`INPUT_BTN_6`), 2F->1F guard, and 3F engine in `zmk-config/drivers/input/input_synaptics_tm_p3125.c`**

Ensure:
- In `TWO_FINGER_SCROLL`, scroll ticks accumulate into `data->pend_wheel` and `data->pend_hwheel` (never calling `input_report_rel` directly inside the I2C loop).
- In `TWO_FINGER_PINCH`, each `PINCH_STEP` (`85`) cancels `pinch_in_release_work` / `pinch_out_release_work`, emits `input_report_key(dev, btn, 0, true, K_NO_WAIT)` followed by `input_report_key(dev, btn, 1, true, K_NO_WAIT)`, and schedules release after `35 ms`.
- On liftoff (`synaptics_process_liftoff`), cancel and release `INPUT_BTN_5` and `INPUT_BTN_6` cleanly alongside `INPUT_BTN_3` and `INPUT_BTN_4`.

- [ ] **Step 4: Run full local test suite (`pytest`)**

Run: `pytest -v`
Expected: PASS (all tests in `tests/` green)

- [ ] **Step 5: Commit Task 4**

```bash
git add tests/test_touchpad_gestures.c
git commit -m "test(touchpad): add unit tests for 2F scroll, pinch-to-zoom, anti-jitter tap, and 3F gestures"
git -C zmk-config add drivers/input/input_synaptics_tm_p3125.c
git -C zmk-config commit -m "feat(touchpad): implement 2F pinch-to-zoom, natural scroll coalescing, anti-jitter tap, and transition guards"
```

---

### Task 5: Documentation Sync, Firmware Critic Audit & Cloud CI Firmware Build

**Files:**
- Modify: `docs/superpowers/specs/2026-09-20-touchpad-integration-design.md`
- Modify: `firmware/ergodox_touchpad-nice_nano_v2-zmk.uf2` (via `build.js`)
- Modify: `firmware/ergodox_dongle-nice_nano_v2-zmk.uf2` (via `build.js`)
- Modify: `firmware/ergodox_dongle_display-nice_nano_v2-zmk.uf2` (via `build.js`)

**Interfaces:**
- Consumes: `.agents/skills/firmware-critic/scripts/audit.js` and `.agents/skills/firmware-critic/scripts/build.js`.
- Produces: Verified ZMK `.uf2` binaries in `firmware/` and updated architecture documentation.

- [ ] **Step 1: Update `docs/superpowers/specs/2026-09-20-touchpad-integration-design.md` to reference the new 32-byte I2C + 8 ms BLE coalesced pipeline and complete gesture table**

Update Section 3.1 of `docs/superpowers/specs/2026-09-20-touchpad-integration-design.md` so no outdated 60-byte / 1:1 linear notes contradict `2026-10-16-touchpad-optimization-design.md`.

- [ ] **Step 2: Run Firmware Critic Audit & Full `pytest` Suite**

Run: `node .agents/skills/firmware-critic/scripts/audit.js && pytest -v`
Expected: 0 errors, all tests PASS.

- [ ] **Step 3: Trigger Cloud ZMK Build and Download `.uf2` Binaries to `firmware/`**

Run: `node .agents/skills/firmware-critic/scripts/build.js`
Expected: GitHub Actions build succeeds for all targets (`ergodox_left`, `ergodox_right`, `ergodox_dongle`, `ergodox_dongle_display`, `ergodox_touchpad`, `settings_reset`) and downloads fresh `.uf2` files to `firmware/`.

- [ ] **Step 4: Commit updated documentation and firmware binaries in root repository**

```bash
git add docs/superpowers/specs/2026-09-20-touchpad-integration-design.md firmware/*.uf2
git commit -m "build(firmware): compile optimized touchpad and dongle firmware with full gesture engine"
```
