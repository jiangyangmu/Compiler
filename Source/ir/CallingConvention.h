#pragma once

#include "../type/Type.h"
#include "Location.h"

namespace Language {

class ParameterPassingCallerProtocol
{
public:
    ParameterPassingCallerProtocol(FunctionType * functionType);

    Location GetParameterLocation(size_t index);

    bool IsParameterPassedByAddress(size_t index);

    bool IsReturnValueAddressAsFirstParameter();

private:
    std::vector<Location> parameterLocations;
    std::vector<bool> parameterPassedByAddress;
    bool rvaAsFirstParameter;
};

class ParameterPassingCalleeProtocol
{
public:
    ParameterPassingCalleeProtocol(FunctionType * functionType);

    Location GetParameterLocation(size_t index);

    bool IsParameterPassedByAddress(size_t index);

    bool IsReturnValueAddressAsFirstParameter();

private:
    std::vector<Location> parameterLocations;
    std::vector<bool> parameterPassedByAddress;
    bool rvaAsFirstParameter;
};

}