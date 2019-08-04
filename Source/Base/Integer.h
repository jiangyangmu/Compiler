#pragma once

#include <cinttypes>

typedef uint64_t u64;
typedef int64_t i64;
typedef uint32_t u32;
typedef u64 uptr;
typedef i64 iptr;

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

template <typename T>
T CeilPowOf2(T i)
{
    T po2 = 1;
    while (po2 < i)
        po2 <<= 1;
    return po2;
}

// round up/down to multiple of value
// round up/down to power of 2
