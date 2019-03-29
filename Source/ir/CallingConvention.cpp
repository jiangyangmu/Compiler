#include "CallingConvention.h"
#include "../util/Bit.h"

namespace Language {

bool IsLargeOrIrregularType(Type * type)
{
    size_t size = TypeSize(type);
    return size > 8 || CountBits(size) != 1;
}


ParameterPassingCallerProtocol::ParameterPassingCallerProtocol(FunctionType * functionType)
{
    // 1. maybe return value as first parameter
    // 2. parameters one by one in register/stack

    size_t rspOffset = 0;
    size_t nextIndex = 0;

    Type * returnType = functionType->target;
    if (IsLargeOrIrregularType(returnType))
    {
        Location loc;
        loc.type = ESP_OFFSET;
        loc.offsetValue = rspOffset;
        parameterLocations.push_back(loc);

        rspOffset += 8;
        nextIndex += 1;

        parameterPassedByAddress.push_back(true);
        rvaAsFirstParameter = true;
    }
    else
    {
        rvaAsFirstParameter = false;
    }

    for (Type ** pp = functionType->memberType; *pp; ++pp)
    {
        Type * parameterType = *pp;

        Location loc;

        // float before 4                   -> by value (xmm)
        // small regular non-float before 4 -> by value (gr)
        // large/irregular non-float        -> by address (stack)
        if (IsFloating(parameterType) && nextIndex < 4)
        {
            loc.type = REGISTER;
            loc.offsetValue = (RegisterType)(XMM0 + nextIndex);

            nextIndex += 1;

            parameterPassedByAddress.push_back(false);
        }
        else if (!IsLargeOrIrregularType(parameterType) && nextIndex < 4)
        {
            loc.type = REGISTER;
            loc.offsetValue = (RegisterType)(RCX + nextIndex);

            nextIndex += 1;

            parameterPassedByAddress.push_back(false);
        }
        else
        {
            loc.type = ESP_OFFSET;
            loc.offsetValue = rspOffset;

            rspOffset += 8;
            nextIndex += 1;

            parameterPassedByAddress.push_back(true);
        }

        parameterLocations.push_back(loc);
    }
}

Location ParameterPassingCallerProtocol::GetParameterLocation(size_t index)
{
    ASSERT(index < parameterLocations.size());
    return parameterLocations[index];
}

bool ParameterPassingCallerProtocol::IsParameterPassedByAddress(size_t index)
{
    ASSERT(index < parameterLocations.size());
    return parameterPassedByAddress[index];
}

bool ParameterPassingCallerProtocol::IsReturnValueAddressAsFirstParameter()
{
    return rvaAsFirstParameter;
}

ParameterPassingCalleeProtocol::ParameterPassingCalleeProtocol(FunctionType * functionType)
{
    // 1. maybe return value as first parameter
    // 2. parameters one by one in register/stack

    size_t rbpOffset = 16; // return address (8 bytes) + rbp backup (8 bytes)
    size_t nextIndex = 0;

    Type * returnType = functionType->target;
    if (IsLargeOrIrregularType(returnType))
    {
        Location loc;
        loc.type = BP_OFFSET;
        loc.offsetValue = rbpOffset;
        parameterLocations.push_back(loc);
        
        rbpOffset += 8;
        nextIndex += 1;

        parameterPassedByAddress.push_back(true);
        rvaAsFirstParameter = true;
    }
    else
    {
        rvaAsFirstParameter = false;
    }

    for (Type ** pp = functionType->memberType; *pp; ++pp)
    {
        Type * parameterType = *pp;

        Location loc;

        // float before 4                   -> by value (xmm)
        // small regular non-float before 4 -> by value (gr)
        // large/irregular non-float        -> by address (stack)
        if (IsFloating(parameterType) && nextIndex < 4)
        {
            loc.type = REGISTER;
            loc.offsetValue = (RegisterType)(XMM0 + nextIndex);

            nextIndex += 1;

            parameterPassedByAddress.push_back(false);
        }
        else if (!IsLargeOrIrregularType(parameterType) && nextIndex < 4)
        {
            loc.type = REGISTER;
            loc.offsetValue = (RegisterType)(RCX + nextIndex);

            nextIndex += 1;

            parameterPassedByAddress.push_back(false);
        }
        else
        {
            loc.type = BP_OFFSET;
            loc.offsetValue = rbpOffset;

            rbpOffset += 8;
            nextIndex += 1;

            parameterPassedByAddress.push_back(true);
        }

        parameterLocations.push_back(loc);
    }
}

Location ParameterPassingCalleeProtocol::GetParameterLocation(size_t index)
{
    ASSERT(index < parameterLocations.size());
    return parameterLocations[index];
}


bool ParameterPassingCalleeProtocol::IsParameterPassedByAddress(size_t index)
{
    ASSERT(index < parameterLocations.size());
    return parameterPassedByAddress[index];
}

bool ParameterPassingCalleeProtocol::IsReturnValueAddressAsFirstParameter()
{
    return rvaAsFirstParameter;
}

}