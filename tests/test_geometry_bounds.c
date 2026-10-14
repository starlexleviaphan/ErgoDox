// tests/test_geometry_bounds.c
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "../zmk-config/boards/shields/ergodox/src/menu/layer_inspector_data.h"

static bool has_row_neighbor(int i) {
    if (i >= 0 && i < 6) return true;    /* Row 0: keys 0..5 have neighbor at i + 1 */
    if (i >= 7 && i < 13) return true;   /* Row 1: keys 7..12 have neighbor at i + 1 */
    if (i >= 14 && i < 20) return true;  /* Row 2: keys 14..19 have neighbor at i + 1 */
    if (i >= 21 && i < 26) return true;  /* Row 3: keys 21..25 have neighbor at i + 1 */
    if (i >= 27 && i < 31) return true;  /* Row 4: keys 27..30 have neighbor at i + 1 */
    return false;
}

int main(void) {
    for (int i = 0; i < 38; i++) {
        /* Must fit within 276px display width */
        assert(left_half_geom[i].x + left_half_geom[i].w <= 276);
        assert(right_half_geom[i].x + right_half_geom[i].w <= 276);

        /* Display bounds (vertical: between 30 and 214) */
        assert(left_half_geom[i].y >= 30 && left_half_geom[i].y + left_half_geom[i].h <= 214);
        assert(right_half_geom[i].y >= 30 && right_half_geom[i].y + right_half_geom[i].h <= 214);

        /* Main cluster keys (0..31): width must be 29 px, height 26 px */
        if (i < 32) {
            assert(left_half_geom[i].w == 29);
            assert(left_half_geom[i].h == 26);
            assert(right_half_geom[i].w == 29);
            assert(right_half_geom[i].h == 26);
        }

        /* Zero horizontal gap with row neighbor */
        if (has_row_neighbor(i)) {
            assert(left_half_geom[i].x + left_half_geom[i].w == left_half_geom[i + 1].x);
            assert(right_half_geom[i].x + right_half_geom[i].w == right_half_geom[i + 1].x);
        }
    }

    /* Left half main cluster starts at x=6, ends at 209 (6 + 7*29) */
    assert(left_half_geom[0].x == 6);
    assert(left_half_geom[6].x + left_half_geom[6].w == 209);

    /* Left half thumb cluster starts at x >= 216 (gap >= 7px from main cluster) */
    for (int i = 32; i < 38; i++) {
        assert(left_half_geom[i].x >= 216);
    }

    /* Right half thumb cluster ends at x <= 64 */
    for (int i = 32; i < 38; i++) {
        assert(right_half_geom[i].x + right_half_geom[i].w <= 64);
    }

    /* Right half main cluster starts at x=72 (gap = 8px from thumb cluster) */
    assert(right_half_geom[0].x == 72);
    assert(right_half_geom[6].x + right_half_geom[6].w == 275);

    printf("PASS\n");
    return 0;
}
