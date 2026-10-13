# Review Package: Task 1

**Base Commit:** 6b29332
**Head Commit:** a4a55b6

## Commit Log
```
a4a55b6 feat(dongle): enable 180 degree screen flip and windows system icon
```

## Stat Summary
```
boards/shields/ergodox/ergodox_dongle_display.conf | 6 +++++-
 1 file changed, 5 insertions(+), 1 deletion(-)
```

## Full Diff
```diff
diff --git a/boards/shields/ergodox/ergodox_dongle_display.conf b/boards/shields/ergodox/ergodox_dongle_display.conf
index bbf4f1a..6573aa3 100644
--- a/boards/shields/ergodox/ergodox_dongle_display.conf
+++ b/boards/shields/ergodox/ergodox_dongle_display.conf
@@ -38,6 +38,9 @@ CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_PROXY=y
 CONFIG_ZMK_BATTERY_REPORT_INTERVAL=60
 
+# HID Indicators (Caps Lock / Num Lock / Scroll Lock state reporting)
+CONFIG_ZMK_HID_INDICATORS=y
+
 # Windows GATT subscription fix
 CONFIG_BT_GATT_ENFORCE_SUBSCRIPTION=n
 CONFIG_BT_GAP_AUTO_UPDATE_CONN_PARAMS=y
@@ -61,7 +64,8 @@ CONFIG_PWM=y
 
 # YADS Widgets Configuration
 CONFIG_DONGLE_SCREEN_HORIZONTAL=y
-CONFIG_DONGLE_SCREEN_FLIPPED=n
+CONFIG_DONGLE_SCREEN_FLIPPED=y
+CONFIG_DONGLE_SCREEN_SYSTEM_ICON=2
 CONFIG_DONGLE_SCREEN_OUTPUT_ACTIVE=y
 CONFIG_DONGLE_SCREEN_LAYER_ACTIVE=y
 CONFIG_DONGLE_SCREEN_BATTERY_ACTIVE=y
```
