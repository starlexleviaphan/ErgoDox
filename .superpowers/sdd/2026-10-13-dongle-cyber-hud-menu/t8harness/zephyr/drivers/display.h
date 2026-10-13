#pragma once
#include <zephyr/device.h>
enum display_orientation { DISPLAY_ORIENTATION_NORMAL, DISPLAY_ORIENTATION_ROTATED_90, DISPLAY_ORIENTATION_ROTATED_180, DISPLAY_ORIENTATION_ROTATED_270 };
int display_set_orientation(const struct device *dev, enum display_orientation o);

