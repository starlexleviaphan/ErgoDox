# Scoped Re-Review Package: Task 6 Fix Round 1

**Fix Base:** 898bdf3
**Fix Head:** 5c11b5f

## Commits
```
5c11b5f fix(menu): add toggle debounce, dispatch all LVGL operations to display queue, and correct enter key position
```

## Diff Summary
- `boards/shields/ergodox/src/menu/menu_controller.c`:
  - Added 200ms debounce in `menu_controller_toggle()`.
  - Routed all LVGL flag toggles and rendering through `menu_ui_work` submitted to `zmk_display_work_q()`.
  - Removed misidentified `POS_ENTER_L (14)`.
- `boards/shields/ergodox/src/menu/menu_controller.h`:
  - Added accessors `menu_controller_get_selected_item` and `menu_controller_set_selected_item`.
