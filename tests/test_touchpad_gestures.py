import os
import subprocess


def test_touchpad_gestures_c_unit_suite():
    exe_path = "tests/test_touchpad_gestures_runner.exe"
    try:
        compile_res = subprocess.run(
            [
                "gcc",
                "-Wall",
                "-Werror",
                "-DZMK_TOUCHPAD_UNIT_TEST",
                "tests/test_touchpad_gestures.c",
                "-o",
                exe_path,
            ],
            capture_output=True,
            text=True,
        )
        assert compile_res.returncode == 0, (
            f"GCC compilation failed:\nSTDOUT:\n{compile_res.stdout}\nSTDERR:\n{compile_res.stderr}"
        )

        run_res = subprocess.run(
            [os.path.abspath(exe_path)],
            capture_output=True,
            text=True,
        )
        assert run_res.returncode == 0, (
            f"Unit test runner failed:\nSTDOUT:\n{run_res.stdout}\nSTDERR:\n{run_res.stderr}"
        )
        assert "PASS" in run_res.stdout
    finally:
        if os.path.exists(exe_path):
            try:
                os.remove(exe_path)
            except OSError:
                pass
