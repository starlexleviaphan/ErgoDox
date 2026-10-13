#pragma once
struct device { int x; };
extern const struct device g_disp_dev;
#define DT_CHOSEN(x) 0
#define DEVICE_DT_GET(x) (&g_disp_dev)
static inline int device_is_ready(const struct device *d){ (void)d; return 1; }

