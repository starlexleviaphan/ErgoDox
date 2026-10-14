// tests/test_geometry_bounds.c
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "../zmk-config/boards/shields/ergodox/src/menu/layer_inspector_data.h"

int main(void) {
    for (int i = 0; i < 38; i++) {
        /* Must fit within 280px display width with >= 4px margins */
        assert(left_half_geom[i].x >= 4);
        assert(left_half_geom[i].x + left_half_geom[i].w <= 276);
        assert(right_half_geom[i].x >= 4);
        assert(right_half_geom[i].x + right_half_geom[i].w <= 276);

        /* Display bounds (vertical: between 30 and 200, above separator at 202) */
        assert(left_half_geom[i].y >= 30 && left_half_geom[i].y + left_half_geom[i].h <= 200);
        assert(right_half_geom[i].y >= 30 && right_half_geom[i].y + right_half_geom[i].h <= 200);

        /* Main cluster keys (0..31): width must be 30 px, height 32 px */
        if (i < 32) {
            assert(left_half_geom[i].w == 30);
            assert(left_half_geom[i].h == 32);
            assert(right_half_geom[i].w == 30);
            assert(right_half_geom[i].h == 32);
        }
    }

    /* Left Half Checks */
    /* Main cluster starts at x=4, ends at 214 (4 + 7*30) */
    assert(left_half_geom[0].x == 4);
    assert(left_half_geom[6].x + left_half_geom[6].w == 214);

    /* Left half Row 0 zero-gap */
    for (int i = 0; i < 6; i++) {
        assert(left_half_geom[i].x + left_half_geom[i].w == left_half_geom[i + 1].x);
    }
    /* Left half Row 1 zero-gap */
    for (int i = 7; i < 13; i++) {
        assert(left_half_geom[i].x + left_half_geom[i].w == left_half_geom[i + 1].x);
    }
    /* Left half Row 2 zero-gap */
    for (int i = 14; i < 20; i++) {
        assert(left_half_geom[i].x + left_half_geom[i].w == left_half_geom[i + 1].x);
    }
    /* Left half Row 3 zero-gap */
    for (int i = 21; i < 26; i++) {
        assert(left_half_geom[i].x + left_half_geom[i].w == left_half_geom[i + 1].x);
    }
    /* Left half Row 4 zero-gap */
    for (int i = 27; i < 31; i++) {
        assert(left_half_geom[i].x + left_half_geom[i].w == left_half_geom[i + 1].x);
    }

    /* Left half thumb cluster starts at x >= 216 (gap >= 2px from Row 0-2, >= 32px from Row 4) */
    for (int i = 32; i < 38; i++) {
        assert(left_half_geom[i].x >= 216);
    }

    /* Right Half Checks */
    /* Right half thumb cluster ends at x <= 64 */
    for (int i = 32; i < 38; i++) {
        assert(right_half_geom[i].x + right_half_geom[i].w <= 64);
    }

    /* Right half Row 0 starts at x=66, ends at 276 (66 + 7*30) */
    assert(right_half_geom[0].x == 66);
    assert(right_half_geom[6].x + right_half_geom[6].w == 276);
    for (int i = 0; i < 6; i++) {
        assert(right_half_geom[i].x + right_half_geom[i].w == right_half_geom[i + 1].x);
    }

    /* Right half Row 1 starts at x=66, ends at 276 */
    assert(right_half_geom[7].x == 66);
    assert(right_half_geom[13].x + right_half_geom[13].w == 276);
    for (int i = 7; i < 13; i++) {
        assert(right_half_geom[i].x + right_half_geom[i].w == right_half_geom[i + 1].x);
    }

    /* Right half Row 2: key [20] is End in Col 0 (x=66), keys [14..19] are H..') in Cols 1..6 (x=96..246) */
    assert(right_half_geom[20].x == 66);
    assert(right_half_geom[20].x + right_half_geom[20].w == right_half_geom[14].x);
    for (int i = 14; i < 19; i++) {
        assert(right_half_geom[i].x + right_half_geom[i].w == right_half_geom[i + 1].x);
    }
    assert(right_half_geom[19].x + right_half_geom[19].w == 276);

    /* Right half Row 3: keys [21..26] in Cols 1..6 (x=96..246) */
    assert(right_half_geom[21].x == 96);
    for (int i = 21; i < 26; i++) {
        assert(right_half_geom[i].x + right_half_geom[i].w == right_half_geom[i + 1].x);
    }
    assert(right_half_geom[26].x + right_half_geom[26].w == 276);

    /* Right half Row 4: keys [27..31] in Cols 1..5 (x=96..216) */
    assert(right_half_geom[27].x == 96);
    for (int i = 27; i < 31; i++) {
        assert(right_half_geom[i].x + right_half_geom[i].w == right_half_geom[i + 1].x);
    }
    assert(right_half_geom[31].x + right_half_geom[31].w == 246);

    printf("PASS\n");
    return 0;
}
