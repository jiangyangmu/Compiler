#include "CallingConvention.h"
#include "../util/Bit.h"

namespace Language {

bool IsLargeOrIrregularType(Type * type)
{
    size_t size = TypeSize(type);
    return size > 8 || CountBits(size) != 1;
}

bool IsRAXType(Type * type)
{
    return IsIntegral(type) && !IsLargeOrIrregularType(type);
}

bool IsXMMType(Type * type)
{
    return IsFloating(type) && !IsLargeOrIrregularType(type);
}

bool IsStackOnlyType(Type * type)
{
    return IsLargeOrIrregularType(type);
}

ParameterPassingCallerProtocol::ParameterPassingCallerProtocol(FunctionType * functionType)
{
    // 1. maybe return value as first parameter
    // 2. parameters one by one in register/stack

    size_t rspOffset = 0;
    size_t nextIndex = 0;

    isVarList = functionType->isVarList;

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

            parameterPassedByAddress.push_back(false);
        }
        else if (!IsLargeOrIrregularType(parameterType) && nextIndex < 4)
        {
            loc.type = REGISTER;
            loc.offsetValue = (RegisterType)(RCX + nextIndex);

            parameterPassedByAddress.push_back(false);
        }
        else
        {
            loc.type = ESP_OFFSET;
            loc.offsetValue = rspOffset;

            parameterPassedByAddress.push_back(true);
        }

        rspOffset += 8;
        nextIndex += 1;

        parameterLocations.push_back(loc);
    }
}

Location GetDefaultParameterLocation(size_t index, Type * argumentType)
{
    Location loc;

    if (IsFloating(argumentType) && index < 4)
    {
        loc.type = REGISTER;
        loc.offsetValue = (RegisterType)(XMM0 + index);
    }
    else if (!IsLargeOrIrregularType(argumentType) && index < 4)
    {
        loc.type = REGISTER;
        loc.offsetValue = (RegisterType)(RCX + index);
    }
    else
    {
        loc.type = ESP_OFFSET;
        loc.offsetValue = index * 8;
    }

    return loc;
}

Location ParameterPassingCallerProtocol::GetParameterLocation(size_t index, Type * argumentType)
{
    if (index < parameterLocations.size())
    {
        return parameterLocations[index];
    }
    else
    {
        ASSERT(isVarList && argumentType);
        return GetDefaultParameterLocation(index, argumentType);
    }
}

bool ParameterPassingCallerProtocol::IsParameterPassedByAddress(size_t index, Type * argumentType)
{
    if (index < parameterLocations.size())
    {
        return parameterPassedByAddress[index];
    }
    else
    {
        ASSERT(isVarList && argumentType);
        return GetDefaultParameterLocation(index, argumentType).type == ESP_OFFSET;
    }
}

bool ParameterPassingCallerProtocol::IsParameterPassedByXMM(size_t index, Type * argumentType)
{
    Location loc;

    if (index < parameterLocations.size())
    {
        loc = parameterLocations[index];
    }
    else
    {
        ASSERT(isVarList && argumentType);
        loc = GetDefaultParameterLocation(index, argumentType);
    }

    return loc.type == REGISTER && (XMM0 <= loc.registerType && loc.registerType <= XMM3);
}

bool ParameterPassingCallerProtocol::IsVarArgument(size_t index)
{
    return isVarList && index >= parameterLocations.size();
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

            parameterPassedByAddress.push_back(false);
        }
        else if (!IsLargeOrIrregularType(parameterType) && nextIndex < 4)
        {
            loc.type = REGISTER;
            loc.offsetValue = (RegisterType)(RCX + nextIndex);

            parameterPassedByAddress.push_back(false);
        }
        else
        {
            loc.type = BP_OFFSET;
            loc.offsetValue = rbpOffset;

            parameterPassedByAddress.push_back(true);
        }

        rbpOffset += 8;
        nextIndex += 1;

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

bool ParameterPassingCalleeProtocol::IsParameterPassedByXMM(size_t index)
{
    ASSERT(index < parameterLocations.size());
    return parameterLocations[index].type == REGISTER &&
        (XMM0 <= parameterLocations[index].registerType && parameterLocations[index].registerType <= XMM3);
}

bool ParameterPassingCalleeProtocol::IsReturnValueAddressAsFirstParameter()
{
    return rvaAsFirstParameter;
}

size_t ParameterPassingCalleeProtocol::ParameterCount()
{
    return parameterLocations.size();
}

Location GetReturnValueLocation(Type * type)
{
    Location loc;

    if (IsLargeOrIrregularType(type))
    {
        loc.type = REGISTER_INDIRECT;
        loc.registerType = RAX;
    }
    else
    {
        loc.type = REGISTER;
        if (IsFloating(type))
            loc.registerType = XMM0;
        else
            loc.registerType = RAX;
    }

    return loc;
}

}