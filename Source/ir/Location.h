#pragma once

#include "../util/Integer.h"

enum RegisterType
{
    RCX,
    RDX,
    R8,
    R9,
    R11,
    XMM0,
    XMM1,
    XMM2,
    XMM3,
};

enum LocationType
{
    NO_WHERE,
    REGISTER,
    ESP_OFFSET,
    LABEL,
    INLINE,
    RUNTIME_MEMORY,

    // help build
    SAME_AS_FIRST_CHILD,
    SAME_AS_FIRST_GRANDCHILD,
    NEED_ALLOC,
};

struct Location
{
    LocationType type;
    union
    {
        RegisterType registerType;
        size_t offsetValue;
        u64 inlineValue;
    };
};