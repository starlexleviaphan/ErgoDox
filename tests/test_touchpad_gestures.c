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

  /* Initial touch-down and first movement report at t = 0 ms to prime BLE timer */
  mock_now_ms = 0;
  mock_enqueue_ptp_packet(0x03, 300, 300, 0, 0, 0, 0, 0, 0, 1, false);
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

  /* 4. Subsequent frame inside interior produces normal relative movement */
  mock_now_ms = 40;
  mock_enqueue_ptp_packet(0x03, 125, 440, 0, 0, 0, 0, 0, 0, 1, false);
  mock_step_work(&test_data.work);
  assert(mock_rel_event_count == 2);
  assert(mock_rel_events[0].code == INPUT_REL_X);
  assert(mock_rel_events[0].value == 15);
  assert(mock_rel_events[1].code == INPUT_REL_Y);
  assert(mock_rel_events[1].value == 10);
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

int main(void) {
  test_i2c_32_byte_read_and_6_packet_cap();
  test_ble_8ms_coalescing_and_stranded_flush();
  test_confidence_and_palm_edge_rejection();
  test_gap_reanchoring_and_spike_recovery();
  printf("PASS: all touchpad gesture tests passed\n");
  return 0;
}
