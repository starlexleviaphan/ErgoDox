#include <stdio.h>
#include <stdlib.h>
#define CONFIG_SETTINGS 1
#define CONFIG_ZMK_LOG_LEVEL 0
#include "../../../../zmk-config/boards/shields/ergodox/src/menu/menu_controller.c"

static struct k_work_q q;
const struct device g_disp_dev = {0};
static int orient_calls; static enum display_orientation last_orient; static int inval_calls;
int display_set_orientation(const struct device *d, enum display_orientation o){ (void)d; orient_calls++; last_orient = o; return 0; }
lv_obj_t *lv_scr_act(void){ static lv_obj_t o; return &o; }
void lv_obj_invalidate(lv_obj_t *o){ (void)o; inval_calls++; }
struct k_work_q *zmk_display_work_q(void){ return &q; }
lv_color_t lv_color_hex(uint32_t c){ lv_color_t x = { (int)c }; return x; }
lv_obj_t *lv_obj_create(lv_obj_t *p){ (void)p; return calloc(1, sizeof(lv_obj_t)); }
lv_obj_t *lv_label_create(lv_obj_t *p){ (void)p; return calloc(1, sizeof(lv_obj_t)); }
void lv_obj_set_size(lv_obj_t *o,int w,int h){(void)o;(void)w;(void)h;}
void lv_obj_set_pos(lv_obj_t *o,int x,int y){(void)o;(void)x;(void)y;}
void lv_obj_set_style_bg_color(lv_obj_t *o, lv_color_t c, int s){(void)o;(void)c;(void)s;}
void lv_obj_set_style_bg_opa(lv_obj_t *o,int v,int s){(void)o;(void)v;(void)s;}
void lv_obj_set_style_border_color(lv_obj_t *o, lv_color_t c, int s){(void)o;(void)c;(void)s;}
void lv_obj_set_style_border_width(lv_obj_t *o,int v,int s){(void)o;(void)v;(void)s;}
void lv_obj_set_style_radius(lv_obj_t *o,int v,int s){(void)o;(void)v;(void)s;}
void lv_obj_set_style_pad_all(lv_obj_t *o,int v,int s){(void)o;(void)v;(void)s;}
void lv_obj_set_style_text_color(lv_obj_t *o, lv_color_t c, int s){(void)o;(void)c;(void)s;}
void lv_obj_clear_flag(lv_obj_t *o,int f){(void)o;(void)f;}
void lv_obj_add_flag(lv_obj_t *o,int f){(void)o;(void)f;}
void lv_obj_align(lv_obj_t *o,int a,int x,int y){(void)o;(void)a;(void)x;(void)y;}
void lv_label_set_text(lv_obj_t *o,const char *t){(void)o;(void)t;}
void lv_label_set_text_fmt(lv_obj_t *o,const char *t,...){(void)o;(void)t;}
lv_obj_t *zmk_display_get_dashboard_root(void){ return NULL; }
int layer_inspector_show(uint8_t l, bool h){(void)l;(void)h;return 0;}
void layer_inspector_hide(void){}
void layer_inspector_handle_nav(enum menu_nav_action a){(void)a;}
bool layer_inspector_is_active(void){return false;}
void layer_inspector_render(void){}
int layer_inspector_init(lv_obj_t *s){(void)s;return 0;}
uint8_t zmk_keymap_highest_layer_active(void){return 0;}
void zmk_hid_keyboard_clear(void){}
const struct zmk_position_state_changed *as_zmk_position_state_changed(const void *e){(void)e;return NULL;}
const struct zmk_keycode_state_changed *as_zmk_keycode_state_changed(const void *e){(void)e;return NULL;}

int settings_name_steq(const char *name, const char *key, const char **next){
    size_t n = strlen(key);
    if (strncmp(name, key, n) != 0) return 0;
    if (name[n] == 0) { *next = NULL; return 1; }
    if (name[n] == 0x2f) { *next = name + n + 1; return 1; }
    return 0;
}
static int save_calls; static char saved[16][48];
int settings_save_one(const char *name, const void *v, size_t l){ (void)v; (void)l; strcpy(saved[save_calls++], name); return 0; }

static const uint8_t *g_buf; static size_t g_len;
static ssize_t rd(void *arg, void *data, size_t len){ (void)arg; if (len > g_len) len = g_len; memcpy(data, g_buf, len); return (ssize_t)len; }
static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)
static void load(const char *n, const void *b, size_t l){ g_buf = b; g_len = l; g_set(n, l, rd, NULL); }

int main(void){
    uint8_t u; uint16_t t;
    /* defaults */
    CHECK(current_brightness == 80 && get_sleep_timeout() == 600 && current_orientation_flipped && current_anim_idx == 0 && current_theme_idx == 0);
    /* valid loads */
    u = 40; load("brightness", &u, 1); CHECK(current_brightness == 40);
    t = 180; load("timeout_s", &t, 2); CHECK(get_sleep_timeout() == 180);
    u = 0; load("flipped", &u, 1); CHECK(!current_orientation_flipped);
    u = 2; load("anim_style", &u, 1); CHECK(current_anim_idx == 2);
    u = 3; load("theme_id", &u, 1); CHECK(current_theme_idx == 3);
    CHECK(settings_apply_work.pending == 5);
    /* orientation: loaded value only queued, applied on display queue */
    CHECK(orient_calls == 0);
    settings_apply_work.h(&settings_apply_work); CHECK(orient_calls == 1 && last_orient == DISPLAY_ORIENTATION_ROTATED_270 && inval_calls == 1);
    set_screen_orientation(true); CHECK(orient_calls == 1 && orientation_apply_work.pending == 1);
    orientation_apply_work.h(&orientation_apply_work); CHECK(orient_calls == 2 && last_orient == DISPLAY_ORIENTATION_ROTATED_90 && inval_calls == 2);
    set_screen_orientation(false); CHECK(orient_calls == 2);
    orientation_apply_work.h(&orientation_apply_work); CHECK(orient_calls == 3 && last_orient == DISPLAY_ORIENTATION_ROTATED_270);
    u = 0; load("flipped", &u, 1); CHECK(!current_orientation_flipped);
    /* out of range rejected */
    int p = settings_apply_work.pending;
    u = 5; load("brightness", &u, 1); CHECK(current_brightness == 40);
    u = 101; load("brightness", &u, 1); CHECK(current_brightness == 40);
    t = 123; load("timeout_s", &t, 2); CHECK(get_sleep_timeout() == 180);
    u = 2; load("flipped", &u, 1); CHECK(!current_orientation_flipped);
    u = 3; load("anim_style", &u, 1); CHECK(current_anim_idx == 2);
    u = 4; load("theme_id", &u, 1); CHECK(current_theme_idx == 3);
    CHECK(settings_apply_work.pending == p);
    /* wrong length rejected */
    uint8_t two[2] = {50, 0}; load("brightness", two, 2); CHECK(current_brightness == 40);
    u = 1; load("timeout_s", &u, 1); CHECK(get_sleep_timeout() == 180);
    load("theme_id", two, 2); CHECK(current_theme_idx == 3);
    CHECK(settings_apply_work.pending == p);
    /* no save on intermediate changes / exit */
    menu_controller_handle_nav(MENU_NAV_INC); menu_controller_set_state(SCREEN_STATE_MENU);
    CHECK(save_calls == 0);
    menu_controller_set_state(SCREEN_STATE_MENU);
    menu_controller_handle_nav(MENU_NAV_EXIT); CHECK(save_calls == 0);
    /* Save & Exit saves all 5 */
    menu_controller_set_selected_item(MENU_ITEM_SAVE_EXIT);
    menu_controller_set_state(SCREEN_STATE_MENU);
    last_nav_timestamp = 0; last_nav_action = (enum menu_nav_action)-1;
    menu_controller_handle_nav(MENU_NAV_SELECT);
    CHECK(save_calls == 5);
    CHECK(strcmp(saved[0], "ergodox_display/brightness") == 0 && strcmp(saved[4], "ergodox_display/theme_id") == 0);
    CHECK(menu_controller_get_state() == SCREEN_STATE_DASHBOARD);
    printf(fails ? "FAILED (%d)\n" : "ALL PASS\n", fails);
    return fails != 0;
}


