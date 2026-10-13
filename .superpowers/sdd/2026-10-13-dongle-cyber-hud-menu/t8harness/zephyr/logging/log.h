#pragma once
#include <stdio.h>
#define LOG_MODULE_DECLARE(a, b)
#define LOG_WRN(...) do { printf("WRN: "); printf(__VA_ARGS__); printf("\n"); } while (0)
#define LOG_INF(...) do { } while (0)

