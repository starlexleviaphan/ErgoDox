#pragma once
#include <stdbool.h>
#include <stdint.h>
struct zmk_keycode_state_changed { bool state; uint32_t keycode; };
const struct zmk_keycode_state_changed *as_zmk_keycode_state_changed(const void *e);

