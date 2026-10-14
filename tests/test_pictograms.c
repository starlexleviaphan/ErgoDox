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
    bool has_wheel_up = false;
    bool has_mouse_up = false;

    for (int l = 0; l < 15; l++) {
        for (int k = 0; k < 76; k++) {
            const char *label = layer_keys_data[l][k].label;
            assert(label != NULL);
            // Max label visual length must fit in key cell (<= 5 chars)
            assert(strlen(label) <= 5);

            // Assert 100% pure ASCII (no unicode missing-glyph tofu [] boxes)
            for (int c = 0; label[c] != '\0'; c++) {
                unsigned char ch = (unsigned char)label[c];
                assert(ch >= 0x20 && ch <= 0x7E);
            }

            // Track standard ASCII compact symbols
            if (strcmp(label, "ENT") == 0) has_enter = true;
            if (strcmp(label, "BS") == 0) has_backspace = true;
            if (strcmp(label, "DEL") == 0) has_delete = true;
            if (strcmp(label, "SPC") == 0) has_space = true;
            if (strcmp(label, "TAB") == 0) has_tab = true;
            if (strcmp(label, "ESC") == 0) has_esc = true;
            if (strcmp(label, "UP") == 0) has_arrow_up = true;
            if (strcmp(label, "DN") == 0) has_arrow_down = true;
            if (strcmp(label, "LT") == 0) has_arrow_left = true;
            if (strcmp(label, "RT") == 0) has_arrow_right = true;
            if (strcmp(label, "SFT") == 0) has_shift = true;
            if (strcmp(label, "CTL") == 0) has_ctrl = true;
            if (strcmp(label, "ALT") == 0) has_alt = true;
            if (strcmp(label, "GUI") == 0) has_gui = true;
            if (strcmp(label, "V+") == 0) has_vol_up = true;
            if (strcmp(label, "V-") == 0) has_vol_down = true;
            if (strcmp(label, "MUTE") == 0) has_mute = true;
            if (strcmp(label, "PLAY") == 0) has_play_pause = true;
            if (strcmp(label, "LCLK") == 0) has_lclk = true;
            if (strcmp(label, "RCLK") == 0) has_rclk = true;
            if (strcmp(label, "WhUP") == 0) has_wheel_up = true;
            if (strcmp(label, "M-UP") == 0) has_mouse_up = true;
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
    assert(has_wheel_up);
    assert(has_mouse_up);

    printf("PASS\n");
    return 0;
}
