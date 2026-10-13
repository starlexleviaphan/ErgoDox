#pragma once
#include <stdbool.h>
#include <stdint.h>
struct zmk_position_state_changed { bool state; uint32_t position; };
const struct zmk_position_state_changed *as_zmk_position_state_changed(const void *e);

