import re

def test_menu_controller_layer_14_usage():
    with open("zmk-config/boards/shields/ergodox/src/menu/menu_controller.c", "r") as f:
        src = f.read()
    assert "zmk_keymap_layer_activate(14" in src, "Menu controller must activate Layer 14 on menu open"
    assert "zmk_keymap_layer_deactivate(14" in src, "Menu controller must deactivate Layer 14 on menu exit"

def test_menu_controller_layer_14_suppression_logic():
    with open("zmk-config/boards/shields/ergodox/src/menu/menu_controller.c", "r") as f:
        src = f.read()

    # Extract menu_controller_set_state function body
    fn_match = re.search(r"void\s+menu_controller_set_state\s*\([^)]*\)\s*\{(.*?)\n\}", src, re.DOTALL)
    assert fn_match is not None, "menu_controller_set_state function not found"
    fn_body = fn_match.group(1)

    # Verify layer 14 is activated when entering SCREEN_STATE_MENU
    menu_state_match = re.search(r"case SCREEN_STATE_MENU:(.*?)(?=case SCREEN_STATE_LAYER_MAP:)", fn_body, re.DOTALL)
    assert menu_state_match is not None, "SCREEN_STATE_MENU block not found in set_state"
    menu_state_body = menu_state_match.group(1)
    assert "zmk_keymap_layer_activate(14" in menu_state_body, "SCREEN_STATE_MENU must activate Layer 14"
    assert "zmk_hid_keyboard_clear()" in menu_state_body, "SCREEN_STATE_MENU must clear HID keyboard"
    assert "zmk_endpoints_send_report(HID_USAGE_KEY)" in menu_state_body, "SCREEN_STATE_MENU must send HID report"

    # Verify layer 14 is activated when entering SCREEN_STATE_LAYER_MAP
    layer_map_match = re.search(r"case SCREEN_STATE_LAYER_MAP:(.*?)(?=\n\s*\})", fn_body, re.DOTALL)
    assert layer_map_match is not None, "SCREEN_STATE_LAYER_MAP block not found in set_state"
    layer_map_body = layer_map_match.group(1)
    assert "zmk_keymap_layer_activate(14" in layer_map_body, "SCREEN_STATE_LAYER_MAP must activate Layer 14"

    # Verify layer 14 is deactivated when exiting to SCREEN_STATE_DASHBOARD
    dash_state_match = re.search(r"case SCREEN_STATE_DASHBOARD:(.*?)(?=case SCREEN_STATE_MENU:)", fn_body, re.DOTALL)
    assert dash_state_match is not None, "SCREEN_STATE_DASHBOARD block not found in set_state"
    dash_state_body = dash_state_match.group(1)
    assert "zmk_keymap_layer_deactivate(14" in dash_state_body, "SCREEN_STATE_DASHBOARD must deactivate Layer 14"
    assert "zmk_keymap_layer_active(14)" in dash_state_body, "SCREEN_STATE_DASHBOARD must check if Layer 14 is active"
    assert "zmk_hid_keyboard_clear()" in dash_state_body, "SCREEN_STATE_DASHBOARD must clear HID keyboard"
    assert "zmk_endpoints_send_report(HID_USAGE_KEY)" in dash_state_body, "SCREEN_STATE_DASHBOARD must send HID report"
