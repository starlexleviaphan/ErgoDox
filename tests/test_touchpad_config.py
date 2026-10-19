def test_touchpad_ble_buffers():
    for path in [
        "zmk-config/boards/shields/ergodox/ergodox_touchpad.conf",
        "zmk-config/boards/shields/ergodox/ergodox_dongle.conf",
        "zmk-config/boards/shields/ergodox/ergodox_dongle_display.conf",
    ]:
        with open(path, "r", encoding="utf-8") as f:
            conf = f.read()
        assert "CONFIG_BT_L2CAP_TX_BUF_COUNT=16" in conf, f"Missing TX_BUF_COUNT=16 in {path}"
        assert "CONFIG_BT_BUF_ACL_TX_COUNT=16" in conf, f"Missing ACL_TX_COUNT=16 in {path}"
        assert "CONFIG_BT_BUF_ACL_RX_COUNT=16" in conf, f"Missing ACL_RX_COUNT=16 in {path}"


def test_dongle_zip_button_behaviors():
    for path in [
        "zmk-config/boards/shields/ergodox/ergodox_dongle.overlay",
        "zmk-config/boards/shields/ergodox/ergodox_dongle_display.overlay",
    ]:
        with open(path, "r", encoding="utf-8") as f:
            overlay = f.read()
        assert "INPUT_BTN_MIDDLE" in overlay, f"Missing INPUT_BTN_MIDDLE in {path}"
        assert "INPUT_BTN_3" in overlay, f"Missing INPUT_BTN_3 in {path}"
        assert "INPUT_BTN_4" in overlay, f"Missing INPUT_BTN_4 in {path}"
        assert "&mkp MB4" in overlay, f"Missing MB4 binding in {path}"
        assert "&mkp MB5" in overlay, f"Missing MB5 binding in {path}"
        assert "INPUT_BTN_5" not in overlay, f"Found unexpected INPUT_BTN_5 in {path}"
        assert "INPUT_BTN_6" not in overlay, f"Found unexpected INPUT_BTN_6 in {path}"
        assert "&kp LC(EQUAL)" not in overlay, f"Found unexpected Zoom In binding in {path}"
        assert "&kp LC(MINUS)" not in overlay, f"Found unexpected Zoom Out binding in {path}"
        assert "&kp LG(" not in overlay, f"Found unexpected modifier binding in {path}"


def test_dongle_separate_mod_release():
    for path in [
        "zmk-config/boards/shields/ergodox/ergodox_dongle.conf",
        "zmk-config/boards/shields/ergodox/ergodox_dongle_display.conf",
    ]:
        with open(path, "r", encoding="utf-8") as f:
            conf = f.read()
        assert "CONFIG_ZMK_HID_SEPARATE_MOD_RELEASE_REPORT=y" in conf, f"Missing separate mod release in {path}"
