#pragma once

#include <stdio.h>

#define CHECK(p) do { \
    if (!(p)) { \
        fprintf(stderr, "Failed CHECK at %s:%d: %s\n", __FILE__, __LINE__, #p); \
        __builtin_trap(); \
    } \
} while (0)
