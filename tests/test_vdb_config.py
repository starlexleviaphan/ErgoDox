import re


def test_vdb_buffer_size():
    with open("zmk-config/boards/shields/ergodox/ergodox_dongle_display.conf", "r") as f:
        content = f.read()
    match = re.search(r"CONFIG_LV_Z_VDB_SIZE=(\d+)", content)
    assert match is not None, "CONFIG_LV_Z_VDB_SIZE missing"
    assert int(match.group(1)) == 25, f"Expected VDB size 25 (saves 34KB RAM), got {match.group(1)}"
