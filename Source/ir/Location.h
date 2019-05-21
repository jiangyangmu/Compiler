#pragma once

#include "../util/Integer.h"
#include "../ir/DefinitionContext.h"

enum RegisterType
{
    RAX,
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
    NO_WHERE,           // rename: LOCATION_UNKNOWN
    REGISTER,           // rename: LOCATION_REGISTER
    ESP_OFFSET,         // rename: LOCATION_SP_OFFSET
    BP_OFFSET,          // rename: LOCATION_BP_OFFSET
    LABEL,              // rename: LOCATION_LABEL
    INLINE,             // rename: LOCATION_INLINE
    REGISTER_INDIRECT,  // rename: LOCATION_REGISTER_INDIRECT

    // help build
    SAME_AS_FIRST_CHILD,
    SAME_AS_FIRST_GRANDCHILD,
    SEARCH_LOCAL_DEFINITION_TABLE,
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
        StringRef * labelValue;
        Language::Definition * definitionValue;
    };
};

inline bool IsValidLocation(const Location & loc)
{
    return loc.type > NO_WHERE && loc.type <= REGISTER_INDIRECT;
}

inline bool IsMemoryLocation(const Location & loc)
{
    return loc.type == ESP_OFFSET || loc.type == BP_OFFSET || loc.type == REGISTER_INDIRECT;
}

inline bool IsXMMLocation(const Location & loc)
{
    return loc.type == REGISTER && (XMM0 <= loc.type && loc.type <= XMM3);
}
