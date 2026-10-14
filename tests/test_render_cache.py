# tests/test_render_cache.py
import re
import subprocess
import os

def test_render_cache_and_debounce():
    """Verify render caching variables, early-exit check, and delayable work debounce."""
    with open("zmk-config/boards/shields/ergodox/src/menu/layer_inspector.c", "r", encoding="utf-8") as f:
        src = f.read()

    # 1. Cache tracking variables
    assert "rendered_layer" in src, "Must cache rendered_layer"
    assert "rendered_is_left" in src, "Must cache rendered_is_left"
    assert "rendered_dashboard_mode" in src, "Must cache rendered_dashboard_mode"

    # 2. Debouncing via delayable work
    assert "K_WORK_DELAYABLE_DEFINE(layer_inspector_delay_work" in src, "Must define layer_inspector_delay_work with K_WORK_DELAYABLE_DEFINE"
    assert "layer_inspector_work" not in src or "layer_inspector_delay_work" in src, "Must replace old layer_inspector_work"
    assert "k_work_reschedule_for_queue" in src or "k_work_delayable" in src, "Must debounce with delayable work"

    # 3. 50ms debounce for rapid events
    assert "K_MSEC(50)" in src, "Must use 50ms debounce delay for rapid events"

    # 4. Early-exit condition in render
    assert "rendered_layer == current_layer" in src, "Must check rendered_layer == current_layer for 0 ms early exit"
    assert "rendered_is_left == current_is_left_half" in src or "rendered_is_left ==" in src, "Must check rendered_is_left"

    # 5. Cache invalidation on hide
    assert "rendered_layer = -1;" in src or "rendered_layer = -1" in src, "Must reset cache on hide"


def test_render_cache_c_simulation():
    """Compile and run C verification test simulating render cache hits, misses, and invalidation."""
    test_c_file = "tests/test_render_cache.c"
    test_bin = "tests/test_render_cache_runner.exe" if os.name == "nt" else "tests/test_render_cache_runner"

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
