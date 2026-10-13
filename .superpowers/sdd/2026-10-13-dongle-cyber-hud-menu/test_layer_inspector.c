/*
 * Unit Test Harness for ErgoDox Layer Inspector (Task 7)
 * Compiles under: gcc -Wall -Wextra -Werror -std=c99
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define EINVAL 22

/* LVGL Mock Definitions */
#define LV_PART_MAIN 0
#define LV_OPA_COVER 255
#define LV_OPA_TRANSP 0
#define LV_OBJ_FLAG_HIDDEN (1 << 0)
#define LV_OBJ_FLAG_SCROLLABLE (1 << 1)

#define LV_ALIGN_TOP_LEFT 1
#define LV_ALIGN_TOP_RIGHT 2
#define LV_ALIGN_BOTTOM_MID 3
#define LV_ALIGN_CENTER 4

typedef struct {
    uint32_t full;
} lv_color_t;

static inline lv_color_t lv_color_hex(uint32_t hex) {
    lv_color_t c;
    c.full = hex;
    return c;
}

typedef struct lv_obj {
    uint32_t flags;
    int16_t x, y, w, h;
    lv_color_t bg_color;
    lv_color_t border_color;
    lv_color_t text_color;
    uint8_t bg_opa;
    uint8_t border_width;
    uint8_t radius;
    uint8_t pad;
    char text[64];
    struct lv_obj *parent;
} lv_obj_t;

static lv_obj_t *lv_obj_create(lv_obj_t *parent) {
    lv_obj_t *obj = (lv_obj_t *)calloc(1, sizeof(lv_obj_t));
    obj->parent = parent;
    return obj;
}

static inline void lv_obj_set_size(lv_obj_t *obj, int16_t w, int16_t h) {
    if (obj) { obj->w = w; obj->h = h; }
}

static inline void lv_obj_set_pos(lv_obj_t *obj, int16_t x, int16_t y) {
    if (obj) { obj->x = x; obj->y = y; }
}

static inline void lv_obj_add_flag(lv_obj_t *obj, uint32_t f) {
    if (obj) { obj->flags |= f; }
}

static inline void lv_obj_clear_flag(lv_obj_t *obj, uint32_t f) {
    if (obj) { obj->flags &= ~f; }
}

static inline void lv_obj_set_style_bg_color(lv_obj_t *obj, lv_color_t c, int p) {
    (void)p; if (obj) obj->bg_color = c;
}

static inline void lv_obj_set_style_bg_opa(lv_obj_t *obj, uint8_t opa, int p) {
    (void)p; if (obj) obj->bg_opa = opa;
}

static inline void lv_obj_set_style_border_color(lv_obj_t *obj, lv_color_t c, int p) {
    (void)p; if (obj) obj->border_color = c;
}

static inline void lv_obj_set_style_border_width(lv_obj_t *obj, uint8_t bw, int p) {
    (void)p; if (obj) obj->border_width = bw;
}

static inline void lv_obj_set_style_radius(lv_obj_t *obj, uint8_t r, int p) {
    (void)p; if (obj) obj->radius = r;
}

static inline void lv_obj_set_style_pad_all(lv_obj_t *obj, uint8_t pad, int p) {
    (void)p; if (obj) obj->pad = pad;
}

static inline void lv_obj_set_style_text_color(lv_obj_t *obj, lv_color_t c, int p) {
    (void)p; if (obj) obj->text_color = c;
}

static inline lv_obj_t *lv_label_create(lv_obj_t *parent) {
    return lv_obj_create(parent);
}

static inline void lv_label_set_text(lv_obj_t *obj, const char *txt) {
    if (obj && txt) {
        strncpy(obj->text, txt, sizeof(obj->text) - 1);
        obj->text[sizeof(obj->text) - 1] = '\0';
    }
}

#include <stdarg.h>
static inline void lv_label_set_text_fmt(lv_obj_t *obj, const char *fmt, ...) {
    if (obj && fmt) {
        va_list args;
        va_start(args, fmt);
        vsnprintf(obj->text, sizeof(obj->text), fmt, args);
        va_end(args);
    }
}

static inline void lv_obj_align(lv_obj_t *obj, int align, int16_t x, int16_t y) {
    (void)align; if (obj) { obj->x = x; obj->y = y; }
}

static inline void lv_obj_center(lv_obj_t *obj) {
    if (obj) { obj->x = 0; obj->y = 0; }
}

/* Zephyr Mock Definitions */
struct k_work {};
typedef struct { void *unused; } sys_snode_t;
typedef struct { void *unused; } sys_slist_t;
#define SYS_SLIST_STATIC_INIT(x) { NULL }
#define LOG_MODULE_DECLARE(...)
#define CONFIG_ZMK_LOG_LEVEL 0

static uint32_t fake_uptime = 1000;
static inline uint32_t k_uptime_get_32(void) {
    return fake_uptime;
}

/* ZMK Mocks */
static uint8_t mock_highest_layer = 0;
uint8_t zmk_keymap_highest_layer_active(void) {
    return mock_highest_layer;
}

static int mock_keyboard_cleared = 0;
void zmk_hid_keyboard_clear(void) {
    mock_keyboard_cleared++;
}

static lv_obj_t *mock_dash_root = NULL;
lv_obj_t *zmk_display_get_dashboard_root(void) {
    return mock_dash_root;
}

static int mock_work_submitted = 0;
void *zmk_display_work_q(void) { return (void *)0x1234; }
int k_work_submit_to_queue(void *q, struct k_work *w) {
    (void)q; (void)w;
    mock_work_submitted++;
    return 0;
}

/* Layer Badge Metadata Mocks */
struct layer_meta {
    const char *name;
    uint32_t color_hex;
};

static const struct layer_meta mock_layer_meta[14] = {
    [0]  = { "WORK", 0x00d2ff },
    [1]  = { "SWAP", 0x38ef7d },
    [2]  = { "LNPD", 0xf9d423 },
    [3]  = { "TYPE", 0x00f2fe },
    [4]  = { "SYM1", 0xa18cd1 },
    [5]  = { "NRPD", 0xff9a44 },
    [6]  = { "MOUS", 0x4facfe },
    [7]  = { "ARRW", 0x43e97b },
    [8]  = { "GAME", 0xff0844 },
    [9]  = { "GSWP", 0xf857a6 },
    [10] = { "NPAD", 0xf6d365 },
    [11] = { "FUN1", 0xb721ff },
    [12] = { "FUN2", 0x6f86d6 },
    [13] = { "SYM2", 0xf355da },
};

uint32_t zmk_widget_layer_badge_get_color(uint8_t layer) {
    if (layer >= 14) layer = 0;
    return mock_layer_meta[layer].color_hex;
}

const char *zmk_widget_layer_badge_get_name(uint8_t layer) {
    if (layer >= 14) layer = 0;
    return mock_layer_meta[layer].name;
}

#include "zmk-config/boards/shields/ergodox/src/menu/menu_controller.h"

static enum screen_state mock_menu_state = SCREEN_STATE_LAYER_MAP;
void menu_controller_set_state(enum screen_state new_state) {
    mock_menu_state = new_state;
}
enum screen_state menu_controller_get_state(void) {
    return mock_menu_state;
}

/* Include Layer Inspector implementation */
#include "zmk-config/boards/shields/ergodox/src/menu/layer_inspector.h"
#include "zmk-config/boards/shields/ergodox/src/menu/layer_inspector_data.h"
#include "zmk-config/boards/shields/ergodox/src/menu/layer_inspector.c"

/* Test Counter */
static int test_passed = 0;
#define RUN_TEST(fn) do { \
    printf("Running %s...", #fn); \
    fn(); \
    printf(" PASSED\n"); \
    test_passed++; \
} while(0)

/* Unit Tests */

static void test_init_validation(void) {
    assert(layer_inspector_init(NULL) == -EINVAL);
    lv_obj_t *screen = lv_obj_create(NULL);
    assert(layer_inspector_init(screen) == 0);
    lv_obj_t *cnt = layer_inspector_get_container();
    assert(cnt != NULL);
    assert(cnt->w == 280);
    assert(cnt->h == 240);
    assert((cnt->flags & LV_OBJ_FLAG_HIDDEN) != 0);
    assert(layer_inspector_is_active() == false);
    assert(layer_inspector_get_current_layer() == 0);
    assert(layer_inspector_get_is_left_half() == true);
    free(screen);
    free(cnt);
}

static void test_show_and_hide(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    layer_inspector_init(screen);
    
    layer_inspector_show(4, false);
    assert(layer_inspector_is_active() == true);
    assert(layer_inspector_get_current_layer() == 4);
    assert(layer_inspector_get_is_left_half() == false);
    
    lv_obj_t *cnt = layer_inspector_get_container();
    assert((cnt->flags & LV_OBJ_FLAG_HIDDEN) == 0);
    
    /* Header title check */
    assert(header_title_label != NULL);
    assert(strcmp(header_title_label->text, "#4 SYM1") == 0);
    assert(header_title_label->text_color.full == 0xa18cd1);
    
    /* Active half banner check */
    assert(header_half_label != NULL);
    assert(strcmp(header_half_label->text, "< RIGHT HALF >") == 0);
    assert(header_half_label->text_color.full == 0xff9a44);
    
    layer_inspector_hide();
    assert(layer_inspector_is_active() == false);
    assert((cnt->flags & LV_OBJ_FLAG_HIDDEN) != 0);
    
    free(screen);
}

static void test_layer_clamping(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    layer_inspector_init(screen);
    
    layer_inspector_show(99, true);
    assert(layer_inspector_get_current_layer() == 0);
    assert(strcmp(header_title_label->text, "#0 WORK") == 0);
    assert(header_title_label->text_color.full == 0x00d2ff);
    
    free(screen);
}

static void test_left_right_half_toggle(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    layer_inspector_init(screen);
    layer_inspector_show(0, true);
    assert(layer_inspector_get_is_left_half() == true);
    assert(strcmp(header_half_label->text, "< LEFT HALF >") == 0);
    
    /* A key (MENU_NAV_DEC) toggles to Right half */
    layer_inspector_handle_nav(MENU_NAV_DEC);
    assert(layer_inspector_get_is_left_half() == false);
    assert(strcmp(header_half_label->text, "< RIGHT HALF >") == 0);
    
    /* D key (MENU_NAV_INC) toggles back to Left half */
    layer_inspector_handle_nav(MENU_NAV_INC);
    assert(layer_inspector_get_is_left_half() == true);
    assert(strcmp(header_half_label->text, "< LEFT HALF >") == 0);
    
    /* Select (MENU_NAV_SELECT) toggles half */
    layer_inspector_handle_nav(MENU_NAV_SELECT);
    assert(layer_inspector_get_is_left_half() == false);
    assert(strcmp(header_half_label->text, "< RIGHT HALF >") == 0);
    
    free(screen);
}

static void test_layer_cycling_up_down(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    layer_inspector_init(screen);
    layer_inspector_show(0, true);
    
    /* W key (MENU_NAV_UP): 0 -> 1 -> ... -> 13 -> 0 */
    layer_inspector_handle_nav(MENU_NAV_UP);
    assert(layer_inspector_get_current_layer() == 1);
    assert(strcmp(header_title_label->text, "#1 SWAP") == 0);
    
    for (int i = 2; i <= 13; i++) {
        layer_inspector_handle_nav(MENU_NAV_UP);
        assert(layer_inspector_get_current_layer() == (uint8_t)i);
    }
    
    /* Wrap 13 -> 0 */
    layer_inspector_handle_nav(MENU_NAV_UP);
    assert(layer_inspector_get_current_layer() == 0);
    assert(strcmp(header_title_label->text, "#0 WORK") == 0);
    
    /* S key (MENU_NAV_DOWN): wrap 0 -> 13 */
    layer_inspector_handle_nav(MENU_NAV_DOWN);
    assert(layer_inspector_get_current_layer() == 13);
    assert(strcmp(header_title_label->text, "#13 SYM2") == 0);
    
    layer_inspector_handle_nav(MENU_NAV_DOWN);
    assert(layer_inspector_get_current_layer() == 12);
    assert(strcmp(header_title_label->text, "#12 FUN2") == 0);
    
    free(screen);
}

static void test_esc_exit_behavior(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    layer_inspector_init(screen);
    layer_inspector_show(2, true);
    assert(layer_inspector_is_active() == true);
    
    /* ESC (MENU_NAV_EXIT) hides inspector and returns to menu */
    mock_menu_state = SCREEN_STATE_LAYER_MAP;
    layer_inspector_handle_nav(MENU_NAV_EXIT);
    assert(layer_inspector_is_active() == false);
    assert(mock_menu_state == SCREEN_STATE_MENU);
    lv_obj_t *cnt = layer_inspector_get_container();
    assert((cnt->flags & LV_OBJ_FLAG_HIDDEN) != 0);
    
    free(screen);
}

static void test_matrix_geometry_bounds(void) {
    /* Verify all 38 keys on Left half are within screen bounds (280x240) */
    for (int i = 0; i < 38; i++) {
        const struct key_geometry *g = &left_half_geom[i];
        assert(g->x >= 0);
        assert(g->y >= 30); /* Below header */
        assert(g->x + g->w <= 280);
        assert(g->y + g->h <= 216); /* Above footer */
        assert(g->w >= 20);
        assert(g->h >= 20);
    }
    
    /* Verify all 38 keys on Right half are within screen bounds (280x240) */
    for (int i = 0; i < 38; i++) {
        const struct key_geometry *g = &right_half_geom[i];
        assert(g->x >= 0);
        assert(g->y >= 30); /* Below header */
        assert(g->x + g->w <= 280);
        assert(g->y + g->h <= 216); /* Above footer */
        assert(g->w >= 20);
        assert(g->h >= 20);
    }
}

static void test_category_styling_and_labels(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    layer_inspector_init(screen);
    
    /* Test Layer 0 WORK (Left half) */
    layer_inspector_show(0, true);
    
    /* Key 1 (digit 1): Alpha category */
    assert(cell_objs[1]->bg_color.full == 0x0f172a);
    assert(cell_objs[1]->border_color.full == 0x00d2ff);
    assert(strcmp(cell_labels[1]->text, "1") == 0);
    
    /* Key 15 (A): Mod category */
    assert(cell_objs[15]->bg_color.full == 0x241432);
    assert(cell_objs[15]->border_color.full == 0xa18cd1);
    assert(strcmp(cell_labels[15]->text, "A") == 0);
    
    /* Key 32 (TG GAME): Layer Switch category */
    assert(cell_objs[32]->bg_color.full == 0x0e2a1b);
    assert(cell_objs[32]->border_color.full == 0x38ef7d);
    assert(strcmp(cell_labels[32]->text, "GAM") == 0);
    
    /* Test Layer 13 SYM2 (Left half): Key 2 (BT1) Media/BT category */
    layer_inspector_show(13, true);
    assert(cell_objs[2]->bg_color.full == 0x2d1428);
    assert(cell_objs[2]->border_color.full == 0xf355da);
    assert(strcmp(cell_labels[2]->text, "BT1") == 0);
    
    /* Test Layer 4 SYM1 (Left half): Key 4 (trans) Transparent category */
    layer_inspector_show(4, true);
    assert(cell_objs[4]->bg_color.full == 0x080b10);
    assert(cell_objs[4]->border_color.full == 0x1e293b);
    assert(strcmp(cell_labels[4]->text, "▽") == 0);
    
    free(screen);
}

int main(void) {
    printf("=====================================================\n");
    printf("🧪 ERGODOX 38-KEY LAYER INSPECTOR C99 UNIT TESTS\n");
    printf("=====================================================\n");
    
    RUN_TEST(test_init_validation);
    RUN_TEST(test_show_and_hide);
    RUN_TEST(test_layer_clamping);
    RUN_TEST(test_left_right_half_toggle);
    RUN_TEST(test_layer_cycling_up_down);
    RUN_TEST(test_esc_exit_behavior);
    RUN_TEST(test_matrix_geometry_bounds);
    RUN_TEST(test_category_styling_and_labels);
    
    printf("=====================================================\n");
    printf("All %d Layer Inspector Tests Passed Cleanly!\n", test_passed);
    printf("=====================================================\n");
    return 0;
}
