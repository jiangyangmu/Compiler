#include "ConstantContext.h"

namespace Language {

ConstantContext * CreateConstantContext()
{
    ConstantContext * constantContext = new ConstantContext();
    constantContext->nextStringLabel = 0;
    return constantContext;
}

int StringHash(StringRef string)
{
    static std::map<int, std::string> collideTest;

    int i = 15485863;
    int index = 1;
    for (char c : string)
    {
        i += 1610612741 * index * index * c;
        ++index;
    }

    auto it = collideTest.find(i);
    if (it != collideTest.end())
        ASSERT(it->second == string.toString());
    else
        collideTest.emplace_hint(it, i, string.toString());

    return i;
}

int FloatHash(float f)
{
    int * p = reinterpret_cast<int *>(&f);
    return *p;
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

static std::string FloatToHexString(float f)
{
    char hex[9];
    int * ip = reinterpret_cast<int *>(&f);
    sprintf_s(hex, 9, "%0.8x", *ip);
    return std::string(hex, hex + 8);
}

Location LocateFloat(ConstantContext * context,
                     float fltValue)
{
    Location loc;

    int floatHash = FloatHash(fltValue);

    ConstantContext::FloatConstant * floatConstant = &context->hashToFloatConstant[floatHash];

    if (floatConstant->label.empty())
    {
        std::string * label = new std::string("$flt_" + FloatToHexString(fltValue));

        floatConstant->label = StringRef(label->c_str(), label->length());
        floatConstant->value = fltValue;
    }

    loc.type = LocationType::LABEL;
    loc.labelValue = new StringRef(floatConstant->label);

    return loc;
}

}