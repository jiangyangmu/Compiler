#pragma once

#include <bitset>

template <typename T>
size_t CountBits(T i)
{
    return std::bitset<sizeof(T)>(i).count();
}

// i != 0
template <typename T>
int FastLSBIndex(T i)
{
    int index = 0;
    while (i >>= 1)
        ++index;
    return index;
}
