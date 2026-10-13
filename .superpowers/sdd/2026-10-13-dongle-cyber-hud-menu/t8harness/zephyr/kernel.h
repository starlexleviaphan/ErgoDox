#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
typedef long ssize_t_;
struct k_work { int pending; void (*h)(struct k_work *); };
struct k_work_q { int x; };
#define IS_ENABLED(x) (x)
#define K_WORK_DEFINE(n, fn) struct k_work n = { 0, fn }
static inline uint32_t k_uptime_get_32(void){ return 1000; }
static inline int k_work_submit_to_queue(struct k_work_q *q, struct k_work *w){ (void)q; w->pending++; return 1; }

