// tests/test_partial_zoom.c
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define NUM_INSPECTED_LAYERS 15
#define KEYS_PER_HALF 38

#define ZOOM_KEY_WIDTH  44
#define ZOOM_KEY_HEIGHT 38

enum key_category {
    KEY_CAT_ALPHA = 0,
    KEY_CAT_MOD,
    KEY_CAT_LAYER,
    KEY_CAT_MEDIA,
    KEY_CAT_TRANS,
    KEY_CAT_COUNT
};

#include "../zmk-config/boards/shields/ergodox/src/menu/layer_inspector_data.h"

static uint8_t count_active_keys(uint8_t layer, bool is_left_half) {
    if (layer >= NUM_INSPECTED_LAYERS) {
        return 0;
    }
    uint8_t active = 0;
    uint8_t offset = is_left_half ? 0 : KEYS_PER_HALF;
    for (uint8_t i = 0; i < KEYS_PER_HALF; i++) {
        if (layer_keys_data[layer][offset + i].cat != KEY_CAT_TRANS) {
            active++;
        }
    }
    return active;
}

static bool is_partial_layer_test(uint8_t layer, bool is_left_half) {
    if (layer >= NUM_INSPECTED_LAYERS) {
        return false;
    }

    /* Full layout layers always render full 38-key zero-gap layout */
    if (layer == 0 /* WORK */ || layer == 1 /* SWAP */ || layer == 3 /* TYPE */ ||
        layer == 4 /* SYM1 */ || layer == 9 /* GSWP */ || layer == 13 /* SYM2 */ ||
        layer == 14 /* MENU */) {
        return false;
    }

    uint8_t active = count_active_keys(layer, is_left_half);
    if (active == 0) {
        return false;
    }

    /* Inspected half has <= 16 active keys, or designated partial functional layer */
    if (active <= 16) {
        return true;
    }

    if (layer == 2 /* LNPD */ || layer == 5 /* NRPD */ || layer == 6 /* MOUS */ ||
        layer == 7 /* ARRW */ || layer == 8 /* GAME */ || layer == 10 /* NPAD */ ||
        layer == 11 /* FUN1 */ || layer == 12 /* FUN2 */) {
        return true;
    }

    return false;
}

int main(void) {
    /* 1. Verify Full Layout Layers return false */
    assert(!is_partial_layer_test(0, true));   /* WORK Left */
    assert(!is_partial_layer_test(0, false));  /* WORK Right */
    assert(!is_partial_layer_test(1, true));   /* SWAP Left */
    assert(!is_partial_layer_test(1, false));  /* SWAP Right */
    assert(!is_partial_layer_test(3, true));   /* TYPE Left */
    assert(!is_partial_layer_test(3, false));  /* TYPE Right */
    assert(!is_partial_layer_test(4, true));   /* SYM1 Left */
    assert(!is_partial_layer_test(4, false));  /* SYM1 Right */
    assert(!is_partial_layer_test(13, true));  /* SYM2 Left */
    assert(!is_partial_layer_test(13, false)); /* SYM2 Right */
    assert(!is_partial_layer_test(14, true));  /* MENU Left (all none) */
    assert(!is_partial_layer_test(14, false)); /* MENU Right (all none) */

    /* 2. Verify Partial Layers return true on active halves */
    assert(is_partial_layer_test(5, true));    /* NRPD Left */
    assert(is_partial_layer_test(5, false));   /* NRPD Right (Numpad) */
    assert(is_partial_layer_test(6, true));    /* MOUS Left */
    assert(is_partial_layer_test(6, false));   /* MOUS Right */
    assert(is_partial_layer_test(7, true));    /* ARRW Left */
    assert(is_partial_layer_test(7, false));   /* ARRW Right */
    assert(is_partial_layer_test(8, true));    /* GAME Left (WASD) */
    assert(is_partial_layer_test(10, false));  /* NPAD Right (Numpad) */
    assert(!is_partial_layer_test(10, true));  /* NPAD Left (0 active keys) */
    assert(is_partial_layer_test(11, true));   /* FUN1 Left (F1-F12) */
    assert(is_partial_layer_test(11, false));  /* FUN1 Right */
    assert(is_partial_layer_test(12, true));   /* FUN2 Left (F13-F24) */
    assert(is_partial_layer_test(12, false));  /* FUN2 Right */

    /* 3. Verify Zoom Bounding Box and Geometry Calculation */
    for (uint8_t l = 0; l < NUM_INSPECTED_LAYERS; l++) {
        for (int h = 0; h < 2; h++) {
            bool is_left = (h == 0);
            if (!is_partial_layer_test(l, is_left)) {
                continue;
            }

            const struct key_geometry *base_geom = is_left ? left_half_geom : right_half_geom;
            uint8_t offset = is_left ? 0 : KEYS_PER_HALF;

            int min_x = 280, max_x = 0;
            int min_y = 240, max_y = 0;
            uint8_t active_count = 0;

            for (uint8_t i = 0; i < KEYS_PER_HALF; i++) {
                if (layer_keys_data[l][offset + i].cat != KEY_CAT_TRANS) {
                    active_count++;
                    const struct key_geometry *bg = &base_geom[i];
                    if (bg->x < min_x) min_x = bg->x;
                    if (bg->x + bg->w > max_x) max_x = bg->x + bg->w;
                    if (bg->y < min_y) min_y = bg->y;
                    if (bg->y + bg->h > max_y) max_y = bg->y + bg->h;
                }
            }

            assert(active_count > 0);
            int cluster_w = max_x - min_x;
            int cluster_h = max_y - min_y;

            int target_w = ZOOM_KEY_WIDTH;
            if ((cluster_w * target_w) / 29 > 268) {
                target_w = (268 * 29) / cluster_w;
            }
            if (target_w < 29) target_w = 29;

            int target_h = ZOOM_KEY_HEIGHT;
            if ((cluster_h * target_h) / 26 > 184) {
                target_h = (184 * 26) / cluster_h;
            }
            if (target_h < 26) target_h = 26;

            int scaled_cluster_w = (cluster_w * target_w) / 29;
            int scaled_cluster_h = (cluster_h * target_h) / 26;

            int origin_x = (280 - scaled_cluster_w) / 2;
            int origin_y = 28 + (184 - scaled_cluster_h) / 2;

            /* Verify origin fits on 280x240 screen within separator lines (y: 27..213) */
            assert(origin_x >= 0);
            assert(origin_x + scaled_cluster_w <= 280);
            assert(origin_y >= 26);
            assert(origin_y + scaled_cluster_h <= 214);

            /* Verify all active keys fit within bounds when scaled */
            for (uint8_t i = 0; i < KEYS_PER_HALF; i++) {
                if (layer_keys_data[l][offset + i].cat != KEY_CAT_TRANS) {
                    const struct key_geometry *bg = &base_geom[i];
                    int rel_x = bg->x - min_x;
                    int rel_y = bg->y - min_y;
                    int kx = origin_x + (rel_x * target_w) / 29;
                    int ky = origin_y + (rel_y * target_h) / 26;
                    int kw = (bg->w * target_w) / 29;
                    int kh = (bg->h * target_h) / 26;

                    assert(kx >= 0 && kx + kw <= 280);
                    assert(ky >= 26 && ky + kh <= 214);
                    /* Standard 1u keys scale up to 44x38 px */
                    if (bg->w == 29) {
                        assert(kw >= 29 && kw <= 44);
                    }
                    if (bg->h == 26) {
                        assert(kh >= 26 && kh <= 38);
                    }
                    assert(kw >= 28 && kw <= 44);
                    assert(kh >= 24 && kh <= 62);
                }
            }
        }
    }

    printf("PASS\n");
    return 0;
}
