#include "ConstantContext.h"

namespace Language {

ConstantContext * CreateConstantContext()
{
    ConstantContext * constantContext = new ConstantContext();
    constantContext->nextStringLabel = 0;
    constantContext->nextFloatLabel = 0;
    return constantContext;
}

int StringHash(StringRef string)
{
    int i = 1610612741;
    for (char c : string)
    {
        i *= c;
    }
    return i;
}

void LocateString(ConstantContext * context,
                  StringRef stringValue,
                  Location * stringLocation,
                  Location * stringPtrLocation)
{
    int stringHash = StringHash(stringValue);

    ConstantContext::StringConstant * stringConstant = &context->hashToStringConstant[stringHash];

    if (stringConstant->label.empty())
    {
        std::string * label = new std::string("$sp" + std::to_string(context->nextStringLabel));
        std::string * stringLabel = new std::string("$str" + std::to_string(context->nextStringLabel));

        stringConstant->label = StringRef(label->c_str(), label->length());
        stringConstant->stringLabel = StringRef(stringLabel->c_str(), stringLabel->length());
        stringConstant->stringValue = stringValue;

        ++context->nextStringLabel;
    }

    stringLocation->type = LocationType::LABEL;
    stringLocation->labelValue = new StringRef(stringConstant->stringLabel);
    stringPtrLocation->type = LocationType::LABEL;
    stringPtrLocation->labelValue = new StringRef(stringConstant->label);
}
// TODO: impl
Location LocateFloat(ConstantContext * context,
                     float fltValue)
{
    ASSERT(false);
    Location loc;
    loc.type = NO_WHERE;
    return loc;
}

}