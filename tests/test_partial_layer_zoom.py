# tests/test_partial_layer_zoom.py
import re
import subprocess
import os

def test_partial_layer_definitions():
    """Verify is_partial_layer helper and zoom constants are defined in source code."""
    with open("zmk-config/boards/shields/ergodox/src/menu/layer_inspector.c", "r", encoding="utf-8") as f:
        src = f.read()
    with open("zmk-config/boards/shields/ergodox/src/menu/layer_inspector.h", "r", encoding="utf-8") as f:
        header = f.read()

    assert "is_partial_layer" in src, "Must define is_partial_layer helper in layer_inspector.c"
    assert "bool is_partial_layer(uint8_t layer, bool is_left_half)" in header, "Must declare is_partial_layer in layer_inspector.h"
    assert "[ZOOM]" in src, "Must indicate zoom mode in the header banner"
    assert "LV_OBJ_FLAG_HIDDEN" in src, "Must hide/dim transparent cells in zoom mode"
    assert "ZOOM_KEY_WIDTH" in src or "44" in src, "Must define zoom key width (44px)"
    assert "ZOOM_KEY_HEIGHT" in src or "38" in src or "36" in src, "Must define zoom key height (36..38px)"

def test_layer_nav_comments_0_to_14():
    """Verify navigation handler comments are updated to 0..14."""
    with open("zmk-config/boards/shields/ergodox/src/menu/layer_inspector.c", "r", encoding="utf-8") as f:
        src = f.read()

    assert "Cycle layer up (0..14)" in src, "Must update UP nav comment to 0..14"
    assert "Cycle layer down (14..0)" in src, "Must update DOWN nav comment to 14..0"

def test_zoom_rendering_logic():
    """Verify zoom rendering handles partial layers, font selection, and centering."""
    with open("zmk-config/boards/shields/ergodox/src/menu/layer_inspector.c", "r", encoding="utf-8") as f:
        src = f.read()

    # Verify is_partial_layer is invoked in render
    assert "is_partial_layer(current_layer, current_is_left_half)" in src
    # Verify vertical pad calculation for zoomed keys
    assert "lv_obj_set_style_pad_top" in src
    # Verify unhidden flag on non-zoom / full layer rendering
    assert "lv_obj_clear_flag" in src

def test_partial_layer_c_harness():
    """Compile and run C verification test for partial layer detection and zoom scaling."""
    test_c_file = "tests/test_partial_zoom.c"
    test_bin = "tests/test_partial_zoom_runner.exe" if os.name == "nt" else "tests/test_partial_zoom_runner"

    try:
        res = subprocess.run(["gcc", "-Wall", "-Werror", test_c_file, "-o", test_bin], capture_output=True, text=True)
        assert res.returncode == 0, f"Compilation failed:\n{res.stderr}"

        run_res = subprocess.run([test_bin], capture_output=True, text=True)
        assert run_res.returncode == 0, f"Test execution failed:\n{run_res.stderr}\n{run_res.stdout}"
        assert "PASS" in run_res.stdout
    finally:
        if os.path.exists(test_bin):
            try:
                os.remove(test_bin)
            except OSError:
                pass
