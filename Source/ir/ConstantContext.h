#pragma once

#include "Location.h"
#include "../Base/String.h"

namespace Language {

struct ConstantContext
{
    struct StringConstant
    {
        StringRef label; // Label to pointer-to-string object. TODO: remove this hack once EXPR_PVAL works
        StringRef stringLabel;
        StringRef stringValue;
    };
    struct FloatConstant
    {
        StringRef label;
        float value;
    };

    int nextStringLabel;

    std::map<int, StringConstant> hashToStringConstant;
    std::map<int, FloatConstant> hashToFloatConstant;
};

ConstantContext *   CreateConstantContext();

void                LocateString(ConstantContext * context,
                                 StringRef strValue,
                                 Location * strLocation,
                                 Location * strPtrLocation);
Location            LocateFloat(ConstantContext * context, float fltValue);

}