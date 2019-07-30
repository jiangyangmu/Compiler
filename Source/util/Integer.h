#pragma once

#include <cinttypes>

typedef uint64_t u64;
typedef uint32_t u32;

template <typename T>
T Max(T a, T b)
{
    return a >= b ? a : b;
}

template <typename T>
T Min(T a, T b)
{
    return a <= b ? a : b;
}

template <typename T>
T IntLog2(T i)
{
    // i > 0, i is power of 2
    T lg2 = 0;
    while ((i >>= 1) != 0)
        ++lg2;
    return lg2;
}

