# Review Package: Task 3

## Commits
```
892c373 feat(widgets): add lock status indicator and 14-layer color badge
```

## Summary
Created `lock_status.h`/`.c` and `layer_badge.h`/`.c` in `zmk-config/boards/shields/ergodox/src/widgets/` and updated `CMakeLists.txt` to register both.
Includes full 14-layer color lookup table and glowing lock status indicator.

## Files Touched
- `CMakeLists.txt`: Added include directories and sources for `lock_status.c` and `layer_badge.c`.
- `src/widgets/lock_status.h` & `lock_status.c`: Lock status widget.
- `src/widgets/layer_badge.h` & `layer_badge.c`: 14-layer badge widget.
