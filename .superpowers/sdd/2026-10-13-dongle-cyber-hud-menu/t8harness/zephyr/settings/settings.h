#pragma once
#include <stddef.h>
#include <sys/types.h>
typedef ssize_t (*settings_read_cb)(void *cb_arg, void *data, size_t len);
int settings_name_steq(const char *name, const char *key, const char **next);
int settings_save_one(const char *name, const void *value, size_t val_len);
extern int (*g_set)(const char *, size_t, settings_read_cb, void *);
#define SETTINGS_STATIC_HANDLER_DEFINE(a, b, c, set, d, e) int (*g_set)(const char *, size_t, settings_read_cb, void *) = set

