/*
 * Copyright (c) 2026 ErgoDox Wireless Project
 * SPDX-License-Identifier: MIT
 *
 * Unit test for WPM EMA smoothing & gentle decay
 */

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
    /* Test initial state */
    smooth_wpm = 0;
    update_wpm_ema(0);
    assert(smooth_wpm == 0);

    /* Test EMA ramp up */
    update_wpm_ema(60);
    assert(smooth_wpm == 15);

    update_wpm_ema(60);
    assert(smooth_wpm == 26);

    /* Test gentle decay on pause (raw_wpm == 0) */
    update_wpm_ema(0); // Smooth decay instead of instant drop to 0
    assert(smooth_wpm == 23);

    update_wpm_ema(0);
    assert(smooth_wpm == 20);

    /* Test decay down to zero without underflow */
    smooth_wpm = 4;
    update_wpm_ema(0);
    assert(smooth_wpm == 1);

    update_wpm_ema(0);
    assert(smooth_wpm == 0);

    smooth_wpm = 2;
    update_wpm_ema(0);
    assert(smooth_wpm == 0);

    smooth_wpm = 1;
    update_wpm_ema(0);
    assert(smooth_wpm == 0);

    printf("PASS\n");
    return 0;
}
