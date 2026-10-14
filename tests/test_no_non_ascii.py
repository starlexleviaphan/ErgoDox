import unicodedata

def test_no_non_ascii_in_layer_inspector_data():
    with open('zmk-config/boards/shields/ergodox/src/menu/layer_inspector_data.h', 'r', encoding='utf-8') as f:
        text = f.read()

    non_ascii = [c for c in text if ord(c) > 127]
    assert len(non_ascii) == 0, f"Found non-ASCII characters that will render as tofu [] boxes in LVGL: {set(non_ascii)}"
