# Scoped Re-Review Package: Task 4 Fix Round 1

**Fix Base:** 034b4ef
**Fix Head:** b2c4225

## Commits
```
b2c4225 fix(widgets): use static work definitions and add bounds guards in bongo cat
```

## Diff
```diff
diff --git a/boards/shields/ergodox/src/widgets/bongo_cat.c b/boards/shields/ergodox/src/widgets/bongo_cat.c
index 8c728e2..782987a 100644
--- a/boards/shields/ergodox/src/widgets/bongo_cat.c
+++ b/boards/shields/ergodox/src/widgets/bongo_cat.c
@@ -51,9 +51,13 @@ static inline bool is_right_key(uint32_t pos) {
 static bool paw_l_down = false;
 static bool paw_r_down = false;
 static uint8_t bongo_wpm = 0;
 
-static struct k_work update_paws_work;
-static struct k_work_delayable lift_paw_l_work;
-static struct k_work_delayable lift_paw_r_work;
+static void bongo_cat_update_paws_handler(struct k_work *work);
+static void bongo_cat_lift_l_handler(struct k_work *work);
+static void bongo_cat_lift_r_handler(struct k_work *work);
+
+static K_WORK_DEFINE(update_paws_work, bongo_cat_update_paws_handler);
+static K_WORK_DELAYABLE_DEFINE(lift_paw_l_work, bongo_cat_lift_l_handler);
+static K_WORK_DELAYABLE_DEFINE(lift_paw_r_work, bongo_cat_lift_r_handler);
 
 static const char *get_cat_face(uint8_t wpm) {
@@ -177,7 +181,7 @@ ZMK_SUBSCRIPTION(widget_bongo_cat, zmk_wpm_state_changed);
 
 static int position_state_changed_listener(const zmk_event_t *eh) {
     const struct zmk_position_state_changed *ev = as_zmk_position_state_changed(eh);
-    if (!ev || !ev->state) {
+    if (!ev || !ev->state || ev->position >= 76 || sys_slist_is_empty(&bongo_cat_widgets)) {
         return 0;
     }
 
@@ -234,14 +238,6 @@ int zmk_widget_bongo_cat_init(struct zmk_widget_bongo_cat *widget, lv_obj_t *parent) {
     lv_obj_align(widget->head_label, LV_ALIGN_TOP_MID, 0, 4);
 
-    static bool work_inited = false;
-    if (!work_inited) {
-        k_work_init(&update_paws_work, bongo_cat_update_paws_handler);
-        k_work_init_delayable(&lift_paw_l_work, bongo_cat_lift_l_handler);
-        k_work_init_delayable(&lift_paw_r_work, bongo_cat_lift_r_handler);
-        work_inited = true;
-    }
-
     sys_slist_append(&bongo_cat_widgets, &widget->node);
 
     struct bongo_cat_state initial_state = bongo_cat_get_state(NULL);
```
