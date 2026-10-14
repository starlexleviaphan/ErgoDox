// tests/test_pictograms.c
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "../zmk-config/boards/shields/ergodox/src/menu/layer_inspector_data.h"

int main(void) {
    bool has_enter = false;
    bool has_backspace = false;
    bool has_delete = false;
    bool has_space = false;
    bool has_tab = false;
    bool has_esc = false;
    bool has_arrow_up = false;
    bool has_arrow_down = false;
    bool has_arrow_left = false;
    bool has_arrow_right = false;
    bool has_shift = false;
    bool has_ctrl = false;
    bool has_alt = false;
    bool has_gui = false;
    bool has_vol_up = false;
    bool has_vol_down = false;
    bool has_mute = false;
    bool has_play_pause = false;
    bool has_lclk = false;
    bool has_rclk = false;

    for (int l = 0; l < 15; l++) {
        for (int k = 0; k < 76; k++) {
            const char *label = layer_keys_data[l][k].label;
            assert(label != NULL);
            // Max label visual length must fit in 34px (<= 8 bytes in UTF-8)
            assert(strlen(label) <= 8);

            // Verify absence of legacy verbose labels
            assert(strcmp(label, "Ent") != 0);
            assert(strcmp(label, "Bsp") != 0);
            assert(strcmp(label, "Del") != 0);
            assert(strcmp(label, "Spc") != 0);
            assert(strcmp(label, "Tab") != 0);
            assert(strcmp(label, "Esc") != 0);
            assert(strcmp(label, "Up ") != 0);
            assert(strcmp(label, "Dow") != 0);
            assert(strcmp(label, "Sft") != 0);
            assert(strcmp(label, "Ctl") != 0);
            assert(strcmp(label, "Alt") != 0);
            assert(strcmp(label, "Gui") != 0);
            assert(strcmp(label, "Mut") != 0);

            // Track pictograms
            if (strcmp(label, "↵") == 0) has_enter = true;
            if (strcmp(label, "⌫") == 0) has_backspace = true;
            if (strcmp(label, "⌦") == 0) has_delete = true;
            if (strcmp(label, "␣") == 0) has_space = true;
            if (strcmp(label, "⇥") == 0) has_tab = true;
            if (strcmp(label, "ESC") == 0) has_esc = true;
            if (strcmp(label, "↑") == 0) has_arrow_up = true;
            if (strcmp(label, "↓") == 0) has_arrow_down = true;
            if (strcmp(label, "←") == 0) has_arrow_left = true;
            if (strcmp(label, "→") == 0) has_arrow_right = true;
            if (strcmp(label, "⇧") == 0) has_shift = true;
            if (strcmp(label, "^") == 0) has_ctrl = true;
            if (strcmp(label, "⌥") == 0) has_alt = true;
            if (strcmp(label, "❖") == 0) has_gui = true;
            if (strcmp(label, "Vol+") == 0) has_vol_up = true;
            if (strcmp(label, "Vol-") == 0) has_vol_down = true;
            if (strcmp(label, "✕") == 0) has_mute = true;
            if (strcmp(label, "▶‖") == 0) has_play_pause = true;
            if (strcmp(label, "L-CLK") == 0) has_lclk = true;
            if (strcmp(label, "R-CLK") == 0) has_rclk = true;
        }
    }

    assert(has_enter);
    assert(has_backspace);
    assert(has_delete);
    assert(has_space);
    assert(has_tab);
    assert(has_esc);
    assert(has_arrow_up);
    assert(has_arrow_down);
    assert(has_arrow_left);
    assert(has_arrow_right);
    assert(has_shift);
    assert(has_ctrl);
    assert(has_alt);
    assert(has_gui);
    assert(has_vol_up);
    assert(has_vol_down);
    assert(has_mute);
    assert(has_play_pause);
    assert(has_lclk);
    assert(has_rclk);

    printf("PASS\n");
    return 0;
}
