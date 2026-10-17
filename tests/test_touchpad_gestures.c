/*
 * Host C unit tests for Synaptics TM-P3125 driver:
 * Task 2: 32-Byte I2C Pipeline, 8 ms BLE Report Coalescer & Sensor Guards
 */

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* ========================================================================= */
/* Zephyr API Mocks for Host Unit Testing                                    */
/* ========================================================================= */

struct device {
  const void *config;
  void *data;
};

struct i2c_dt_spec {
  const struct device *bus;
  uint16_t addr;
};

typedef uint32_t gpio_port_pins_t;
typedef uint32_t gpio_flags_t;

struct gpio_dt_spec {
  const struct device *port;
  uint8_t pin;
  gpio_flags_t dt_flags;
};

struct gpio_callback {
  void *handler;
  gpio_port_pins_t pin_mask;
};

struct k_work;
typedef void (*k_work_handler_t)(struct k_work *work);

struct k_work {
  k_work_handler_t handler;
};

struct k_work_delayable {
  struct k_work work;
  bool scheduled;
  int64_t delay_ms;
  int64_t due_ms;
};

typedef int64_t k_timeout_t;
#define K_NO_WAIT ((k_timeout_t)0)
#define K_MSEC(ms) ((k_timeout_t)(ms))

#ifndef CONTAINER_OF
#define CONTAINER_OF(ptr, type, field)                                         \
  ((type *)((char *)(ptr) - offsetof(type, field)))
#endif

static inline struct k_work_delayable *
k_work_delayable_from_work(struct k_work *work) {
  return CONTAINER_OF(work, struct k_work_delayable, work);
}

#define INPUT_REL_X 0x00
#define INPUT_REL_Y 0x01
#define INPUT_REL_HWHEEL 0x06
#define INPUT_REL_WHEEL 0x08

#define INPUT_BTN_LEFT 0x110
#define INPUT_BTN_RIGHT 0x111
#define INPUT_BTN_MIDDLE 0x112
#define INPUT_BTN_3 0x113
#define INPUT_BTN_4 0x114
#define INPUT_BTN_5 0x115
#define INPUT_BTN_6 0x116

#define GPIO_INPUT (1U << 0)
#define GPIO_PULL_UP (1U << 1)
#define GPIO_INT_EDGE_TO_ACTIVE (1U << 2)
#define BIT(n) (1UL << (n))
#define ENODEV 19

#define LOG_ERR(...)                                                           \
  do {                                                                         \
  } while (0)
#define LOG_INF(...)                                                           \
  do {                                                                         \
  } while (0)
#define LOG_WRN(...)                                                           \
  do {                                                                         \
  } while (0)
#define LOG_DBG(...)                                                           \
  do {                                                                         \
  } while (0)

/* Mock state */
static int64_t mock_now_ms = 0;
static uint8_t mock_packet_queue[64][64];
static int mock_packet_head = 0;
static int mock_packet_tail = 0;
static bool mock_force_irq_high = false;
static int mock_i2c_read_calls = 0;
static uint32_t mock_last_i2c_read_len = 0;

struct mock_event {
  uint16_t code;
  int32_t value;
  bool sync;
  int64_t timestamp;
};

static struct mock_event mock_rel_events[128];
static int mock_rel_event_count = 0;

static struct mock_event mock_key_events[128];
static int mock_key_event_count = 0;

static int64_t k_uptime_get(void) { return mock_now_ms; }
static void k_msleep(int32_t ms) { mock_now_ms += ms; }

static void k_work_init_delayable(struct k_work_delayable *dwork,
                                  k_work_handler_t handler) {
  dwork->work.handler = handler;
  dwork->scheduled = false;
  dwork->delay_ms = 0;
  dwork->due_ms = 0;
}

static int k_work_schedule(struct k_work_delayable *dwork, k_timeout_t delay) {
  if (!dwork->scheduled) {
    dwork->scheduled = true;
    dwork->delay_ms = delay;
    dwork->due_ms = mock_now_ms + delay;
  }
  return 1;
}

static int k_work_reschedule(struct k_work_delayable *dwork,
                             k_timeout_t delay) {
  dwork->scheduled = true;
  dwork->delay_ms = delay;
  dwork->due_ms = mock_now_ms + delay;
  return 1;
}

static int k_work_cancel_delayable(struct k_work_delayable *dwork) {
  dwork->scheduled = false;
  return 0;
}

static int input_report_rel(const struct device *dev, uint16_t code,
                            int32_t value, bool sync, k_timeout_t timeout) {
  (void)dev;
  (void)timeout;
  assert(mock_rel_event_count < 128);
  mock_rel_events[mock_rel_event_count++] = (struct mock_event){
      .code = code, .value = value, .sync = sync, .timestamp = mock_now_ms};
  return 0;
}

static int input_report_key(const struct device *dev, uint16_t code,
                            int32_t value, bool sync, k_timeout_t timeout) {
  (void)dev;
  (void)timeout;
  assert(mock_key_event_count < 128);
  mock_key_events[mock_key_event_count++] = (struct mock_event){
      .code = code, .value = value, .sync = sync, .timestamp = mock_now_ms};
  return 0;
}

static int gpio_pin_get_dt(const struct gpio_dt_spec *spec) {
  (void)spec;
  if (mock_force_irq_high || (mock_packet_head < mock_packet_tail)) {
    return 1;
  }
  return 0;
}

static bool i2c_is_ready_dt(const struct i2c_dt_spec *spec) {
  (void)spec;
  return true;
}

static bool gpio_is_ready_dt(const struct gpio_dt_spec *spec) {
  (void)spec;
  return true;
}

static int i2c_write(const struct device *dev, const uint8_t *buf,
                     uint32_t num_bytes, uint16_t addr) {
  (void)dev;
  (void)buf;
  (void)num_bytes;
  (void)addr;
  return 0;
}

static int i2c_read(const struct device *dev, uint8_t *buf, uint32_t num_bytes,
                    uint16_t addr) {
  (void)dev;
  (void)addr;
  mock_i2c_read_calls++;
  mock_last_i2c_read_len = num_bytes;
  if (mock_packet_head < mock_packet_tail) {
    uint32_t copy_len = num_bytes <= 64 ? num_bytes : 64;
    memcpy(buf, mock_packet_queue[mock_packet_head], copy_len);
    mock_packet_head++;
    return 0;
  }
  memset(buf, 0, num_bytes);
  return 0;
}

static int gpio_pin_interrupt_configure_dt(const struct gpio_dt_spec *spec,
                                           uint32_t flags) {
  (void)spec;
  (void)flags;
  return 0;
}

static void gpio_init_callback(struct gpio_callback *callback, void *handler,
                               gpio_port_pins_t pin_mask) {
  callback->handler = handler;
  callback->pin_mask = pin_mask;
}

static int gpio_add_callback(const struct device *port,
                             struct gpio_callback *callback) {
  (void)port;
  (void)callback;
  return 0;
}

static int gpio_pin_configure_dt(const struct gpio_dt_spec *spec,
                                 gpio_flags_t extra_flags) {
  (void)spec;
  (void)extra_flags;
  return 0;
}

/* Include driver implementation directly under unit test guard */
#include "../zmk-config/drivers/input/input_synaptics_tm_p3125.c"

/* ========================================================================= */
/* Test Helpers                                                              */
/* ========================================================================= */

static struct synaptics_config test_cfg;
static struct synaptics_data test_data;
static struct device test_dev = {
    .config = &test_cfg,
    .data = &test_data,
};

static void mock_reset(void) {
  mock_now_ms = 1000;
  mock_packet_head = 0;
  mock_packet_tail = 0;
  mock_force_irq_high = false;
  mock_i2c_read_calls = 0;
  mock_last_i2c_read_len = 0;
  mock_rel_event_count = 0;
  mock_key_event_count = 0;

  memset(&test_cfg, 0, sizeof(test_cfg));
  memset(&test_data, 0, sizeof(test_data));
  synaptics_init(&test_dev);
}

static void mock_enqueue_ptp_packet(uint8_t status0, uint16_t x0, uint16_t y0,
                                    uint8_t status1, uint16_t x1, uint16_t y1,
                                    uint8_t status2, uint16_t x2, uint16_t y2,
                                    uint8_t contact_count, bool btn) {
  assert(mock_packet_tail < 64);
  uint8_t *pkt = mock_packet_queue[mock_packet_tail++];
  memset(pkt, 0, 64);
  pkt[0] = 0x20;
  pkt[1] = 0x00;
  pkt[2] = SYNAPTICS_REPORT_TOUCH;

  pkt[3] = status0;
  pkt[4] = (uint8_t)(x0 & 0xFF);
  pkt[5] = (uint8_t)((x0 >> 8) & 0xFF);
  pkt[6] = (uint8_t)(y0 & 0xFF);
  pkt[7] = (uint8_t)((y0 >> 8) & 0xFF);

  pkt[8] = status1;
  pkt[9] = (uint8_t)(x1 & 0xFF);
  pkt[10] = (uint8_t)((x1 >> 8) & 0xFF);
  pkt[11] = (uint8_t)(y1 & 0xFF);
  pkt[12] = (uint8_t)((y1 >> 8) & 0xFF);

  pkt[13] = status2;
  pkt[14] = (uint8_t)(x2 & 0xFF);
  pkt[15] = (uint8_t)((x2 >> 8) & 0xFF);
  pkt[16] = (uint8_t)(y2 & 0xFF);
  pkt[17] = (uint8_t)((y2 >> 8) & 0xFF);

  pkt[30] = contact_count;
  pkt[31] = btn ? 0x01 : 0x00;
}

static void mock_step_work(struct k_work_delayable *dwork) {
  dwork->scheduled = false;
  dwork->work.handler(&dwork->work);
}

/* ========================================================================= */
/* Unit Tests                                                                */
/* ========================================================================= */

static void test_i2c_32_byte_read_and_6_packet_cap(void) {
  mock_reset();

  for (int i = 0; i < 10; i++) {
    mock_enqueue_ptp_packet(0x03, (uint16_t)(300 + i * 5), 300, 0, 0, 0, 0, 0,
                            0, 1, false);
  }

  mock_step_work(&test_data.work);

  assert(mock_i2c_read_calls == 6);
  assert(mock_last_i2c_read_len == 32);
  assert(test_data.work.scheduled == true);
  assert(test_data.work.delay_ms == 1);
}

static void test_ble_8ms_coalescing_and_stranded_flush(void) {
  mock_reset();

  /* Initial touch-down at t = -25 ms and first movement report at t = 0 ms
   * (past 20 ms grace window) to prime BLE timer at t = 0 ms */
  mock_now_ms = -25;
  mock_enqueue_ptp_packet(0x03, 300, 300, 0, 0, 0, 0, 0, 0, 1, false);
  mock_step_work(&test_data.work);

  mock_now_ms = 0;
  mock_enqueue_ptp_packet(0x03, 305, 305, 0, 0, 0, 0, 0, 0, 1, false);
  mock_step_work(&test_data.work);
  assert(mock_rel_event_count == 2);
  mock_rel_event_count = 0;

  /* Feed 4 packets at t = 1, 2, 3, 4 ms with irq_gpio dropping to 0 after packet 4 */
  for (int t = 1; t <= 4; t++) {
    mock_now_ms = t;
    mock_enqueue_ptp_packet(0x03, (uint16_t)(305 + t * 10),
                            (uint16_t)(305 + t * 5), 0, 0, 0, 0, 0, 0, 1,
                            false);
    mock_step_work(&test_data.work);
    assert(mock_rel_event_count == 0);
  }

  assert(gpio_pin_get_dt(&test_cfg.irq_gpio) == 0);
  assert(test_data.work.scheduled == true);
  assert(test_data.work.delay_ms <= 8);

  /* Execute scheduled flush at t = 8 ms */
  mock_now_ms = 8;
  mock_step_work(&test_data.work);

  assert(mock_rel_event_count == 2);
  assert(mock_rel_events[0].code == INPUT_REL_X);
  assert(mock_rel_events[0].value == 40);
  assert(mock_rel_events[1].code == INPUT_REL_Y);
  assert(mock_rel_events[1].value == 20);
  assert(mock_rel_events[1].sync == true);
}

static void test_confidence_and_palm_edge_rejection(void) {
  mock_reset();

  /* 1. Feed 1F touch with status0 = 0x02 (Confidence == 0) -> assert ignored */
  mock_now_ms = 10;
  mock_enqueue_ptp_packet(0x02, 400, 400, 0, 0, 0, 0, 0, 0, 1, false);
  mock_enqueue_ptp_packet(0x02, 420, 420, 0, 0, 0, 0, 0, 0, 1, false);
  mock_step_work(&test_data.work);
  assert(test_data.prev_touching == false);
  assert(mock_rel_event_count == 0);

  /* 2. Feed 1F touch at x0 = 40 (< PALM_EDGE_LEFT 70) with status0 = 0x03 -> palm suppressed */
  mock_now_ms = 20;
  mock_enqueue_ptp_packet(0x03, 40, 400, 0, 0, 0, 0, 0, 0, 1, false);
  mock_enqueue_ptp_packet(0x03, 60, 410, 0, 0, 0, 0, 0, 0, 1, false);
  mock_enqueue_ptp_packet(0x03, 85, 420, 0, 0, 0, 0, 0, 0, 1, false);
  mock_step_work(&test_data.work);
  assert(test_data.palm_suppressed == true);
  assert(mock_rel_event_count == 0);

  /* 3. Move into interior [100, 1120] -> clears palm_suppressed and re-anchors */
  mock_now_ms = 30;
  mock_enqueue_ptp_packet(0x03, 110, 430, 0, 0, 0, 0, 0, 0, 1, false);
  mock_step_work(&test_data.work);
  assert(test_data.palm_suppressed == false);
  assert(mock_rel_event_count == 0);

  /* 4. Subsequent frame inside interior produces normal relative movement
   * (raw_dx = 15, raw_dy = 10 -> v = 25, gain = 24/16 -> out_dx = 22, out_dy = 15) */
  mock_now_ms = 40;
  mock_enqueue_ptp_packet(0x03, 125, 440, 0, 0, 0, 0, 0, 0, 1, false);
  mock_step_work(&test_data.work);
  assert(mock_rel_event_count == 2);
  assert(mock_rel_events[0].code == INPUT_REL_X);
  assert(mock_rel_events[0].value == 22);
  assert(mock_rel_events[1].code == INPUT_REL_Y);
  assert(mock_rel_events[1].value == 15);
}

static void test_gap_reanchoring_and_spike_recovery(void) {
  mock_reset();

  /* Feed 1F at (300, 300) at t = 0 */
  mock_now_ms = 0;
  mock_enqueue_ptp_packet(0x03, 300, 300, 0, 0, 0, 0, 0, 0, 1, false);
  mock_step_work(&test_data.work);
  assert(mock_rel_event_count == 0);

  /* Feed (600, 600) at t = 60 ms (> 45 ms gap) -> re-anchored with 0 delta */
  mock_now_ms = 60;
  mock_enqueue_ptp_packet(0x03, 600, 600, 0, 0, 0, 0, 0, 0, 1, false);
  mock_step_work(&test_data.work);
  assert(mock_rel_event_count == 0);
  assert(test_data.prev_x0 == 600);
  assert(test_data.prev_y0 == 600);

  /* Feed (900, 300) at t = 64 ms (dx = 300 >= 250, dy = -300 <= -250 spike) ->
   * spike rejected AND prev_x0/prev_y0 updated to (900, 300) */
  mock_now_ms = 64;
  mock_enqueue_ptp_packet(0x03, 900, 300, 0, 0, 0, 0, 0, 0, 1, false);
  mock_step_work(&test_data.work);
  assert(mock_rel_event_count == 0);
  assert(test_data.prev_x0 == 900);
  assert(test_data.prev_y0 == 300);

  /* Next frame at (910, 300) at t = 72 ms produces normal movement (dx = 10) */
  mock_now_ms = 72;
  mock_enqueue_ptp_packet(0x03, 910, 300, 0, 0, 0, 0, 0, 0, 1, false);
  mock_step_work(&test_data.work);
  assert(mock_rel_event_count == 1);
  assert(mock_rel_events[0].code == INPUT_REL_X);
  assert(mock_rel_events[0].value == 10);
  assert(mock_rel_events[0].sync == true);
}

static void test_1f_ballistics_and_subpixel(void) {
  mock_reset();

  int16_t out_dx = 0;
  int16_t out_dy = 0;

  /* 1. Slow step raw_dx = 1, raw_dy = 0 (v <= 6, EMA + Q4 gain 12/16):
   * preserves fractional subpixel_rem_x across calls and emits +1 once >= 16 */
  synaptics_apply_ballistics(&test_data, 1, 0, &out_dx, &out_dy);
  assert(out_dx == 0);
  assert(out_dy == 0);
  assert(test_data.subpixel_rem_x == 12);

  synaptics_apply_ballistics(&test_data, 1, 0, &out_dx, &out_dy);
  assert(out_dx == 1);
  assert(out_dy == 0);
  assert(test_data.subpixel_rem_x == 8);

  test_data.subpixel_rem_x = 0;
  test_data.subpixel_rem_y = 0;

  /* 2. Medium step raw_dx = 12, raw_dy = 0 (6 < v <= 22, Q4 gain 16/16) -> 12 */
  synaptics_apply_ballistics(&test_data, 12, 0, &out_dx, &out_dy);
  assert(out_dx == 12);
  assert(out_dy == 0);

  /* 3. Fast step raw_dx = 30, raw_dy = 0 (22 < v <= 50, Q4 gain 24/16) -> 45 */
  synaptics_apply_ballistics(&test_data, 30, 0, &out_dx, &out_dy);
  assert(out_dx == 45);
  assert(out_dy == 0);

  /* 4. Swipe step raw_dx = 60, raw_dy = 0 (v > 50, Q4 gain 32/16) -> 120 */
  synaptics_apply_ballistics(&test_data, 60, 0, &out_dx, &out_dy);
  assert(out_dx == 120);
  assert(out_dy == 0);
}

static void test_1f_tap_and_grace_window_early_lift(void) {
  mock_reset();

  /* Touch 1F at (400, 400) at t = 0 */
  mock_now_ms = 0;
  mock_enqueue_ptp_packet(0x03, 400, 400, 0, 0, 0, 0, 0, 0, 1, false);
  mock_step_work(&test_data.work);
  assert(test_data.in_grace_window == true);
  assert(mock_rel_event_count == 0);

  /* Move to (410, 405) at t = 8 ms (inside 20 ms / < 35 unit grace window) */
  mock_now_ms = 8;
  mock_enqueue_ptp_packet(0x03, 410, 405, 0, 0, 0, 0, 0, 0, 1, false);
  mock_step_work(&test_data.work);
  assert(test_data.in_grace_window == true);
  assert(test_data.grace_dx == 10);
  assert(test_data.grace_dy == 5);
  assert(mock_rel_event_count == 0);

  /* Lift at t = 15 ms -> 0 cursor movement emitted, grace cleared, tap click emitted */
  mock_now_ms = 15;
  mock_enqueue_ptp_packet(0x00, 0, 0, 0, 0, 0, 0, 0, 0, 0, false);
  mock_step_work(&test_data.work);
  assert(mock_rel_event_count == 0);
  assert(test_data.in_grace_window == false);
  assert(test_data.grace_dx == 0);
  assert(test_data.grace_dy == 0);
  assert(mock_key_event_count == 1);
  assert(mock_key_events[0].code == INPUT_BTN_LEFT);
  assert(mock_key_events[0].value == 1);
  assert(test_data.tap_release_work.scheduled == true);
  assert(test_data.tap_release_work.delay_ms == 40);

  /* Advance 40 ms to t = 55 ms and execute tap release */
  mock_now_ms = 55;
  mock_step_work(&test_data.tap_release_work);
  assert(mock_key_event_count == 2);
  assert(mock_key_events[1].code == INPUT_BTN_LEFT);
  assert(mock_key_events[1].value == 0);
}

static void test_1f_double_tap_drag_and_liftoff_release(void) {
  mock_reset();

  /* 1. Perform 1F tap at (400, 400) (t = 0..50 ms) */
  mock_now_ms = 0;
  mock_enqueue_ptp_packet(0x03, 400, 400, 0, 0, 0, 0, 0, 0, 1, false);
  mock_step_work(&test_data.work);

  mock_now_ms = 50;
  mock_enqueue_ptp_packet(0x00, 0, 0, 0, 0, 0, 0, 0, 0, 0, false);
  mock_step_work(&test_data.work);
  assert(mock_key_event_count == 1);
  assert(mock_key_events[0].code == INPUT_BTN_LEFT);
  assert(mock_key_events[0].value == 1);

  mock_now_ms = 90;
  mock_step_work(&test_data.tap_release_work);
  assert(mock_key_event_count == 2);
  assert(mock_key_events[1].value == 0);
  mock_key_event_count = 0;

  /* 2. Re-touch at (410, 410) at t = 150 ms (< 400 ms and < 140 units) and move */
  mock_now_ms = 150;
  mock_enqueue_ptp_packet(0x03, 410, 410, 0, 0, 0, 0, 0, 0, 1, false);
  mock_step_work(&test_data.work);
  assert(test_data.is_tap_dragging == true);
  assert(mock_key_event_count >= 1);
  assert(mock_key_events[mock_key_event_count - 1].code == INPUT_BTN_LEFT);
  assert(mock_key_events[mock_key_event_count - 1].value == 1);

  /* Stationary pause for 80 ms: Left button MUST remain held down */
  mock_now_ms = 230;
  mock_enqueue_ptp_packet(0x03, 410, 410, 0, 0, 0, 0, 0, 0, 1, false);
  mock_step_work(&test_data.work);
  assert(test_data.is_tap_dragging == true);
  assert(mock_key_events[mock_key_event_count - 1].code == INPUT_BTN_LEFT);
  assert(mock_key_events[mock_key_event_count - 1].value == 1);

  /* Continuous move while dragging */
  mock_now_ms = 250;
  mock_enqueue_ptp_packet(0x03, 425, 420, 0, 0, 0, 0, 0, 0, 1, false);
  mock_step_work(&test_data.work);
  assert(test_data.is_tap_dragging == true);

  /* 3. Lift finger at t = 300 ms -> INPUT_BTN_LEFT immediately released (0)
   * and is_tap_dragging set to false */
  int key_events_before_lift = mock_key_event_count;
  mock_now_ms = 300;
  mock_enqueue_ptp_packet(0x00, 0, 0, 0, 0, 0, 0, 0, 0, 0, false);
  mock_step_work(&test_data.work);
  assert(mock_key_event_count == key_events_before_lift + 1);
  assert(mock_key_events[mock_key_event_count - 1].code == INPUT_BTN_LEFT);
  assert(mock_key_events[mock_key_event_count - 1].value == 0);
  assert(test_data.is_tap_dragging == false);
}

static void test_physical_clickpad_zones_and_liftoff_before_release(void) {
  mock_reset();

  /* 1. Press physical click (buf[31] = 1) with 1F at (800, 750) (x0 > 650 && y0 > 650) */
  mock_now_ms = 0;
  mock_enqueue_ptp_packet(0x03, 800, 750, 0, 0, 0, 0, 0, 0, 1, true);
  mock_step_work(&test_data.work);
  assert(mock_key_event_count == 1);
  assert(mock_key_events[0].code == INPUT_BTN_RIGHT);
  assert(mock_key_events[0].value == 1);

  /* 2. Lift finger (contacts = 0, buf[31] = 1) -> no tap-to-click fires on liftoff */
  mock_now_ms = 30;
  mock_enqueue_ptp_packet(0x00, 0, 0, 0, 0, 0, 0, 0, 0, 0, true);
  mock_step_work(&test_data.work);
  assert(mock_key_event_count == 1);

  /* 3. Release physical click (buf[31] = 0) -> INPUT_BTN_RIGHT released (0), NOT INPUT_BTN_LEFT */
  mock_now_ms = 60;
  mock_enqueue_ptp_packet(0x00, 0, 0, 0, 0, 0, 0, 0, 0, 0, false);
  mock_step_work(&test_data.work);
  assert(mock_key_event_count == 2);
  assert(mock_key_events[1].code == INPUT_BTN_RIGHT);
  assert(mock_key_events[1].value == 0);
}

static void test_2f_tap_with_settling_jitter(void) {
  mock_reset();

  /* Touch 2 fingers at (400, 400) and (600, 400): centroid (500, 400), span 200 */
  mock_now_ms = 0;
  mock_enqueue_ptp_packet(0x03, 400, 400, 0x03, 600, 400, 0, 0, 0, 2, false);
  mock_step_work(&test_data.work);

  /* Jitter centroid by 30 units (< 45 deadzone) and span by 20 units (< 55 deadzone):
   * Finger 0 at (390, 430), Finger 1 at (610, 430) -> centroid (500, 430), span 220 */
  mock_now_ms = 40;
  mock_enqueue_ptp_packet(0x03, 390, 430, 0x03, 610, 430, 0, 0, 0, 2, false);
  mock_step_work(&test_data.work);
  assert(test_data.two_finger_mode == TWO_FINGER_IDLE);
  assert(mock_rel_event_count == 0);
  assert(mock_key_event_count == 0);

  /* Lift after 120 ms -> assert INPUT_BTN_RIGHT click emitted (1 then 0 after 40 ms) */
  mock_now_ms = 120;
  mock_enqueue_ptp_packet(0x00, 0, 0, 0, 0, 0, 0, 0, 0, 0, false);
  mock_step_work(&test_data.work);
  assert(mock_rel_event_count == 0);
  assert(mock_key_event_count == 1);
  assert(mock_key_events[0].code == INPUT_BTN_RIGHT);
  assert(mock_key_events[0].value == 1);
  assert(test_data.tap_right_release_work.scheduled == true);
  assert(test_data.tap_right_release_work.delay_ms == 40);

  mock_now_ms = 160;
  mock_step_work(&test_data.tap_right_release_work);
  assert(mock_key_event_count == 2);
  assert(mock_key_events[1].code == INPUT_BTN_RIGHT);
  assert(mock_key_events[1].value == 0);
}

static void test_2f_natural_scroll_aggregated_in_8ms_frame(void) {
  mock_reset();

  /* Prime BLE frame timer at t = 0 ms and touch 2 fingers at (400, 500) and (600, 500) */
  mock_now_ms = 0;
  test_data.has_ble_report = true;
  test_data.last_ble_report_time = 0;
  mock_enqueue_ptp_packet(0x03, 400, 500, 0x03, 600, 500, 0, 0, 0, 2, false);
  mock_step_work(&test_data.work);

  /* Move parallel up: dy = -30 per packet for 3 packets within 6 ms (total dy = -90) */
  for (int i = 1; i <= 3; i++) {
    mock_now_ms = i * 2; /* t = 2, 4, 6 ms */
    uint16_t y = (uint16_t)(500 - i * 30);
    mock_enqueue_ptp_packet(0x03, 400, y, 0x03, 600, y, 0, 0, 0, 2, false);
    mock_step_work(&test_data.work);
    /* Assert INPUT_REL_WHEEL is NOT emitted inside the packet loop during t = 1..6 ms */
    assert(mock_rel_event_count == 0);
  }

  assert(test_data.two_finger_mode == TWO_FINGER_SCROLL);
  assert(test_data.pend_wheel == -2);

  /* At t = 8 ms BLE flush, emitted once as a single aggregated tick count */
  mock_now_ms = 8;
  mock_step_work(&test_data.work);
  assert(mock_rel_event_count == 1);
  assert(mock_rel_events[0].code == INPUT_REL_WHEEL);
  assert(mock_rel_events[0].value == -2);
  assert(mock_rel_events[0].sync == true);

  /* Verify horizontal scroll when |dx| > 2 * |dy| accumulates into pend_hwheel */
  mock_rel_event_count = 0;
  mock_now_ms = 10;
  mock_enqueue_ptp_packet(0x03, 460, 420, 0x03, 660, 420, 0, 0, 0, 2, false);
  mock_step_work(&test_data.work);
  assert(test_data.pend_hwheel == 2);
  assert(mock_rel_event_count == 0);

  mock_now_ms = 16;
  mock_step_work(&test_data.work);
  assert(mock_rel_event_count == 1);
  assert(mock_rel_events[0].code == INPUT_REL_HWHEEL);
  assert(mock_rel_events[0].value == 2);
  assert(mock_rel_events[0].sync == true);
}

static void test_2f_pinch_to_zoom_in_and_out_rapid(void) {
  mock_reset();

  /* Touch 2 fingers at (300, 400) and (600, 400) (initial span 300 >= 160) */
  mock_now_ms = 0;
  mock_enqueue_ptp_packet(0x03, 300, 400, 0x03, 600, 400, 0, 0, 0, 2, false);
  mock_step_work(&test_data.work);

  /* Expand span by +180 units (> PINCH_STEP, where PINCH_STEP = 85) within 10 ms:
   * Finger 0 at (210, 400), Finger 1 at (690, 400) -> centroid (450, 400), span 480 (+180) */
  mock_now_ms = 5;
  mock_enqueue_ptp_packet(0x03, 210, 400, 0x03, 690, 400, 0, 0, 0, 2, false);
  mock_step_work(&test_data.work);

  assert(test_data.two_finger_mode == TWO_FINGER_PINCH);
  assert(mock_key_event_count == 1);
  assert(mock_key_events[0].code == INPUT_BTN_5 && mock_key_events[0].value == 1);
  assert(test_data.pinch_in_release_work.scheduled == true);
  assert(test_data.pinch_in_release_work.delay_ms == 25);

  /* Execute release at t = 30 ms */
  mock_now_ms = 30;
  mock_step_work(&test_data.pinch_in_release_work);
  assert(mock_key_event_count == 2);
  assert(mock_key_events[1].code == INPUT_BTN_5 && mock_key_events[1].value == 0);

  /* Contract span by -90 units at t = 70 ms (>= 60 ms rate limit from t = 5 ms):
   * Finger 0 at (255, 400), Finger 1 at (645, 400) -> centroid (450, 400), span 390 (-90) */
  mock_key_event_count = 0;
  mock_now_ms = 70;
  mock_enqueue_ptp_packet(0x03, 255, 400, 0x03, 645, 400, 0, 0, 0, 2, false);
  mock_step_work(&test_data.work);

  assert(mock_key_event_count == 1);
  assert(mock_key_events[0].code == INPUT_BTN_6 && mock_key_events[0].value == 1);
  assert(test_data.pinch_out_release_work.scheduled == true);
  assert(test_data.pinch_out_release_work.delay_ms == 25);

  /* Execute release at t = 95 ms */
  mock_now_ms = 95;
  mock_step_work(&test_data.pinch_out_release_work);
  assert(mock_key_event_count == 2);
  assert(mock_key_events[1].code == INPUT_BTN_6 && mock_key_events[1].value == 0);
}

static void test_1f_to_2f_grace_and_2f_to_1f_liftoff_guard(void) {
  mock_reset();

  /* Touch 1F at t = 0 */
  mock_now_ms = 0;
  mock_enqueue_ptp_packet(0x03, 400, 400, 0, 0, 0, 0, 0, 0, 1, false);
  mock_step_work(&test_data.work);

  /* Move (15, 15) at t = 8 ms (buffered in grace_dx/grace_dy) */
  mock_now_ms = 8;
  mock_enqueue_ptp_packet(0x03, 415, 415, 0, 0, 0, 0, 0, 0, 1, false);
  mock_step_work(&test_data.work);
  assert(test_data.in_grace_window == true);
  assert(test_data.grace_dx != 0 && test_data.grace_dy != 0);
  assert(mock_rel_event_count == 0);

  /* Land 2nd finger at t = 12 ms -> grace_dx/grace_dy discarded, zero INPUT_REL_X/Y emitted */
  mock_now_ms = 12;
  mock_enqueue_ptp_packet(0x03, 415, 415, 0x03, 615, 415, 0, 0, 0, 2, false);
  mock_step_work(&test_data.work);
  assert(test_data.in_grace_window == false);
  assert(test_data.grace_dx == 0 && test_data.grace_dy == 0);
  assert(mock_rel_event_count == 0);

  /* Scroll with 2F at t = 20 ms */
  mock_now_ms = 20;
  mock_enqueue_ptp_packet(0x03, 415, 475, 0x03, 615, 475, 0, 0, 0, 2, false);
  mock_step_work(&test_data.work);
  assert(test_data.two_finger_mode == TWO_FINGER_SCROLL);
  mock_rel_event_count = 0;

  /* Lift 2nd finger leaving 1F moving (50, 50) while gesture_max_fingers == 2 */
  mock_now_ms = 30;
  mock_enqueue_ptp_packet(0x03, 465, 525, 0x00, 0, 0, 0, 0, 0, 1, false);
  mock_step_work(&test_data.work);

  mock_now_ms = 40;
  mock_enqueue_ptp_packet(0x03, 515, 575, 0x00, 0, 0, 0, 0, 0, 1, false);
  mock_step_work(&test_data.work);
  assert(test_data.gesture_max_fingers == 2);
  assert(mock_rel_event_count == 0);

  /* Lift all fingers (contacts == 0) */
  mock_now_ms = 50;
  mock_enqueue_ptp_packet(0x00, 0, 0, 0, 0, 0, 0, 0, 0, 0, false);
  mock_step_work(&test_data.work);
  assert(test_data.gesture_max_fingers == 0);
  assert(mock_rel_event_count == 0);
  assert(mock_key_event_count == 0);
}

static void test_3f_tap_and_swipes(void) {
  /* 1. 3F tap (< 280 ms, no swipe) emits INPUT_BTN_MIDDLE */
  mock_reset();
  mock_now_ms = 0;
  mock_enqueue_ptp_packet(0x03, 300, 400, 0x03, 500, 400, 0x03, 700, 400, 3,
                          false);
  mock_step_work(&test_data.work);

  mock_now_ms = 100;
  mock_enqueue_ptp_packet(0x00, 0, 0, 0, 0, 0, 0, 0, 0, 0, false);
  mock_step_work(&test_data.work);
  assert(mock_key_event_count == 1);
  assert(mock_key_events[0].code == INPUT_BTN_MIDDLE);
  assert(mock_key_events[0].value == 1);
  assert(test_data.tap_middle_release_work.scheduled == true);
  assert(test_data.tap_middle_release_work.delay_ms == 40);

  mock_now_ms = 140;
  mock_step_work(&test_data.tap_middle_release_work);
  assert(mock_key_event_count == 2);
  assert(mock_key_events[1].code == INPUT_BTN_MIDDLE);
  assert(mock_key_events[1].value == 0);

  /* 2. 3F swipe up (swipe_dy < -120) emits INPUT_BTN_3 */
  mock_reset();
  mock_now_ms = 0;
  mock_enqueue_ptp_packet(0x03, 300, 500, 0x03, 500, 500, 0x03, 700, 500, 3,
                          false);
  mock_step_work(&test_data.work);

  mock_now_ms = 20;
  mock_enqueue_ptp_packet(0x03, 300, 350, 0x03, 500, 350, 0x03, 700, 350, 3,
                          false);
  mock_step_work(&test_data.work);
  assert(mock_key_event_count == 1);
  assert(mock_key_events[0].code == INPUT_BTN_3);
  assert(mock_key_events[0].value == 1);
  assert(test_data.swipe_up_release_work.scheduled == true);
  assert(test_data.swipe_up_release_work.delay_ms == 40);

  /* 3. 3F swipe down (swipe_dy > 120) emits INPUT_BTN_4 */
  mock_reset();
  mock_now_ms = 0;
  mock_enqueue_ptp_packet(0x03, 300, 300, 0x03, 500, 300, 0x03, 700, 300, 3,
                          false);
  mock_step_work(&test_data.work);

  mock_now_ms = 20;
  mock_enqueue_ptp_packet(0x03, 300, 450, 0x03, 500, 450, 0x03, 700, 450, 3,
                          false);
  mock_step_work(&test_data.work);
  assert(mock_key_event_count == 1);
  assert(mock_key_events[0].code == INPUT_BTN_4);
  assert(test_data.swipe_down_release_work.delay_ms == 40);
}

static void test_double_tap_offset_no_cursor_jump_and_enters_drag(void) {
  mock_reset();

  /* 1. Perform Tap 1 at (400, 400) (t = 0..30 ms) */
  mock_now_ms = 0;
  mock_enqueue_ptp_packet(0x03, 400, 400, 0, 0, 0, 0, 0, 0, 1, false);
  mock_step_work(&test_data.work);

  /* Liftoff at t = 30 ms */
  mock_now_ms = 30;
  mock_enqueue_ptp_packet(0x00, 0, 0, 0, 0, 0, 0, 0, 0, 0, false);
  mock_step_work(&test_data.work);
  assert(mock_key_event_count == 1);
  assert(mock_key_events[0].code == INPUT_BTN_LEFT && mock_key_events[0].value == 1);

  /* Release Tap 1 at t = 70 ms */
  mock_now_ms = 70;
  mock_step_work(&test_data.tap_release_work);
  assert(mock_key_event_count == 2);
  assert(mock_key_events[1].value == 0);
  mock_key_event_count = 0;
  mock_rel_event_count = 0;

  /* 2. Tap 2 lands at (600, 400) at t = 140 ms (d_tap_x = 200, which is > 140) */
  mock_now_ms = 140;
  mock_enqueue_ptp_packet(0x03, 600, 400, 0, 0, 0, 0, 0, 0, 1, false);
  mock_step_work(&test_data.work);

  /* Assertions:
   * 1. Must recognize tap-and-drag (is_tap_dragging == true).
   * 2. Must NOT emit ANY relative movement on touch down (mock_rel_event_count == 0).
   * 3. Must hold INPUT_BTN_LEFT.
   */
  assert(test_data.is_tap_dragging == true);
  assert(mock_rel_event_count == 0);
  assert(mock_key_event_count >= 1);
  assert(mock_key_events[mock_key_event_count - 1].code == INPUT_BTN_LEFT);
  assert(mock_key_events[mock_key_event_count - 1].value == 1);

  /* 2b. Hold finger stationary for 500 ms with 0 packets read (inactivity):
   * Because is_tap_dragging == true, the 250 ms timeout does NOT release Left button!
   * Drag timeout is 1500 ms! */
  mock_now_ms = 400;
  mock_step_work(&test_data.work);
  assert(test_data.is_tap_dragging == true);
  assert(mock_key_events[mock_key_event_count - 1].code == INPUT_BTN_LEFT);
  assert(mock_key_events[mock_key_event_count - 1].value == 1);

  /* 2c. Even if status0 bit 0 drops (confidence flutter, status0 = 0x02):
   * tip0 remains true because Tip Switch is held! */
  mock_now_ms = 450;
  mock_enqueue_ptp_packet(0x02, 600, 400, 0, 0, 0, 0, 0, 0, 1, false);
  mock_step_work(&test_data.work);
  assert(test_data.is_tap_dragging == true);
  assert(mock_key_events[mock_key_event_count - 1].code == INPUT_BTN_LEFT);
  assert(mock_key_events[mock_key_event_count - 1].value == 1);

  /* 3. Now move continuously from (600, 400) to (630, 400) at t = 470 ms */
  mock_now_ms = 470;
  mock_enqueue_ptp_packet(0x03, 630, 400, 0, 0, 0, 0, 0, 0, 1, false);
  mock_step_work(&test_data.work);

  /* Relative movement should be emitted continuously from 600 -> 630 (dx = 30) */
  assert(mock_rel_event_count >= 1);
  assert(mock_rel_events[0].code == INPUT_REL_X);
  assert(mock_rel_events[0].value > 0);

  /* 4. Liftoff at t = 400 ms: releases INPUT_BTN_LEFT = 0 */
  int keys_before_lift = mock_key_event_count;
  mock_now_ms = 400;
  mock_enqueue_ptp_packet(0x00, 0, 0, 0, 0, 0, 0, 0, 0, 0, false);
  mock_step_work(&test_data.work);
  assert(mock_key_event_count > keys_before_lift);
  assert(mock_key_events[mock_key_event_count - 1].code == INPUT_BTN_LEFT);
  assert(mock_key_events[mock_key_event_count - 1].value == 0);
  assert(test_data.is_tap_dragging == false);
}

static void test_double_tap_without_liftoff_packet_no_cursor_jump(void) {
  mock_reset();

  /* 1. Touch at (400, 400) at t = 0 */
  mock_now_ms = 0;
  mock_enqueue_ptp_packet(0x03, 400, 400, 0, 0, 0, 0, 0, 0, 1, false);
  mock_step_work(&test_data.work);

  /* 2. Finger lifts at t = 30 ms WITHOUT sending a 0x00 packet.
   * Work handler runs with 0 packets read at t = 280 ms (>= 250 ms liftoff timeout). */
  mock_now_ms = 280;
  mock_step_work(&test_data.work);
  /* Fast liftoff watchdog should have triggered liftoff! */
  assert(test_data.prev_touching == false);

  /* 3. Tap 2 lands at (550, 400) at t = 350 ms */
  mock_rel_event_count = 0;
  mock_now_ms = 350;
  mock_enqueue_ptp_packet(0x03, 550, 400, 0, 0, 0, 0, 0, 0, 1, false);
  mock_step_work(&test_data.work);

  /* MUST NOT emit cursor jump */
  assert(mock_rel_event_count == 0);
}

static void test_discontinuity_jump_filter_prevents_teleportation(void) {
  mock_reset();

  /* Touch at (400, 400) */
  mock_now_ms = 0;
  mock_enqueue_ptp_packet(0x03, 400, 400, 0, 0, 0, 0, 0, 0, 1, false);
  mock_step_work(&test_data.work);

  /* Touch down teleportation to (700, 400) (dx = 300 > 250) within 15 ms without liftoff packet */
  mock_now_ms = 15;
  mock_enqueue_ptp_packet(0x03, 700, 400, 0, 0, 0, 0, 0, 0, 1, false);
  mock_step_work(&test_data.work);

  /* Under our discontinuity guard, dx=300 is rejected as continuous motion, so 0 delta emitted */
  assert(mock_rel_event_count == 0);
  assert(test_data.prev_x0 == 700);
}

static void test_pinch_zoom_dropout_no_freeze(void) {
  mock_reset();

  /* Touch 2 fingers at (300, 400) and (600, 400) */
  mock_now_ms = 0;
  mock_enqueue_ptp_packet(0x03, 300, 400, 0x03, 600, 400, 0, 0, 0, 2, false);
  mock_step_work(&test_data.work);

  /* Spread fingers to (210, 400) and (690, 400) (+180 span) */
  mock_now_ms = 10;
  mock_enqueue_ptp_packet(0x03, 210, 400, 0x03, 690, 400, 0, 0, 0, 2, false);
  mock_step_work(&test_data.work);
  assert(test_data.two_finger_mode == TWO_FINGER_PINCH);

  /* Finger 1 drops confidence (0x02 instead of 0x03) near edge */
  mock_now_ms = 20;
  mock_enqueue_ptp_packet(0x03, 200, 400, 0x02, 700, 400, 0, 0, 0, 1, false);
  mock_step_work(&test_data.work);

  /* Must cleanly terminate pinch mode and release INPUT_BTN_5 */
  assert(test_data.two_finger_mode == TWO_FINGER_IDLE);

  /* All fingers lift */
  mock_now_ms = 35;
  mock_enqueue_ptp_packet(0x00, 0, 0, 0, 0, 0, 0, 0, 0, 0, false);
  mock_step_work(&test_data.work);
  assert(test_data.gesture_max_fingers == 0);
}

static void test_tap_release_handler_does_not_abort_active_drag(void) {
  mock_reset();

  /* 1. Perform 1F tap at (400, 400) (t = 0..30 ms) */
  mock_now_ms = 0;
  mock_enqueue_ptp_packet(0x03, 400, 400, 0, 0, 0, 0, 0, 0, 1, false);
  mock_step_work(&test_data.work);

  mock_now_ms = 30;
  mock_enqueue_ptp_packet(0x00, 0, 0, 0, 0, 0, 0, 0, 0, 0, false);
  mock_step_work(&test_data.work);
  assert(mock_key_event_count == 1);
  assert(mock_key_events[0].code == INPUT_BTN_LEFT);
  assert(mock_key_events[0].value == 1);

  /* 2. Tap 2 lands at t = 50 ms (BEFORE tap_release_work fires at 70 ms) */
  mock_now_ms = 50;
  mock_enqueue_ptp_packet(0x03, 410, 410, 0, 0, 0, 0, 0, 0, 1, false);
  mock_step_work(&test_data.work);
  assert(test_data.is_tap_dragging == true);
  assert(mock_key_events[mock_key_event_count - 1].code == INPUT_BTN_LEFT);
  assert(mock_key_events[mock_key_event_count - 1].value == 1);

  /* 3. Delayed tap_release_work runs at t = 70 ms */
  mock_now_ms = 70;
  int key_count_before = mock_key_event_count;
  mock_step_work(&test_data.tap_release_work);

  /* CRITICAL: Must NOT abort drag or release INPUT_BTN_LEFT = 0 */
  assert(test_data.is_tap_dragging == true);
  assert(mock_key_event_count == key_count_before);

  /* 4. Physical liftoff at t = 200 ms: NOW drag releases */
  mock_now_ms = 200;
  mock_enqueue_ptp_packet(0x00, 0, 0, 0, 0, 0, 0, 0, 0, 0, false);
  mock_step_work(&test_data.work);
  assert(test_data.is_tap_dragging == false);
  assert(mock_key_events[mock_key_event_count - 1].code == INPUT_BTN_LEFT);
  assert(mock_key_events[mock_key_event_count - 1].value == 0);
}

static void test_inactivity_watchdog_never_emits_tap_click(void) {
  mock_reset();

  /* 1. Finger touches down at (400, 400) at t = 0 */
  mock_now_ms = 0;
  mock_enqueue_ptp_packet(0x03, 400, 400, 0, 0, 0, 0, 0, 0, 1, false);
  mock_step_work(&test_data.work);
  assert(test_data.prev_touching == true);
  assert(mock_key_event_count == 0);

  /* 2. Finger rests stationary without generating packets.
   * At t = 260 ms (> 250 ms watchdog timeout), watchdog executes */
  mock_now_ms = 260;
  mock_step_work(&test_data.work);

  /* Watchdog cleans up prev_touching */
  assert(test_data.prev_touching == false);
  /* BUT CRITICAL INVARIANT: NO TAP CLICK MAY BE EMITTED ON TIMEOUT */
  assert(mock_key_event_count == 0);
}

static void test_touch_confidence_flutter_does_not_trigger_liftoff_or_tap(void) {
  mock_reset();

  /* 1. Finger touches down at (400, 400) with confidence (0x03) at t = 0 */
  mock_now_ms = 0;
  mock_enqueue_ptp_packet(0x03, 400, 400, 0, 0, 0, 0, 0, 0, 1, false);
  mock_step_work(&test_data.work);
  assert(test_data.prev_touching == true);

  /* 2. At t = 20 ms, confidence bit flickers to 0 (status0 = 0x02, Tip Switch still 1) */
  mock_now_ms = 20;
  mock_enqueue_ptp_packet(0x02, 405, 405, 0, 0, 0, 0, 0, 0, 1, false);
  mock_step_work(&test_data.work);

  /* Contact MUST be maintained, NOT treated as liftoff, NO tap emitted */
  assert(test_data.prev_touching == true);
  assert(mock_key_event_count == 0);

  /* 3. At t = 60 ms, finger physically lifts off (status0 = 0x00) */
  mock_now_ms = 60;
  mock_enqueue_ptp_packet(0x00, 0, 0, 0, 0, 0, 0, 0, 0, 0, false);
  mock_step_work(&test_data.work);

  /* Physical liftoff produces the single tap click */
  assert(mock_key_event_count == 1);
  assert(mock_key_events[0].code == INPUT_BTN_LEFT);
  assert(mock_key_events[0].value == 1);
}

int main(void) {
  test_i2c_32_byte_read_and_6_packet_cap();
  test_ble_8ms_coalescing_and_stranded_flush();
  test_confidence_and_palm_edge_rejection();
  test_gap_reanchoring_and_spike_recovery();
  test_1f_ballistics_and_subpixel();
  test_1f_tap_and_grace_window_early_lift();
  test_1f_double_tap_drag_and_liftoff_release();
  test_tap_release_handler_does_not_abort_active_drag();
  test_inactivity_watchdog_never_emits_tap_click();
  test_touch_confidence_flutter_does_not_trigger_liftoff_or_tap();
  test_physical_clickpad_zones_and_liftoff_before_release();
  test_2f_tap_with_settling_jitter();
  test_2f_natural_scroll_aggregated_in_8ms_frame();
  test_2f_pinch_to_zoom_in_and_out_rapid();
  test_1f_to_2f_grace_and_2f_to_1f_liftoff_guard();
  test_3f_tap_and_swipes();
  test_double_tap_offset_no_cursor_jump_and_enters_drag();
  test_double_tap_without_liftoff_packet_no_cursor_jump();
  test_discontinuity_jump_filter_prevents_teleportation();
  test_pinch_zoom_dropout_no_freeze();
  printf("PASS: all touchpad gesture tests passed\n");
  return 0;
}
