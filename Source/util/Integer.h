#pragma once

#include <cinttypes>

typedef uint64_t u64;

template <typename T>
T Max(T a, T b)
{
    return a >= b ? a : b;
}
