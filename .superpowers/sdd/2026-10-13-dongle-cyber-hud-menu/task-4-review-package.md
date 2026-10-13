# Review Package: Task 4

## Commits
```
034b4ef feat(widgets): add reactive bongo cat mascot and wpm speedometer gauge
```

## Summary
Created `bongo_cat.h`/`.c` and `wpm_gauge.h`/`.c` in `zmk-config/boards/shields/ergodox/src/widgets/`.
Updated `CMakeLists.txt` and `ergodox_dongle_display.conf` (`CONFIG_ZMK_WPM=y`).
Bongo Cat handles 76-key matrix partitioning (38 Left / 38 Right) with 150ms paw lift delays and 3-tier WPM expressions.
WPM Gauge renders arc meter with 4 color speed tiers (<45 Cyan, 45..80 Lime, 81..110 Orange, >110 Crimson).

## Files Touched
- `CMakeLists.txt`: Added `bongo_cat.c` and `wpm_gauge.c`.
- `ergodox_dongle_display.conf`: Added `CONFIG_ZMK_WPM=y`.
- `src/widgets/bongo_cat.h` & `bongo_cat.c`.
- `src/widgets/wpm_gauge.h` & `wpm_gauge.c`.
