#include "Translation.h"

#include "../ir/CallingConvention.h"
#include "../util/Bit.h"

namespace Language
{

// constant value -> x64 text

std::string SizeToX64TypeString(size_t size)
{
    std::string s;
    switch (size)
    {
        case 1: s = "BYTE"; break;
        case 2: s = "WORD"; break;
        case 4: s = "DWORD"; break;
        case 8: s = "QWORD"; break;
        default: ASSERT(false); break;
    }
    return s;
}

std::string ByteToHexString(unsigned char i)
{
    char hex[3];
    sprintf_s(hex, 3, "%0.2x", i);
    return std::string(hex, hex + 2);
}

std::string IntegerToHexString(size_t i)
{
    char hex[18];
    sprintf_s(hex, 18, "%0.16llxh", i);
    return std::string(hex, hex + 17);
}

std::string IntegerToHexString(int i)
{
    char hex[10];
    sprintf_s(hex, 10, "%0.8xh", i);
    return std::string(hex, hex + 9);
}

std::string IntegerToHexString(void * data, size_t size)
{
    int i;
    switch (size)
    {
        case 1: i = *static_cast<signed char *>(data);
        case 2: i = *static_cast<short *>(data);
        case 4: i = *static_cast<int *>(data);
        default: ASSERT(false); break;
    }
    return IntegerToHexString(i);
}

std::string ByteArrayToHexString(void * data, size_t size)
{
    std::string s;

    ASSERT(size > 0);

    unsigned char * p = static_cast<unsigned char *>(data);
    while (size > 0)
    {
        s += ByteToHexString(*p) +  + "H,";
        ++p;
        --size;
    }
    s.pop_back();

    return s;
}

std::string FloatToHexString(float f)
{
    char hex[9];
    int * ip = reinterpret_cast<int *>(&f);
    sprintf_s(hex, 9, "%0.8x", *ip);
    return std::string(hex, hex + 8);
}

// location string

std::string AX(size_t width)
{
    std::string s;
    switch (width)
    {
        case 1:  s = "al"; break;
        case 2:  s = "ax"; break;
        case 4:  s = "eax"; break;
        case 8:  s = "rax"; break;
        default: ASSERT(false); break;
    }
    return s;
}
std::string CX(size_t width)
{
    std::string s;
    switch (width)
    {
        case 1:  s = "cl"; break;
        case 2:  s = "cx"; break;
        case 4:  s = "ecx"; break;
        case 8:  s = "rcx"; break;
        default: ASSERT(false); break;
    }
    return s;
}
std::string DX(size_t width)
{
    std::string s;
    switch (width)
    {
        case 1:  s = "dl"; break;
        case 2:  s = "dx"; break;
        case 4:  s = "edx"; break;
        case 8:  s = "rdx"; break;
        default: ASSERT(false); break;
    }
    return s;
}
std::string SI(size_t width)
{
    std::string s;
    switch (width)
    {
        case 2:  s = "si"; break;
        case 4:  s = "esi"; break;
        case 8:  s = "rsi"; break;
        default: ASSERT(false); break;
    }
    return s;
}
std::string DI(size_t width)
{
    std::string s;
    switch (width)
    {
        case 2:  s = "di"; break;
        case 4:  s = "edi"; break;
        case 8:  s = "rdi"; break;
        default: ASSERT(false); break;
    }
    return s;
}
std::string XMM(size_t index)
{
    ASSERT(index < 8);
    return "XMM" + std::to_string(index);
}

std::string RegisterTypeToString(RegisterType type, size_t width)
{
    std::string s;
    switch (type)
    {
        case RAX:                       s = AX(width); break;
        case RCX:                       s = CX(width); break;
        case RDX:                       s = DX(width); break;
        case R8:    ASSERT(width == 8); s = "r8"; break;
        case R9:    ASSERT(width == 8); s = "r9"; break;
        case R11:   ASSERT(width == 8); s = "r11"; break;
        case XMM0:                      s = XMM(0); break;
        case XMM1:                      s = XMM(1); break;
        case XMM2:                      s = XMM(2); break;
        case XMM3:                      s = XMM(3); break;
        default:    ASSERT(false); break;
    }
    return s;
}

std::string X64LocationString(Location loc, size_t width)
{
    ASSERT(loc.type >= REGISTER && loc.type <= INLINE);
    std::string s;
    switch (loc.type)
    {
        case REGISTER:      s = RegisterTypeToString(loc.registerType, width); break;
        case ESP_OFFSET:    s = SizeToX64TypeString(width) + " PTR [rsp + " + std::to_string(loc.offsetValue) + "]"; break;
        case BP_OFFSET:     s = SizeToX64TypeString(width) + " PTR [rbp + " + std::to_string(loc.offsetValue) + "]"; break;
        case LABEL:         s = SizeToX64TypeString(width) + " PTR " + loc.labelValue->toString(); break;
        case INLINE:        s = IntegerToHexString(loc.inlineValue); break;
        default:            break;
    }
    return s;
}

// register mask (only non-volatile registers)

void AssertRegisterMask(FunctionContext * context, size_t mask)
{
    ASSERT(CountBits(mask) == 1);
    ASSERT((context->dirtyRegisters & mask) != 0);
}

// program API

void AddExtern(x64Program * program,
               StringRef label)
{
    program->externLabels.push_back(label.toString());
}

void AddPublic(x64Program * program,
               StringRef label)
{
    program->publicLabels.push_back(label.toString());
}

void AddToBSSSegment(x64Program * program,
                     StringRef label,
                     size_t size,
                     size_t align,
                     bool isPublic)
{
    ASSERT(size > 0 && align > 0 && size % align == 0);

    if (isPublic)
    {
        program->publicLabels.push_back(label.toString());
    }

    // <label> <align-string> 0<size/align>H DUP
    program->bssSegment +=
        label.toString() + " " + SizeToX64TypeString(align) + " 0" + IntegerToHexString(size / align) + "H DUP\n";
}

void AddToDATASegment(x64Program * program,
                      StringRef label,
                      size_t size,
                      size_t align,
                      bool isPublic,
                      void * value)
{
    ASSERT(size > 0 && align > 0 && size % align == 0);

    if (isPublic)
    {
        program->publicLabels.push_back(label.toString());
    }

    // 1,2,4,8 byte: <label> <size-string> 0<value>H
    // other   byte: <label> BYTE <b0>H,<b1>H,<b2>H,...
    switch (size)
    {
        case 1:
        case 2:
        case 4:
        case 8:
            program->dataSegment +=
                label.toString() + " " + SizeToX64TypeString(size) + " 0" + IntegerToHexString(value, size) + "H\n";;
            break;
        default:
            program->dataSegment +=
                label.toString() + " BYTE " + ByteArrayToHexString(value, size) + "\n";;
            break;
    }
}

void AddObjectDefinition(x64Program * program,
                         ObjectDefinition * objectDefinition)
{
    if (objectDefinition->objStorageType == ObjectStorageType::IMPORT_OBJECT)
    {
        AddExtern(program, objectDefinition->def.name);
    }
    else if (objectDefinition->objStorageType == ObjectStorageType::GLOBAL_OBJECT ||
             objectDefinition->objStorageType == ObjectStorageType::GLOBAL_EXPORT_OBJECT ||
             objectDefinition->objStorageType == ObjectStorageType::FUNCTION_STATIC_OBJECT)
    {
        StringRef   label       = objectDefinition->def.name;
        size_t      size        = TypeSize(objectDefinition->objType);
        size_t      align       = TypeAlignment(objectDefinition->objType);
        bool        isPublic    = objectDefinition->objStorageType == ObjectStorageType::GLOBAL_EXPORT_OBJECT;
        void *      value       = objectDefinition->objValue;
        if (value)
        {
            AddToDATASegment(program, label, size, align, isPublic, value);
        }
        else
        {
            AddToBSSSegment(program, label, size, align, isPublic);
        }
    }
}

// Only handle function name
void AddFunctionDefinition(x64Program * program,
                           FunctionDefinition * functionDefinition)
{
    if (functionDefinition->funcStorageType == FunctionStorageType::GLOBAL_EXPORT_FUNCTION)
    {
        AddPublic(program, functionDefinition->def.name);
    }
    else if (functionDefinition->funcStorageType == FunctionStorageType::IMPORT_FUNCTION)
    {
        std::string s = functionDefinition->def.name.toString() + ":PROC";
        AddExtern(program, StringRef(s.c_str(), s.length()));
    }
}

// translation functions

void TranslateDefinitionContext(x64Program * program,
                                DefinitionContext * context)
{
    for (Definition * definition : context->definitions[ID_NAMESPACE])
    {
        if (definition->type == DefinitionType::OBJECT_DEFINITION)
        {
            AddObjectDefinition(program, AsObjectDefinition(definition));
        }
        else if (definition->type == DefinitionType::FUNCTION_DEFINITION)
        {
            AddFunctionDefinition(program, AsFunctionDefinition(definition));
        }
    }

    if (context->firstChild)
        TranslateDefinitionContext(program, context->firstChild);
    if (context->next)
        TranslateDefinitionContext(program, context->next);
}

void TranslateConstantContext(x64Program * program,
                              ConstantContext * context)
{
    // string constant  -> <label> DB <string>,00H
    for (auto kv : context->hashToStringConstant)
    {
        const StringRef & label = kv.second.label;
        const StringRef & stringLabel = kv.second.stringLabel;
        const StringRef & stringValue = kv.second.stringValue;
        program->constSegment += stringLabel.toString() + " DB '" + stringValue.toString() + "',00H\n";
        program->dataSegment += label.toString() + " DQ " + stringLabel.toString() + "\n";
    }

    // float constant   -> <label> DD 0<hex>r ; <value>
    for (auto kv : context->hashToFloatConstant)
    {
        const StringRef & label = kv.second.label;
        const float &     value = kv.second.value;
        program->constSegment += label.toString() + " DD 0" + FloatToHexString(value) + "r ; " + std::to_string(value) + "\n";
    }
}

std::string TranslateCallExpression(Type * pointerToFuncType,
                                    Location funcLoc,
                                    std::vector<Type*> paramTypes,
                                    std::vector<Location> paramLocations,
                                    Type * outType,
                                    Location outLoc)
{
    // Assume function and arguments are matched, and argument conversion already applied.

    // 1. maybe copy return value as first parameter
    // 2. copy param to reg/stack
    // 3. maybe use register call, issue call
    // 4. copy return value to outLoc

    std::string s;

    ASSERT(IsPointerToFunction(pointerToFuncType));

    FunctionType * functionType = AsFunction(AsPointer(pointerToFuncType)->target);

    ParameterPassingCallerProtocol callerProtocol(functionType);
    size_t argumentIndex = 0;

    if (callerProtocol.IsReturnValueAddressAsFirstParameter())
    {
        size_t width = 8;

        // lea rax, outLoc
        // mov ..., rax
        s += "lea " + AX(width) + ", " + X64LocationString(outLoc, width) + "\n" +
             "mov " + X64LocationString(callerProtocol.GetParameterLocation(argumentIndex), width) + ", " + AX(width) + "\n";

        argumentIndex += 1;
    }

    for (size_t i = 0; i < paramTypes.size(); ++i)
    {
        Location &  paramLoc = paramLocations[i];
        Type *      paramType = paramTypes[i];
        size_t      paramSize = TypeSize(paramType);

        if (callerProtocol.IsParameterPassedByAddress(argumentIndex))
        {
            size_t width = 8;

            // lea rax, paramLoc
            // mov ..., rax
            s += "lea " + AX(width) + ", " + X64LocationString(paramLoc, width) + "\n" +
                 "mov " + X64LocationString(callerProtocol.GetParameterLocation(argumentIndex), width) + ", " + AX(width) + "\n";
        }
        else
        {
            size_t width = paramSize;

            // mov rax, paramLoc
            // mov ..., rax
            s += "mov " + AX(width) + ", " + X64LocationString(paramLoc, width) + "\n" +
                 "mov " + X64LocationString(callerProtocol.GetParameterLocation(argumentIndex), width) + ", " + AX(width) + "\n";
        }

        argumentIndex += 1;
    }

    if (funcLoc.type == LocationType::LABEL)
    {
        // call label
        s += "call " + funcLoc.labelValue->toString() + "\n";
    }
    else
    {
        // mov r11, inLoc
        // call r11
        s += "mov r11, " + X64LocationString(funcLoc, 8) + "\n";
        s += std::string("call ") + "r11" + "\n";
    }

    size_t outSize = TypeSize(outType);
    if (IsIntegral(outType))
    {
        // mov outLoc, rax
        s += "mov " + X64LocationString(outLoc, outSize) + ", " + AX(outSize) + "\n";
    }
    else if (IsFloating(outType))
    {
        // movss outLoc, xmm0
        if (outSize == 4)
            s += "movss " + X64LocationString(outLoc, outSize) + ", " + XMM(0) + "\n";
        else if (outSize == 8)
            s += "movsd " + X64LocationString(outLoc, outSize) + ", " + XMM(0) + "\n";
        else
            ASSERT(false);
    }
    else
    {
        ASSERT(false);
    }

    return s;
}

// TODO: shortcut effect
// TODO: implement register indirection
std::string TranslateExpression(FunctionContext * context,
                                Node * expression)
{
    // Input: expression tree (all UAC/promotions/positive already convert to cast nodes)
    // Output: code string
    // side effect: use stack/reg to compute value, store in stack/reg

    ASSERT(expression && expression->type > BEGIN_EXPRESSION && expression->type < END_EXPRESSION);
    std::string s;

    ExpressionIntention intent = context->currentIntention.back();
    Type *      outType = expression->expr.type;
    size_t      outSize = TypeSize(outType);
    Location    outLoc = expression->expr.loc;

    // Each expression code block should
    // 1. visit children (eval order, shortcut effect)
    // 2. get and check children loc/type/size
    // 3. generate code
    // 4. mask non-volatile registers

    if (expression->type == EXPR_ID)
    {
        // do nothing
        ASSERT(IsValidLocation(expression->expr.loc));
    }
    else if (expression->type == EXPR_CONSTANT)
    {
        // do nothing
    }
    else if (expression->type == EXPR_CALL)
    {
        // visit children
        for (Node * child = expression->down;
                child;
                child = child->right)
        {
            s += TranslateExpression(context, child);
        }

        Type * functionType = expression->down->expr.type;
        Location functionLoc = expression->down->expr.loc;
        std::vector<Type *> argumentTypes;
        std::vector<Location> argumentLocations;

        for (Node * argumentNode = expression->down->right;
             argumentNode;
             argumentNode = argumentNode->right)
        {
            argumentTypes.push_back(argumentNode->expr.type);
            argumentLocations.push_back(argumentNode->expr.loc);
        }

        s += TranslateCallExpression(functionType,
                                     functionLoc,
                                     argumentTypes,
                                     argumentLocations,
                                     outType,
                                     outLoc);
    }
    else if (expression->type == EXPR_CVT_NOOP)
    {
        // visit children
        s += TranslateExpression(context, expression->down);

        // do nothing
    }
    else if (expression->type == EXPR_CVT_I2I)
    {
        // visit children
        s += TranslateExpression(context, expression->down);

        ASSERT(IsInt(outType));

        Location    inLoc = expression->down->expr.loc;
        Type *      inType = expression->down->expr.type;
        size_t      inSize = TypeSize(inType);

        ASSERT(IsInt(inType));
        ASSERT(outSize != inSize);

        // large from small -> movsx
        // small from large -> mov
        // equal            -> mov

        if (outSize > inSize)
        {
            // mov ax, inLoc
            // movsx rax, ax
            // mov outLoc, rax
            s += "mov " + AX(inSize) + ", " + X64LocationString(inLoc, inSize) + "\n" +
                 "movsx " + AX(outSize) + ", " + AX(inSize) + "\n" +
                 "mov " + X64LocationString(outLoc, outSize) + ", " + AX(outSize) + "\n";
        }
        else
        {
            // mov rax, inLoc
            // mov outLoc, ax
            s += "mov " + AX(inSize) + ", " + X64LocationString(inLoc, inSize) + "\n" +
                 "mov " + X64LocationString(outLoc, outSize) + ", " + AX(outSize) + "\n";
        }
    }
    else if (expression->type == EXPR_CVT_F2I)
    {
        // visit children
        s += TranslateExpression(context, expression->down);

        ASSERT(IsInt(outType));

        Location    inLoc = expression->down->expr.loc;
        Type *      inType = expression->down->expr.type;
        size_t      inSize = TypeSize(inType);

        ASSERT(IsFloating(inType));

        // float-4  -> int-*    = cvtss2si + cvt_i2i
        // float-8  -> int-*    = cvtsd2si + cvt_i2i
        // float-10 -> int-*    = error

        if (inSize == 4)
        {
            // movss xmm0, inLoc
            // cvtss2si eax, xmm0
            // cvt_i2i outLoc, eax
            s += "movss " + XMM(0) + ", " + X64LocationString(inLoc, inSize) + "\n" +
                 "cvtss2si " + AX(inSize) + ", " + XMM(0) + "\n";
            {
                if (outSize > inSize)
                {
                    s += "movsx " + AX(outSize) + ", " + AX(inSize) + "\n";
                }
                s += "mov " + X64LocationString(outLoc, outSize) + ", " + AX(outSize) + "\n";
            }
        }
        else if (inSize == 8)
        {
            // movsd xmm0, inLoc
            // cvtsd2si rax, xmm0
            // cvt_i2i outLoc, rax
            s += "movsd " + XMM(0) + ", " + X64LocationString(inLoc, inSize) + "\n" +
                "cvtsd2si " + AX(inSize) + ", " + XMM(0) + "\n";
            {
                ASSERT(outSize <= inSize);
                s += "mov " + X64LocationString(outLoc, outSize) + ", " + AX(outSize) + "\n";
            }
        }
        else
        {
            // SSE don't support long double
            ASSERT(false);
        }
    }
    else if (expression->type == EXPR_CVT_B2I)
    {
        // visit children
        s += TranslateExpression(context, expression->down);

        ASSERT(IsInt(outType));

        Location    inLoc = expression->down->expr.loc;
        Type *      inType = expression->down->expr.type;
        size_t      inSize = TypeSize(inType);

        ASSERT(IsBool(inType));
        ASSERT(outSize >= inSize);

        // large from small -> movsx
        // equal            -> mov

        if (outSize > inSize)
        {
            // mov ax, inLoc
            // movsx rax, ax
            // mov outLoc, rax
            s += "mov " + AX(inSize) + ", " + X64LocationString(inLoc, inSize) + "\n" +
                "movsx " + AX(outSize) + ", " + AX(inSize) + "\n" +
                "mov " + X64LocationString(outLoc, outSize) + ", " + AX(outSize) + "\n";
        }
        else
        {
            // mov rax, inLoc
            // mov outLoc, rax
            s += "mov " + AX(inSize) + ", " + X64LocationString(inLoc, inSize) + "\n" +
                "mov " + X64LocationString(outLoc, outSize) + ", " + AX(outSize) + "\n";
        }
    }
    else if (expression->type == EXPR_CVT_F2F)
    {
        // visit children
        s += TranslateExpression(context, expression->down);

        ASSERT(IsFloating(outType));

        Location    inLoc = expression->down->expr.loc;
        Type *      inType = expression->down->expr.type;
        size_t      inSize = TypeSize(inType);

        ASSERT(IsFloating(inType));
        ASSERT(outSize != inSize);

        // float-4  -> float-8      = cvtss2sd
        // float-8  -> float-4      = cvtsd2ss
        // float-*  -> float-*      = error

        if (inSize == 4 && outSize == 8)
        {
            // movss xmm1, inLoc
            // cvtss2sd xmm0, xmm1
            // movsd outLoc, xmm0
            s += "movss " + XMM(1) + ", " + X64LocationString(inLoc, inSize) + "\n" +
                 "cvtss2sd " + XMM(0) + ", " + XMM(1) + "\n" +
                 "movsd " + X64LocationString(outLoc, outSize) + ", " + XMM(0) + "\n";
        }
        else if (inSize == 8 && outSize == 4)
        {
            // movsd xmm1, inLoc
            // cvtsd2ss xmm0, xmm1
            // movss outLoc, xmm0
            s += "movsd " + XMM(1) + ", " + X64LocationString(inLoc, inSize) + "\n" +
                 "cvtsd2ss " + XMM(0) + ", " + XMM(1) + "\n" +
                 "movss " + X64LocationString(outLoc, outSize) + ", " + XMM(0) + "\n";
        }
        else
        {
            // SSE don't support long double
            ASSERT(false);
        }
    }
    else if (expression->type == EXPR_CVT_I2F)
    {
        // visit children
        s += TranslateExpression(context, expression->down);

        ASSERT(IsFloating(outType));

        Location    inLoc = expression->down->expr.loc;
        Type *      inType = expression->down->expr.type;
        size_t      inSize = TypeSize(inType);

        ASSERT(IsInt(inType));

        // int-* -> float-4         = cvt_i2i(i-4/i-8) + cvtsi2ss
        // int-* -> float-8         = cvt_i2i(i-8) + cvtsi2sd
        // int-* -> float-10        = error

        if (outSize == 4)
        {
            size_t width = inSize > 4 ? 8 : 4;

            // mov ax, inLoc
            // cvt_i2i rax, ax
            // cvtsi2ss xmm0, rax
            // movss outLoc, xmm0
            s += "mov " + AX(inSize) + ", " + X64LocationString(inLoc, inSize) + "\n";
            {
                if (width > inSize)
                {
                    s += "movsx " + AX(width) + ", " + AX(inSize) + "\n";
                }
            }
            s += "cvtsi2ss " + XMM(0) + ", " + AX(inSize) + "\n";
            s += "movss " + X64LocationString(outLoc, outSize) + ", " + XMM(0) + "\n";
        }
        else if (outSize == 8)
        {
            size_t width = 8;

            // mov ax, inLoc
            // cvt_i2i rax, ax
            // cvtsi2sd xmm0, rax
            // movsd outLoc, xmm0
            s += "mov " + AX(inSize) + ", " + X64LocationString(inLoc, inSize) + "\n";
            {
                if (width > inSize)
                {
                    s += "movsx " + AX(width) + ", " + AX(inSize) + "\n";
                }
            }
            s += "cvtsi2sd " + XMM(0) + ", " + AX(inSize) + "\n";
            s += "movsd " + X64LocationString(outLoc, outSize) + ", " + XMM(0) + "\n";
        }
        else
        {
            // SSE don't support long double
            ASSERT(false);
        }
    }
    else if (expression->type == EXPR_CVT_B2F)
    {
        // visit children
        s += TranslateExpression(context, expression->down);

        ASSERT(IsFloating(outType));

        Location    inLoc = expression->down->expr.loc;
        Type *      inType = expression->down->expr.type;
        size_t      inSize = TypeSize(inType);

        ASSERT(IsBool(inType));

        // bool -> float-4          = cvt_i2i(i-4) + cvtsi2ss
        // bool -> float-8          = cvt_i2i(i-8) + cvtsi2sd
        // bool -> float-10         = error

        if (outSize == 4)
        {
            // mov ax, inLoc
            // movsx rax, ax
            // cvtsi2ss xmm0, rax
            // movss outLoc, xmm0
            s += "mov " + AX(inSize) + ", " + X64LocationString(inLoc, inSize) + "\n" +
                 "movsx " + AX(outSize) + ", " + AX(inSize) + "\n" +
                 "cvtsi2ss " + XMM(0) + ", " + AX(inSize) + "\n" +
                 "movss " + X64LocationString(outLoc, outSize) + ", " + XMM(0) + "\n";
        }
        else if (outSize == 8)
        {
            // mov ax, inLoc
            // movsx rax, ax
            // cvtsi2sd xmm0, rax
            // movsd outLoc, xmm0
            s += "mov " + AX(inSize) + ", " + X64LocationString(inLoc, inSize) + "\n" +
                 "movsx " + AX(outSize) + ", " + AX(inSize) + "\n" +
                 "cvtsi2sd " + XMM(0) + ", " + AX(inSize) + "\n" +
                 "movsd " + X64LocationString(outLoc, outSize) + ", " + XMM(0) + "\n";
        }
        else
        {
            // SSE don't support long double
            ASSERT(false);
        }
    }
    else if (expression->type == EXPR_CVT_I2B)
    {
        // visit children
        s += TranslateExpression(context, expression->down);

        ASSERT(IsBool(outType));

        Location    inLoc = expression->down->expr.loc;
        Type *      inType = expression->down->expr.type;
        size_t      inSize = TypeSize(inType);

        ASSERT(IsInt(inType));

        // mov rax, inLoc
        // cmp rax, 0
        // mov rcx, 1
        // cmovne rax, rcx
        // mov outLoc, rax
        s += "mov " + AX(inSize) + ", " + X64LocationString(inLoc, inSize) + "\n" +
             "cmp " + AX(inSize) + ", " + IntegerToHexString(0) + "\n" +
             "mov " + CX(2) + ", " + IntegerToHexString(1) + "\n" +
             "cmovne " + AX(2) + ", " + CX(2) + "\n" +
             "mov " + X64LocationString(outLoc, outSize) + ", " + AX(outSize) + "\n";
    }
    else if (expression->type == EXPR_CVT_F2B)
    {
        // visit children
        s += TranslateExpression(context, expression->down);

        ASSERT(IsBool(outType));

        Location    inLoc = expression->down->expr.loc;
        Type *      inType = expression->down->expr.type;
        size_t      inSize = TypeSize(inType);

        ASSERT(IsFloating(inType));

        // float-4  -> bool         = cmpness
        // float-8  -> bool         = cmpnesd
        // float-10 -> bool         = error

        // movss xmm0, inLoc
        // movss xmm1, __real@00000000
        // cmpness xmm0, xmm1
        // movss <tmp>, xmm0
        // mov eax, <tmp>
        // mov outLoc, ax
        if (inSize == 4)
        {
            s += "movss " + XMM(0) + ", " + X64LocationString(inLoc, inSize) + "\n" +
                 "movss " + XMM(1) + ", " + X64LocationString(LocateFloat(context->constantContext, 0.0f), inSize) + "\n" +
                 "cmpness " + XMM(0) + ", " + XMM(1) + "\n" +
                 "movss " + X64LocationString(GetSwapLocation(context), inSize) + ", " + XMM(0) + "\n" +
                 "mov " + AX(inSize) + ", " + X64LocationString(GetSwapLocation(context), inSize) + "\n" +
                 "mov " + X64LocationString(outLoc, outSize) + ", " + AX(outSize) + "\n";
        }
        else if (inSize == 8)
        {
            s += "movsd " + XMM(0) + ", " + X64LocationString(inLoc, inSize) + "\n" +
                 "movsd " + XMM(1) + ", " + X64LocationString(LocateFloat(context->constantContext, 0.0f), inSize) + "\n" +
                 "cmpnesd " + XMM(0) + ", " + XMM(1) + "\n" +
                 "movsd " + X64LocationString(GetSwapLocation(context), inSize) + ", " + XMM(0) + "\n" +
                 "mov " + AX(inSize) + ", " + X64LocationString(GetSwapLocation(context), inSize) + "\n" +
                 "mov " + X64LocationString(outLoc, outSize) + ", " + AX(outSize) + "\n";
        }
        else
        {
            // SSE don't support long double
            ASSERT(false);
        }
    }
    else if (expression->type == EXPR_BOOL_NOT)
    {
        // visit children
        s += TranslateExpression(context, expression->down);

        ASSERT(IsBool(outType));

        Location    inLoc = expression->down->expr.loc;
        Type *      inType = expression->down->expr.type;
        size_t      inSize = TypeSize(inType);

        ASSERT(IsBool(inType));

        // mov rax, 1
        // sub rax, inLoc
        // mov outloc, rax
        s += "mov " + AX(1) + ", " + IntegerToHexString(1) + "\n" +
             "sub " + AX(1) + ", " + X64LocationString(inLoc, 1) + "\n" +
             "mov " + X64LocationString(outLoc, 1) + ", " + AX(1) + "\n";
    }
    else if (expression->type == EXPR_BOOL_AND ||
             expression->type == EXPR_BOOL_OR)
    {
        // visit children
        s += TranslateExpression(context, expression->down);
        s += TranslateExpression(context, expression->down->right);

        ASSERT(IsBool(outType));

        Location    inLoc1 = expression->down->expr.loc;
        Type *      inType1 = expression->down->expr.type;
        size_t      inSize1 = TypeSize(inType1);
        
        Location    inLoc2 = expression->down->right->expr.loc;
        Type *      inType2 = expression->down->right->expr.type;
        size_t      inSize2 = TypeSize(inType2);

        ASSERT(IsBool(inType1) && IsBool(inType2));

        std::string op;
        switch (expression->type)
        {
            case EXPR_BOOL_AND: op = "and "; break;
            case EXPR_BOOL_OR:  op = "or "; break;
            default: ASSERT(false); break;
        }

        // mov rax, inLoc1
        // op  rax, inLoc2
        // mov outLoc, rax
        s += "mov " + AX(1) + ", " + X64LocationString(inLoc1, 1) + "\n" +
             op     + AX(1) + ", " + X64LocationString(inLoc2, 1) + "\n" +
             "mov " + X64LocationString(outLoc, 1) + ", " + AX(1) + "\n";
    }
    else if (expression->type == EXPR_INEG ||
             expression->type == EXPR_INOT ||
             expression->type == EXPR_IINC ||
             expression->type == EXPR_IDEC)
    {
        // visit children
        s += TranslateExpression(context, expression->down);

        ASSERT(IsInt(outType) && outSize >= 4);

        Location    inLoc = expression->down->expr.loc;
        Type *      inType = expression->down->expr.type;
        size_t      inSize = TypeSize(inType);

        ASSERT(IsInt(inType) && inSize == outSize);

        size_t      width = outSize;

        ASSERT(width == inSize);

        std::string op;
        switch (expression->type)
        {
            case EXPR_INEG: op = "neg "; break;
            case EXPR_INOT: op = "not "; break;
            case EXPR_IINC: op = "inc "; break;
            case EXPR_IDEC: op = "dec "; break;
            default: ASSERT(false); break;
        }

        // mov rax, inLoc
        // op  rax
        // mov outLoc, rax
        s += "mov " + AX(width) + ", " + X64LocationString(inLoc, width) + "\n" +
             op     + AX(width) + "\n" +
             "mov " + X64LocationString(outLoc, width) + ", " + AX(width) + "\n";
    }
    else if (expression->type == EXPR_IADD ||
             expression->type == EXPR_ISUB ||
             expression->type == EXPR_IMUL ||
             expression->type == EXPR_IDIV ||
             expression->type == EXPR_IMOD ||
             expression->type == EXPR_IAND ||
             expression->type == EXPR_IXOR ||
             expression->type == EXPR_IOR)
    {
        // visit children
        s += TranslateExpression(context, expression->down);
        s += TranslateExpression(context, expression->down->right);

        ASSERT(IsInt(outType));

        Location    inLoc1 = expression->down->expr.loc;
        Type *      inType1 = expression->down->expr.type;
        size_t      inSize1 = TypeSize(inType1);

        Location    inLoc2 = expression->down->right->expr.loc;
        Type *      inType2 = expression->down->right->expr.type;
        size_t      inSize2 = TypeSize(inType2);

        ASSERT(IsInt(inType1) && IsInt(inType2));

        size_t      width = outSize;

        ASSERT(width == inSize1 && width == inSize2);

        std::string op;
        switch (expression->type)
        {
            case EXPR_IADD: op = "add "; break;
            case EXPR_ISUB: op = "sub "; break;
            case EXPR_IMUL: op = "mul "; break;
            case EXPR_IDIV: op = "div "; break;
            case EXPR_IMOD: op = "div "; break;
            case EXPR_IAND: op = "and "; break;
            case EXPR_IXOR: op = "xor "; break;
            case EXPR_IOR:  op = "or "; break;
            default: ASSERT(false); break;
        }


        if (expression->type == EXPR_IMUL ||
            expression->type == EXPR_IDIV ||
            expression->type == EXPR_IMOD)
        {
            std::string result = expression->type == EXPR_IMOD
                                 ? DX(width)
                                 : AX(width);

            // mov rax, inLoc1
            // op  inLoc2
            // mov outLoc, result
            s += "mov " + AX(width) + ", " + X64LocationString(inLoc1, width) + "\n" +
                 op     + X64LocationString(inLoc2, width) + "\n" +
                 "mov " + X64LocationString(outLoc, width) + ", " + result + "\n";
        }
        else
        {
            // mov rax, inLoc1
            // op  rax, inLoc2
            // mov outLoc, rax
            s += "mov " + AX(width) + ", " + X64LocationString(inLoc1, width) + "\n" +
                 op     + AX(width) + ", " + X64LocationString(inLoc2, width) + "\n" +
                 "mov " + X64LocationString(outLoc, width) + ", " + AX(width) + "\n";
        }
    }
    else if (expression->type == EXPR_ISHL ||
             expression->type == EXPR_ISHR)
    {
        // visit children
        s += TranslateExpression(context, expression->down);
        s += TranslateExpression(context, expression->down->right);

        ASSERT(IsInt(outType));

        Location    inLoc1 = expression->down->expr.loc;
        Type *      inType1 = expression->down->expr.type;
        size_t      inSize1 = TypeSize(inType1);

        Location    inLoc2 = expression->down->right->expr.loc;
        Type *      inType2 = expression->down->right->expr.type;
        size_t      inSize2 = TypeSize(inType2);

        ASSERT(IsInt(inType1) && IsInt(inType2));
        
        size_t      width = outSize;

        ASSERT(width == inSize1);

        std::string op;
        switch (expression->type)
        {
            case EXPR_ISHL: op = "shl "; break;
            case EXPR_ISHR: op = "shr "; break;
            default: ASSERT(false); break;
        }

        // XXX: shift count >= 256
        // mov cl, inLoc2
        // mov rax, inLoc1
        // op  rax, cl
        // mov outLoc, rax
        s += "mov " + CX(1) + ", " + X64LocationString(inLoc2, 1) + "\n" +
             "mov " + AX(width) + ", " + X64LocationString(inLoc1, width) + "\n" +
             op     + AX(width) + ", " + CX(1) + "\n" +
             "mov " + X64LocationString(outLoc, width) + ", " + AX(width) + "\n";
    }
    else if (expression->type == EXPR_IEQ ||
             expression->type == EXPR_INE ||
             expression->type == EXPR_ILT ||
             expression->type == EXPR_ILE ||
             expression->type == EXPR_IGE ||
             expression->type == EXPR_IGT)
    {
        // visit children
        s += TranslateExpression(context, expression->down);
        s += TranslateExpression(context, expression->down->right);

        ASSERT(IsBool(outType));

        Location    inLoc1 = expression->down->expr.loc;
        Type *      inType1 = expression->down->expr.type;
        size_t      inSize1 = TypeSize(inType1);

        Location    inLoc2 = expression->down->right->expr.loc;
        Type *      inType2 = expression->down->right->expr.type;
        size_t      inSize2 = TypeSize(inType2);

        ASSERT(IsInt(inType1) && IsInt(inType2));

        size_t      width = inSize1;

        ASSERT(width == inSize2);

        std::string cmov;
        switch (expression->type)
        {
            case EXPR_IEQ: cmov = "cmove "; break;
            case EXPR_INE: cmov = "cmovne "; break;
            case EXPR_ILT: cmov = "cmovl "; break;
            case EXPR_ILE: cmov = "cmovle "; break;
            case EXPR_IGE: cmov = "cmovge "; break;
            case EXPR_IGT: cmov = "cmovg "; break;
            default: ASSERT(false); break;
        }

        // mov rax, inLoc1
        // mov rdx, inLoc2
        // cmp rax, rdx
        // mov rax, 0
        // mov rcx, 1
        // cmovXX rax, rcx
        // mov outLoc, rax
        s += "mov " + AX(width) + ", " + X64LocationString(inLoc1, width) + "\n" +
             "mov " + DX(width) + ", " + X64LocationString(inLoc2, width) + "\n" +
             "cmp " + AX(width) + ", " + DX(width) + "\n" +
             "mov " + AX(1) + ", " + IntegerToHexString(0) + "\n" +
             "mov " + CX(2) + ", " + IntegerToHexString(1) + "\n" +
             cmov   + AX(2) + ", " + CX(2) + "\n" +
             "mov " + X64LocationString(outLoc, 1) + ", " + AX(1) + "\n";
    }
    else if (expression->type == EXPR_FNEG)
    {
        // visit children
        s += TranslateExpression(context, expression->down);

        ASSERT(IsFloating(outType) && outSize == 4);

        Location    inLoc = expression->down->expr.loc;
        Type *      inType = expression->down->expr.type;
        size_t      inSize = TypeSize(inType);

        ASSERT(IsFloating(inType) && inSize == outSize);

        size_t      width = outSize;

        ASSERT(width == inSize);

        // movss xmm0, inLoc
        // xorps xmm0, DWORD PTR __xmm@80000000
        // movss outLoc, xmm0
        s += "movss " + XMM(0) + ", " + X64LocationString(inLoc, width) + "\n" +
             "xorps " + XMM(0) + ", " + X64LocationString(LocateFloat(context->constantContext, 0x80000000), width) + "\n" +
             "movss " + X64LocationString(outLoc, width) + ", " + XMM(0) + "\n";
    }
    else if (expression->type == EXPR_FADD ||
             expression->type == EXPR_FSUB ||
             expression->type == EXPR_FMUL ||
             expression->type == EXPR_FDIV)
    {
        // visit children
        s += TranslateExpression(context, expression->down);
        s += TranslateExpression(context, expression->down->right);

        ASSERT(IsFloating(outType) && outSize == 4);

        Location    inLoc1 = expression->down->expr.loc;
        Type *      inType1 = expression->down->expr.type;
        size_t      inSize1 = TypeSize(inType1);

        Location    inLoc2 = expression->down->expr.loc;
        Type *      inType2 = expression->down->expr.type;
        size_t      inSize2 = TypeSize(inType2);

        ASSERT(IsFloating(inType1) && IsFloating(inType2));
        
        size_t      width = outSize;

        ASSERT(width == inSize1 && width == inSize2);

        std::string op;
        switch (expression->type)
        {
            case EXPR_FADD: op = "addss "; break;
            case EXPR_FSUB: op = "subss "; break;
            case EXPR_FMUL: op = "mulss "; break;
            case EXPR_FDIV: op = "divss "; break;
            default: ASSERT(false); break;
        }

        // movss xmm0, inLoc1
        // movss xmm1, inLoc2
        // op    xmm0, xmm1
        // movss outLoc, xmm0
        s += "movss " + XMM(0) + ", " + X64LocationString(inLoc1, width) + "\n" +
             "movss " + XMM(1) + ", " + X64LocationString(inLoc2, width) + "\n" +
             op       + XMM(0) + ", " + XMM(1) + "\n" +
             "movss " + X64LocationString(outLoc, width) + ", " + XMM(0) + "\n";
    }
    else if (expression->type == EXPR_FEQ)
    {
        // visit children
        s += TranslateExpression(context, expression->down);
        s += TranslateExpression(context, expression->down->right);

        ASSERT(IsBool(outType));

        Location    inLoc1 = expression->down->expr.loc;
        Type *      inType1 = expression->down->expr.type;
        size_t      inSize1 = TypeSize(inType1);

        Location    inLoc2 = expression->down->right->expr.loc;
        Type *      inType2 = expression->down->right->expr.type;
        size_t      inSize2 = TypeSize(inType2);

        ASSERT(IsFloating(inType1) && IsFloating(inType2));

        size_t      width = inSize1;

        ASSERT(width == inSize2);

        std::string op;
        switch (expression->type)
        {
            case EXPR_FEQ: op = "cmpeqss "; break;
            case EXPR_FNE: op = "cmpness "; break;
            case EXPR_FLT: op = "cmpltss "; break;
            case EXPR_FLE: op = "cmpless "; break;
            case EXPR_FGE: op = "cmpgess "; break;
            case EXPR_FGT: op = "cmpgtss "; break;
            default: ASSERT(false); break;
        }

        // movss xmm2, inLoc1 ; save left operand
        // movss xmm0, inLoc1
        // movss xmm1, inLoc2
        // op    xmm0, xmm1
        // movss inLoc1, xmm0 ; use left operand space to compute result
        // mov   rax, inLoc1
        // cmp   rax, 0
        // mov   rcx, 1
        // cmovne rax, rcx
        // mov   outLoc, rax
        // movss inLoc1, xmm2 ; restore left operand
        s += "movss " + XMM(2) + ", " + X64LocationString(inLoc1, width) + "\n" + 
             "movss " + XMM(0) + ", " + X64LocationString(inLoc1, width) + "\n" +
             "movss " + XMM(1) + ", " + X64LocationString(inLoc2, width) + "\n" +
             op       + XMM(0) + ", " + XMM(1) + "\n" +
             "movss " + X64LocationString(inLoc1, width) + ", " + XMM(0) + "\n" +
             "mov "   + AX(1) + ", " + X64LocationString(inLoc1, 1) + "\n" +
             "cmp "   + AX(1) + ", " + IntegerToHexString(0) + "\n" +
             "mov "   + CX(2) + ", " + IntegerToHexString(1) + "\n" +
             "cmovne " + AX(2) + ", " + CX(2) + "\n" +             
             "mov "   + X64LocationString(outLoc, 1) + ", " + AX(1) + "\n" +
             "movss " + X64LocationString(inLoc1, width) + ", " + XMM(2) + "\n";
    }
    else if (expression->type == EXPR_PADD ||
             expression->type == EXPR_PSUB)
    {
        // visit children
        s += TranslateExpression(context, expression->down);
        s += TranslateExpression(context, expression->down->right);

        ASSERT(IsPointerToObject(outType));

        Location    inLoc1 = expression->down->expr.loc;
        Type *      inType1 = expression->down->expr.type;
        size_t      inSize1 = TypeSize(inType1);

        Location    inLoc2 = expression->down->right->expr.loc;
        Type *      inType2 = expression->down->right->expr.type;
        size_t      inSize2 = TypeSize(inType2);

        ASSERT(IsPointerToObject(inType1) && IsIntegral(inType2));

        size_t      width = outSize;

        ASSERT(width == inSize1 && width == inSize2);

        int         shiftStep = (expression->type == EXPR_PADD)
                                ? static_cast<int>(TypeSize(AsPointer(outType)->target))
                                : -static_cast<int>(TypeSize(AsPointer(outType)->target));

        // mov rax, inLoc2
        // mov rcx, shiftStep
        // imul rcx
        // add rax, inLoc1
        // mov outLoc, rax
        s += "mov " + AX(width) + ", " + X64LocationString(inLoc2, width) + "\n" +
             "mov " + CX(width) + ", " + IntegerToHexString(shiftStep) + "\n" +
             "mul " + CX(width) + "\n" +
             "add " + AX(width) + ", " + X64LocationString(inLoc1, width) + "\n" +
             "mov " + X64LocationString(outLoc, width) + ", " + AX(width) + "\n";
    }
    else if (expression->type == EXPR_PDIFF)
    {
        // visit children
        s += TranslateExpression(context, expression->down);
        s += TranslateExpression(context, expression->down->right);

        ASSERT(IsPtrDiffType(outType));

        Location    inLoc1 = expression->down->expr.loc;
        Type *      inType1 = expression->down->expr.type;
        size_t      inSize1 = TypeSize(inType1);

        Location    inLoc2 = expression->down->right->expr.loc;
        Type *      inType2 = expression->down->right->expr.type;
        size_t      inSize2 = TypeSize(inType2);

        ASSERT(IsPointerToObject(inType1) && IsPointerToObject(inType2));

        size_t      width = outSize;

        ASSERT(width == inSize1 && width == inSize2);

        Type *      inObjType1 = AsPointer(inType1)->target;
        Type *      inObjType2 = AsPointer(inType2)->target;

        ASSERT(TypeEqual(inObjType1, inObjType2));

        int         objSize = static_cast<int>(TypeSize(inObjType1));

        // mov rax, inLoc1
        // sub rax, inLoc2
        // mov rcx, objSize
        // div rcx
        // mov outLoc, rax
        s += "mov " + AX(width) + ", " + X64LocationString(inLoc1, width) + "\n" +
             "sub " + AX(width) + ", " + X64LocationString(inLoc2, width) + "\n" +
             "mov " + CX(width) + ", " + IntegerToHexString(objSize) + "\n" +
             "div " + CX(width) + "\n" +
             "mov " + X64LocationString(outLoc, width) + ", " + AX(width) + "\n";
    }
    else if (expression->type == EXPR_PVAL)
    {
        // TODO: rewrite all outLoc/inLoc* code to handle REGISTER_INDIRECT
    }
    else if (expression->type == EXPR_PEQ ||
             expression->type == EXPR_PNE ||
             expression->type == EXPR_PLT ||
             expression->type == EXPR_PLE ||
             expression->type == EXPR_PGE ||
             expression->type == EXPR_PGT)
    {
        // visit children
        s += TranslateExpression(context, expression->down);
        s += TranslateExpression(context, expression->down->right);

        ASSERT(IsBool(outType));

        Location    inLoc1 = expression->down->expr.loc;
        Type *      inType1 = expression->down->expr.type;
        size_t      inSize1 = TypeSize(inType1);

        Location    inLoc2 = expression->down->right->expr.loc;
        Type *      inType2 = expression->down->right->expr.type;
        size_t      inSize2 = TypeSize(inType2);

        ASSERT(IsPointer(inType1) && IsPointer(inType2));

        size_t      width = inSize1;

        ASSERT(width == inSize2);

        std::string cmov;
        switch (expression->type)
        {
            case EXPR_PEQ: cmov = "cmove "; break;
            case EXPR_PNE: cmov = "cmovne "; break;
            case EXPR_PLT: cmov = "cmovl "; break;
            case EXPR_PLE: cmov = "cmovle "; break;
            case EXPR_PGE: cmov = "cmovge "; break;
            case EXPR_PGT: cmov = "cmovg "; break;
            default: ASSERT(false); break;
        }

        // mov rax, inLoc1
        // mov rdx, inLoc2
        // cmp rax, rdx
        // mov rax, 0
        // mov rcx, 1
        // cmovXX rax, rcx
        // mov outLoc, rax
        s += "mov " + AX(width) + ", " + X64LocationString(inLoc1, width) + "\n" +
             "mov " + DX(width) + ", " + X64LocationString(inLoc2, width) + "\n" +
             "cmp " + AX(width) + ", " + DX(width) + "\n" +
             "mov " + AX(1) + ", " + IntegerToHexString(0) + "\n" +
             "mov " + CX(2) + ", " + IntegerToHexString(1) + "\n" +
             cmov   + AX(2) + ", " + CX(2) + "\n" +
             "mov " + X64LocationString(outLoc, 1) + ", " + AX(1) + "\n";
    }
    else if (expression->type == EXPR_MADDR)
    {
        // visit children
        s += TranslateExpression(context, expression->down);

        ASSERT(IsPointer(outType));

        Location    inLoc = expression->down->expr.loc;
        Type *      inType = expression->down->expr.type;
        size_t      inSize = TypeSize(inType);

        size_t      width = outSize;

        // lea rax, inLoc1
        // mov outLoc, rax
        s += "lea " + AX(width) + ", " + X64LocationString(inLoc, width) + "\n" +
             "mov " + X64LocationString(outLoc, width) + ", " + AX(width) + "\n";
    }
    else if (expression->type == EXPR_MCOPY)
    {
        // visit children
        s += TranslateExpression(context, expression->down);
        s += TranslateExpression(context, expression->down->right);

        Location    inLoc1 = expression->down->expr.loc;
        Type *      inType1 = expression->down->expr.type;
        size_t      inSize1 = TypeSize(inType1);

        Location    inLoc2 = expression->down->right->expr.loc;
        Type *      inType2 = expression->down->right->expr.type;
        size_t      inSize2 = TypeSize(inType2);

        ASSERT(TypeEqual(outType, inType1));
        ASSERT(outSize == inSize1 && outSize == inSize2);

        int         objSize = static_cast<int>(inSize1);

        if (CountBits(objSize) == 1 && objSize <= 8)
        {
            // 1,2,4,8 -> rax
            size_t      width = objSize;

            // mov rax, inLoc2
            // mov inLoc1, rax
            if  (outLoc.type != REGISTER_INDIRECT)
            {
                s += "mov " + AX(width) + ", " + X64LocationString(inLoc2, width) + "\n" +
                     "mov " + X64LocationString(outLoc, width) + ", " + AX(width) + "\n";
            }
            else
            {
                // mov rax, inLoc2
                // mov rcx, pval
                // *mov rcx, [rcx]
                // mov [rcx], rax
                ASSERT(expression->down->type == EXPR_PVAL);

                Location origin;
                int pvalCount = 0;

                Node * pvalChain = expression->down;
                while(pvalChain->type == EXPR_PVAL)
                {
                    ++pvalCount;
                    pvalChain = pvalChain->down;
                }
                origin = pvalChain->expr.loc;

                s += "mov " + AX(width) + ", " + X64LocationString(inLoc2, width) + "\n";
                s += "mov " + CX(8) + ", " + X64LocationString(origin, 8) + "\n";
                for (int i = 1; i < pvalCount; ++i)
                {
                    s += "mov " + CX(8) + ", " + SizeToX64TypeString(8) + " PTR [" + CX(8) + "]\n";
                }
                s += "mov " + SizeToX64TypeString(width) + " PTR [" + CX(8) + "], " + AX(width) + "\n";
            }
        }
        else
        {
            // *       -> rep movsb
            size_t      width = 8;

            // lea rax, inLoc1
            // lea rcx, inLoc2
            // mov rdi, rax
            // mov rsi, rcx
            // mov rcx, objSize
            // rep movsb
            s += "lea " + AX(width) + ", " + X64LocationString(inLoc1, width) + "\n" +
                 "lea " + CX(width) + ", " + X64LocationString(inLoc2, width) + "\n" +
                 "mov " + DI(width) + ", " + AX(width) + "\n" +
                 "mov " + SI(width) + ", " + CX(width) + "\n" +
                 "mov " + CX(width) + ", " + IntegerToHexString(objSize) + "\n" +
                 "rep movsb" + "\n";

            AssertRegisterMask(context, RDI_MASK);
            AssertRegisterMask(context, RSI_MASK);
        }
    }
    else if (expression->type == EXPR_MDUP)
    {
        // visit children
        s += TranslateExpression(context, expression->down);

        Location    inLoc = expression->down->expr.loc;
        Type *      inType = expression->down->expr.type;
        size_t      inSize = TypeSize(inType);

        ASSERT(TypeEqual(outType, inType));
        ASSERT(outSize == inSize);

        if (CountBits(inSize) == 1 && inSize <= 8)
        {
            // 1,2,4,8 -> rax
            size_t      width = inSize;

            // mov rax, inLoc
            // mov outLoc, rax
            if (inLoc.type != REGISTER_INDIRECT)
            {
                s += "mov " + AX(width) + ", " + X64LocationString(inLoc, width) + "\n" +
                     "mov " + X64LocationString(outLoc, width) + ", " + AX(width) + "\n";
            }
            else
            {
                ASSERT(outLoc.type != REGISTER_INDIRECT);
                
                // MDUP( out, MCOPY(PVAL(p), 2) )
                // Convention: value is in rax
                // mov outLoc, rax
                s += "mov " + X64LocationString(outLoc, width) + ", " + AX(width) + "\n";
            }
        }
        else
        {
            // *       -> rep movsb
            size_t      width = 8;

            // lea rax, outLoc
            // lea rcx, inLoc
            // mov rdi, rax
            // mov rsi, rcx
            // mov rcx, objSize
            // rep movsb
            s += "lea " + AX(width) + ", " + X64LocationString(outLoc, width) + "\n" +
                "lea " + CX(width) + ", " + X64LocationString(inLoc, width) + "\n" +
                "mov " + DI(width) + ", " + AX(width) + "\n" +
                "mov " + SI(width) + ", " + CX(width) + "\n" +
                "mov " + CX(width) + ", " + IntegerToHexString(inSize) + "\n" +
                "rep movsb" + "\n";

            AssertRegisterMask(context, RDI_MASK);
            AssertRegisterMask(context, RSI_MASK);
        }
    }
    else if (expression->type == EXPR_MADD)
    {
        // visit children
        s += TranslateExpression(context, expression->down);
        s += TranslateExpression(context, expression->down->right);

        ASSERT(IsPointer(outType));

        Location    inLoc1 = expression->down->expr.loc;
        Type *      inType1 = expression->down->expr.type;
        size_t      inSize1 = TypeSize(inType1);

        Location    inLoc2 = expression->down->right->expr.loc;
        Type *      inType2 = expression->down->right->expr.type;
        size_t      inSize2 = TypeSize(inType2);

        ASSERT(IsPointer(inType1) && IsInt(inType2));

        size_t      width = outSize;

        ASSERT(width == inSize1 && width == inSize2);

        // mov rax, inLoc1
        // add rax, inLoc2
        // mov outLoc, rax
        s += "mov " + AX(width) + ", " + X64LocationString(inLoc1, width) + "\n" +
             "add " + AX(width) + ", " + X64LocationString(inLoc2, width) + "\n" +
             "mov " + X64LocationString(outLoc, width) + ", " + AX(width) + "\n";
    }
    else if (expression->type == EXPR_CONDITION)
    {
        // visit children
        s += TranslateExpression(context, expression->down);
        s += TranslateExpression(context, expression->down->right);
        s += TranslateExpression(context, expression->down->right->right);

        Location    condLoc = expression->down->expr.loc;
        Type *      condType = expression->down->expr.type;
        size_t      condSize = TypeSize(condType);

        Location    inLoc1 = expression->down->right->expr.loc;
        Type *      inType1 = expression->down->right->expr.type;
        size_t      inSize1 = TypeSize(inType1);

        Location    inLoc2 = expression->down->right->right->expr.loc;
        Type *      inType2 = expression->down->right->right->expr.type;
        size_t      inSize2 = TypeSize(inType2);

        ASSERT(IsBool(condType));

        std::string L0 = std::string("@L") + std::to_string(context->nextUniqueLabel++);
        std::string L1 = std::string("@L") + std::to_string(context->nextUniqueLabel++);

        // mov rax, condLoc
        // cmp rax, 0
        // je L0
        // mcopy outLoc, inLoc1
        // jmp L1
        // L0:
        // mcopy outLoc, inLoc2
        // L1:
        s += "mov " + AX(condSize) + ", " + X64LocationString(condLoc, condSize) + "\n" +
             "cmp " + AX(condSize) + ", " + IntegerToHexString(0) + "\n" +
             "je " + L0 + "\n";
        {
            if (CountBits(inSize1) == 1 && inSize1 <= 8)
            {
                // 1,2,4,8 -> rax
                size_t      width = inSize1;

                // mov rax, inLoc
                // mov outLoc, rax
                s += "mov " + AX(width) + ", " + X64LocationString(inLoc1, width) + "\n" +
                     "mov " + X64LocationString(outLoc, width) + ", " + AX(width) + "\n";
            }
            else
            {
                // *       -> rep movsb
                size_t      width = 8;

                // lea rax, outLoc
                // lea rcx, inLoc
                // mov rdi, rax
                // mov rsi, rcx
                // mov rcx, objSize
                // rep movsb
                s += "lea " + AX(width) + ", " + X64LocationString(outLoc, width) + "\n" +
                     "lea " + CX(width) + ", " + X64LocationString(inLoc1, width) + "\n" +
                     "mov " + DI(width) + ", " + AX(width) + "\n" +
                     "mov " + SI(width) + ", " + CX(width) + "\n" +
                     "mov " + CX(width) + ", " + IntegerToHexString(inSize1) + "\n" +
                     "rep movsb" + "\n";

                AssertRegisterMask(context, RDI_MASK);
                AssertRegisterMask(context, RSI_MASK);
            }
        }
        s += "jmp " + L1 + "\n";
        s += L0 + ":\n";
        {
            if (CountBits(inSize2) == 1 && inSize2 <= 8)
            {
                // 1,2,4,8 -> rax
                size_t      width = inSize2;

                // mov rax, inLoc
                // mov outLoc, rax
                s += "mov " + AX(width) + ", " + X64LocationString(inLoc2, width) + "\n" +
                     "mov " + X64LocationString(outLoc, width) + ", " + AX(width) + "\n";
            }
            else
            {
                // *       -> rep movsb
                size_t      width = 8;

                // lea rax, outLoc
                // lea rcx, inLoc
                // mov rdi, rax
                // mov rsi, rcx
                // mov rcx, objSize
                // rep movsb
                s += "lea " + AX(width) + ", " + X64LocationString(outLoc, width) + "\n" +
                     "lea " + CX(width) + ", " + X64LocationString(inLoc2, width) + "\n" +
                     "mov " + DI(width) + ", " + AX(width) + "\n" +
                     "mov " + SI(width) + ", " + CX(width) + "\n" +
                     "mov " + CX(width) + ", " + IntegerToHexString(inSize2) + "\n" +
                     "rep movsb" + "\n";

                AssertRegisterMask(context, RDI_MASK);
                AssertRegisterMask(context, RSI_MASK);
            }
        }
        s += L1 + ":\n";
    }
    else if (expression->type == EXPR_ELIST)
    {
        // visit children
        for (Node * child = expression->down;
             child;
             child = child->right)
        {
            s += TranslateExpression(context, child);
        }

        Node *      lastChild = LastChild(expression);
        Location    inLoc = lastChild->expr.loc;
        Type *      inType = lastChild->expr.type;
        size_t      inSize = TypeSize(inType);

        ASSERT(TypeEqual(outType, inType));
        ASSERT(outSize == inSize);

        // mdup outLoc, inLoc
        {
            if (CountBits(inSize) == 1 && inSize <= 8)
            {
                // 1,2,4,8 -> rax
                size_t      width = inSize;

                // mov rax, inLoc
                // mov outLoc, rax
                s += "mov " + AX(width) + ", " + X64LocationString(inLoc, width) + "\n" +
                     "mov " + X64LocationString(outLoc, width) + ", " + AX(width) + "\n";
            }
            else
            {
                // *       -> rep movsb
                size_t      width = 8;

                // lea rax, outLoc
                // lea rcx, inLoc
                // mov rdi, rax
                // mov rsi, rcx
                // mov rcx, objSize
                // rep movsb
                s += "lea " + AX(width) + ", " + X64LocationString(outLoc, width) + "\n" +
                     "lea " + CX(width) + ", " + X64LocationString(inLoc, width) + "\n" +
                     "mov " + DI(width) + ", " + AX(width) + "\n" +
                     "mov " + SI(width) + ", " + CX(width) + "\n" +
                     "mov " + CX(width) + ", " + IntegerToHexString(inSize) + "\n" +
                     "rep movsb" + "\n";

                AssertRegisterMask(context, RDI_MASK);
                AssertRegisterMask(context, RSI_MASK);
            }
        }
    }
    //else
    //{
    //    ASSERT(false);
    //}

    return s;
}

std::string TranslateReturnStatement(FunctionContext * context, Node * returnStatement)
{
    std::string s;

    ASSERT(returnStatement->down);
    Node * expr = returnStatement->down;

    if (expr->type != EMPTY_EXPRESSION)
    {
        context->currentIntention.push_back(WANT_NOTHING);
        s += TranslateExpression(context, expr);
        context->currentIntention.pop_back();

        // copy return value to reg/stack
        Type *      outType = expr->expr.type;
        size_t      outSize = TypeSize(outType);
        Location    outLoc = expr->expr.loc;
        s += "; copy return value\n";
        if (IsRAXType(expr->expr.type))
        {
            // mov rax, outLoc
            s += "mov " + AX(outSize) + ", " + X64LocationString(outLoc, outSize) + "\n";
        }
        else if (IsXMMType(expr->expr.type))
        {
            // movss xmm0, outLoc
            if (outSize == 4)
                s += "movss " + XMM(0) + ", " + X64LocationString(outLoc, outSize) + "\n";
            else if (outSize == 8)
                s += "movsd " + XMM(0) + ", " + X64LocationString(outLoc, outSize) + "\n";
            else
                ASSERT(false);
        }
        else
        {
            // TODO: copy non-register return value
            ASSERT(false);
        }
    }
    
    // epilog: add stack, restore regs
    s += "add rsp, " + IntegerToHexString(context->stackAllocSize) + "\n";
    s += "pop rdi\n";
    s += "pop rsi\n";
    s += "pop rbp\n";
    s += "ret\n";

    return s;
}

std::string TranslateStatement(FunctionContext * context,
                               Node * statement)
{
    ASSERT(statement && statement->type >= BEGIN_STATEMENT && statement->type <= END_STATEMENT);

    std::string s;

    if (statement->type == STMT_COMPOUND)
    {
        for (Node * child = statement->down;
             child;
             child = child->right)
        {
            s += TranslateStatement(context, child);
        }
    }
    else if (statement->type == STMT_EXPRESSION)
    {
        ASSERT(statement->down);
        Node * expr = statement->down;
        
        if (expr->type != EMPTY_EXPRESSION)
        {
            context->currentIntention.push_back(WANT_NOTHING);        
            s += TranslateExpression(context, expr);        
            context->currentIntention.pop_back();
        }
    }
    else if (statement->type == STMT_IF)
    {
        ASSERT(statement->down && statement->down->right);
        Node * expr = statement->down;
        Node * ifBody = expr->right;
        Node * elseBody = ifBody->right;

        context->currentIntention.push_back(WANT_VALUE);
        s += TranslateExpression(context, expr);
        context->currentIntention.pop_back();

        s += "cmp al, 0\n";
        if (!elseBody)
        {
            std::string L0 = std::string("@L") + std::to_string(context->nextUniqueLabel++);

            s += "je " + L0 + "\n";
            s += TranslateStatement(context, ifBody);
            s += L0 + ":\n";
        }
        else
        {
            std::string L0 = std::string("@L") + std::to_string(context->nextUniqueLabel++);
            std::string L1 = std::string("@L") + std::to_string(context->nextUniqueLabel++);

            s += "je " + L0 + "\n";
            s += TranslateStatement(context, ifBody);
            s += "jmp " + L1 + "\n";
            s += L0 + ":\n";
            s += TranslateStatement(context, elseBody);        
            s += L1 + ":\n";
        }
    }
    else if (statement->type == STMT_WHILE)
    {
        ASSERT(statement->down && statement->down->right);
        Node * expr = statement->down;
        Node * body = expr->right;

        ASSERT(context->targetToLabels.find(statement) != context->targetToLabels.end());
        const std::string & L0 = context->targetToLabels[statement][0];
        const std::string & L1 = context->targetToLabels[statement][1];

        s += L0 + ":\n";
        context->currentIntention.push_back(WANT_VALUE);
        s += TranslateExpression(context, expr);
        context->currentIntention.pop_back();
        s += "cmp al, 0\n";
        s += "je " + L1 + "\n";
        s += TranslateStatement(context, body);
        s += "jmp " + L0 + "\n";
        s += L1 + ":\n";
    }
    else if (statement->type == STMT_DO_WHILE)
    {
        ASSERT(statement->down && statement->down->right);
        Node * body = statement->down;
        Node * expr = body->right;

        ASSERT(context->targetToLabels.find(statement) != context->targetToLabels.end());
        const std::string & L0 = context->targetToLabels[statement][0];
        const std::string & L1 = context->targetToLabels[statement][1];

        s += L0 + ":\n";
        
        s += TranslateStatement(context, body);
        
        context->currentIntention.push_back(WANT_VALUE);
        s += TranslateExpression(context, expr);
        context->currentIntention.pop_back();
        s += "cmp al, 0\n";
        s += "jne " + L0 + "\n";

        s += L1 + ":\n";
    }
    else if (statement->type == STMT_FOR)
    {
        ASSERT(statement->down && statement->down->right && statement->down->right->right && statement->down->right->right->right);
        Node * preExpr = statement->down;
        Node * loopExpr = preExpr->right;
        Node * postExpr = loopExpr->right;
        Node * body = postExpr->right;

        ASSERT(context->targetToLabels.find(statement) != context->targetToLabels.end());
        const std::string & L0 = context->targetToLabels[statement][0];
        const std::string & L1 = context->targetToLabels[statement][1];

        if (preExpr->type != EMPTY_EXPRESSION)
        {
            context->currentIntention.push_back(WANT_NOTHING);
            s += TranslateExpression(context, preExpr);
            context->currentIntention.pop_back();
        }

        s += L0 + ":\n";
        context->currentIntention.push_back(WANT_VALUE);
        s += TranslateExpression(context, loopExpr);
        context->currentIntention.pop_back();
        s += "cmp al, 0\n";
        s += "je " + L1 + "\n";

        s += TranslateStatement(context, body);
        if (postExpr->type != EMPTY_EXPRESSION)
        {
            context->currentIntention.push_back(WANT_NOTHING);
            s += TranslateExpression(context, postExpr);
            context->currentIntention.pop_back();
        }

        s += "jmp " + L0 + "\n";
        s += L1 + ":\n";
    }
    else if (statement->type == STMT_CONTINUE ||
             statement->type == STMT_BREAK)
    {
        ASSERT(context->nodeToTarget.find(statement) != context->nodeToTarget.end());
        Node * targetNode   = context->nodeToTarget[statement].first;
        int labelIndex      = context->nodeToTarget[statement].second;

        const std::string & label = context->targetToLabels[targetNode][labelIndex];

        s += "jmp " + label + "\n";
    }
    else if (statement->type == STMT_RETURN)
    {
        s += TranslateReturnStatement(context, statement);
    }
    else if (statement->type == STMT_GOTO)
    {
        s += "jmp " + statement->stmt.label->toString() + "\n";
    }
    else if (statement->type == STMT_LABEL)
    {
        s += statement->stmt.label->toString() + ":\n";
    }
    else if (statement->type == STMT_SWITCH)
    {
        // ... expr code ...
        // cmp rax, case1-value
        // je case1-label
        // cmp rax, case2-value
        // je case2-label
        // ...
        // jmp default-label/end
        // ... body code ...
        // end-label:

        ASSERT(statement->down && statement->down->right);
        Node * expr = statement->down;
        Node * body = expr->right;

        context->currentIntention.push_back(WANT_VALUE);
        s += TranslateExpression(context, expr);
        context->currentIntention.pop_back();

        const std::vector<std::string> & labels     = context->targetToLabels[statement];
        const std::vector<Node *> & defaultAndCases = context->switchToChildren[statement];
        for (size_t i = 1; i < defaultAndCases.size(); ++i)
        {
            u64 caseValue = defaultAndCases[i]->stmt.caseValue;
            s += "cmp rax, " + std::to_string(caseValue) + "\n";
            s += "je " + labels[i + 2];
        }

        s += "jmp " + labels[0] + "\n";

        s += TranslateStatement(context, body);

        s += labels[1] + ":\n";
    }
    else if (statement->type == STMT_CASE ||
             statement->type == STMT_DEFAULT)
    {
        ASSERT(context->nodeToTarget.find(statement) != context->nodeToTarget.end());
        Node * targetNode = context->nodeToTarget[statement].first;
        int labelIndex = context->nodeToTarget[statement].second;

        const std::string & label = context->targetToLabels[targetNode][labelIndex];

        s += label + ":\n";
    }
    else
    {
        ASSERT(false);
    }

    return s;
}

void TranslateFunctionContext(x64Program * program,
                              FunctionContext * context)
{
    std::string & text = program->textSegment;

    // <label> PROC
    text += context->functionName + " PROC\n";

    // prolog: save arg, save reg, alloc stack
    PrepareStack(context);
    text += "push rbp\n";
    text += "mov rbp, rsp\n";
    text += "push rsi\n";
    text += "push rdi\n";
    text += "sub rsp, " + IntegerToHexString(context->stackAllocSize) + "\n";

    // body (including epilog)
    text += TranslateStatement(context, context->functionBody);

    // <label> ENDP
    text += context->functionName + " ENDP\n";
}

x64Program Translate(DefinitionContext * definitionContext,
                     ConstantContext * constantContext,
                     std::vector<FunctionContext *> & functionContexts)
{
    x64Program program;

    // Extract data info: definition -> data segment
    TranslateDefinitionContext(&program, definitionContext);

    // Extract data info: constant -> data segment
    TranslateConstantContext(&program, constantContext);

    // function -> proc
    for (FunctionContext * functionContext : functionContexts)
    {
        TranslateFunctionContext(&program, functionContext);
    }

    return program;
}

std::string GetProgram(const x64Program & program)
{
    std::string s;

    for (auto & publicLabel : program.publicLabels)
    {
        s += "PUBLIC " + publicLabel + "\n";
    }
    for (auto & externLabel : program.externLabels)
    {
        s += "EXTERN " + externLabel + "\n";
    }
    if (!program.constSegment.empty())
    {
        s += "CONST SEGMENT\n" +
            program.constSegment +
            "CONST ENDS\n";
    }
    if (!program.bssSegment.empty())
    {
        s += "_BSS SEGMENT\n" +
            program.bssSegment +
            "_BSS ENDS\n";
    }
    if (!program.dataSegment.empty())
    {
        s += "_DATA SEGMENT\n" +
            program.dataSegment +
            "_DATA ENDS\n";
    }
    if (!program.textSegment.empty())
    {
        s += "_TEXT SEGMENT\n" +
            program.textSegment +
            "_TEXT ENDS\n";
    }

    s += "END\n\n";

    return s;
}

void PrintProgram(x64Program * program)
{
    for (auto & publicLabel : program->publicLabels)
    {
        std::cout << "PUBLIC " << publicLabel << std::endl;
    }
    for (auto & externLabel : program->externLabels)
    {
        std::cout << "EXTERN " << externLabel << std::endl;
    }
    if (!program->constSegment.empty())
    {
        std::cout
            << "CONST SEGMENT" << std::endl
            << program->constSegment
            << "CONST ENDS" << std::endl;
    }
    if (!program->bssSegment.empty())
    {
        std::cout
            << "_BSS SEGMENT" << std::endl
            << program->bssSegment
            << "_BSS ENDS" << std::endl;
    }
    if (!program->dataSegment.empty())
    {
        std::cout
            << "_DATA SEGMENT" << std::endl
            << program->dataSegment
            << "_DATA ENDS" << std::endl;
    }
    if (!program->textSegment.empty())
    {
        std::cout
            << "_TEXT SEGMENT" << std::endl
            << program->textSegment
            << "_TEXT ENDS" << std::endl;
    }
    std::cout << "END" << std::endl;
}

}