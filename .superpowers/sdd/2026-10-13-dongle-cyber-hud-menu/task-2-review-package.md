# Review Package: Task 2

## Git Log
- `zmk-config`: `d21fd0e` (`feat(keymap): bind dongle menu toggle on MOUS layer`)
- Root: `7965b9b` (`docs: update layout viewer and 02_LAYERS.md for dongle menu key`)

## Diff in zmk-config
```diff
diff --git a/config/ergodox.keymap b/config/ergodox.keymap
index 68677c7..62d08a0 100644
--- a/config/ergodox.keymap
+++ b/config/ergodox.keymap
@@ -211,7 +211,7 @@
         layer_6 {
             display-name = "MOUS";
             bindings = <
-            &kp ESC         &trans          &trans          &trans          &trans          &trans          &trans            &trans          &trans          &trans          &trans          &trans          &trans          &kp BSPC        
+            &kp ESC         &kp F21         &trans          &trans          &trans          &trans          &trans            &trans          &trans          &trans          &trans          &trans          &trans          &kp BSPC        
```

## Diff in root (layout-viewer/data.js and docs/layout/02_LAYERS.md)
```diff
diff --git a/layout-viewer/data.js b/layout-viewer/data.js
@@ -6604,7 +6604,7 @@ window.KEYBOARD_DATA = {
-          "tapCode": "KC_TRANSPARENT",
+          "tapCode": "KC_F21",
           "holdCode": "",
-          "tapText": "▽",
-          "subText": "",
-          "customLabel": "▽",
-          "keyType": "transparent"
+          "tapText": "MENU",
+          "subText": "F21",
+          "customLabel": "MENU",
+          "keyType": "special"
```
```diff
diff --git a/docs/layout/02_LAYERS.md b/docs/layout/02_LAYERS.md
@@ -165,6 +165,9 @@
+- **Служебные клавиши**:
+  - `Key 0`: физический `Escape`.
+  - `Key 1`: переключатель Cyber HUD меню донгла (`MENU` / `F21`) — открытие и закрытие экранного OSD-меню настроек донгла.
```
