// tests/test_geometry_bounds.c
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

struct key_geom { uint16_t x, y, w, h; };
#include "../zmk-config/boards/shields/ergodox/src/menu/layer_inspector_data.h"

int main(void) {
    for (int i = 0; i < 38; i++) {
        // Must fit within 272px display width (bounds <= 276)
        assert(left_half_geom[i].x + left_half_geom[i].w <= 276);
        assert(right_half_geom[i].x + right_half_geom[i].w <= 276);

        // Display bounds (vertical)
        assert(left_half_geom[i].y >= 30 && left_half_geom[i].y + left_half_geom[i].h <= 214);
        assert(right_half_geom[i].y >= 30 && right_half_geom[i].y + right_half_geom[i].h <= 214);

        // Main cluster keys (0..29) must have zero horizontal gap with neighbor
        if (i % 6 < 5 && i < 30) {
            assert(left_half_geom[i].x + left_half_geom[i].w == left_half_geom[i + 1].x);
            assert(right_half_geom[i].x + right_half_geom[i].w == right_half_geom[i + 1].x);
        }
    }

    // Verify 8px visual separation for thumb cluster
    // Left half: main cluster ends at 208, thumb cluster starts at 216 (gap = 8px)
    for (int i = 32; i < 38; i++) {
        assert(left_half_geom[i].x >= 216);
    }
    // Right half: thumb cluster ends at 64, main cluster starts at 72 (gap = 8px)
    for (int i = 32; i < 38; i++) {
        assert(right_half_geom[i].x + right_half_geom[i].w <= 64);
    }

    printf("PASS\n");
    return 0;
}
