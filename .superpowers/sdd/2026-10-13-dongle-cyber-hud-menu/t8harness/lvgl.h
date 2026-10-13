#pragma once
#include <stdint.h>
typedef struct { int dummy; } lv_obj_t; typedef struct { int c; } lv_color_t;
#define LV_OPA_COVER 255
#define LV_OPA_TRANSP 0
#define LV_OBJ_FLAG_HIDDEN 1
#define LV_OBJ_FLAG_SCROLLABLE 2
#define LV_ALIGN_TOP_MID 0
#define LV_ALIGN_LEFT_MID 1
#define LV_ALIGN_RIGHT_MID 2
#define LV_ALIGN_BOTTOM_MID 3
lv_color_t lv_color_hex(uint32_t c);
lv_obj_t *lv_obj_create(lv_obj_t *p); lv_obj_t *lv_label_create(lv_obj_t *p);
void lv_obj_set_size(lv_obj_t *o,int w,int h); void lv_obj_set_pos(lv_obj_t *o,int x,int y);
void lv_obj_set_style_bg_color(lv_obj_t *o, lv_color_t c, int s); void lv_obj_set_style_bg_opa(lv_obj_t *o,int v,int s);
void lv_obj_set_style_border_color(lv_obj_t *o, lv_color_t c, int s); void lv_obj_set_style_border_width(lv_obj_t *o,int v,int s);
void lv_obj_set_style_radius(lv_obj_t *o,int v,int s); void lv_obj_set_style_pad_all(lv_obj_t *o,int v,int s);
void lv_obj_set_style_text_color(lv_obj_t *o, lv_color_t c, int s);
void lv_obj_clear_flag(lv_obj_t *o,int f); void lv_obj_add_flag(lv_obj_t *o,int f);
void lv_obj_align(lv_obj_t *o,int a,int x,int y);
void lv_label_set_text(lv_obj_t *o,const char *t); void lv_label_set_text_fmt(lv_obj_t *o,const char *t,...);

typedef struct { int z; } lv_obj_dummy_t; lv_obj_t *lv_scr_act(void); void lv_obj_invalidate(lv_obj_t *o);
