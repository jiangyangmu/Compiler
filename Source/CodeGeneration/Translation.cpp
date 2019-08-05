#include "Translation.h"

#include <iostream>

#include "../IR/CallingConvention.h"
#include "../Base/Bit.h"

namespace Language
{

// Stack layout

#define R12_MASK (1)
#define R13_MASK (1 << 1)
#define R14_MASK (1 << 2)
#define R15_MASK (1 << 3)
#define RDI_MASK (1 << 4)
#define RSI_MASK (1 << 5)
#define RBX_MASK (1 << 6)
#define RBP_MASK (1 << 7)
#define RSP_MASK (1 << 8)
#define XMM6_MASK (1 << 9)
#define XMM7_MASK (1 << 10)
#define XMM8_MASK (1 << 11)
#define XMM9_MASK (1 << 12)
#define XMM10_MASK (1 << 13)
#define XMM11_MASK (1 << 14)
#define XMM12_MASK (1 << 15)
#define XMM13_MASK (1 << 16)
#define XMM14_MASK (1 << 17)
#define XMM15_MASK (1 << 18)

x64StackLayout * CreateStackLayout()
{
    x64StackLayout * stackLayout = new x64StackLayout;

    stackLayout->localZoneSize = 0;
    stackLayout->maxTempZoneSize = 0;
    stackLayout->maxSpillZoneSize = 0;
    stackLayout->maxCallZoneSize = 0;
    stackLayout->stackAllocSize = 0;
    stackLayout->stackFrameSize = 0;
    stackLayout->registerZoneEndOffset = 0;
    stackLayout->localZoneEndOffset = 0;
    stackLayout->tempZoneBeginOffset = 0;
    stackLayout->spillZoneEndOffset = 0;
    stackLayout->callZoneEndOffset = 0;
    stackLayout->dirtyRegisters = 0;

    return stackLayout;
}

size_t ComputeCallZoneSize(Node * call)
{
    ASSERT(call->down);

    size_t size = 0;

    // int/float/block(<=64)    -> register/value-on-stack
    // block(>64)               -> address-on-stack
    size += (CountChild(call) - 1) * 8;

    // large return value
    Type * returnType = call->down->expr.type;
    if (TypeSize(returnType) > 8)
    {
        size += 8;
    }

    return size;
}

void GetAllLocalObjectsInDefinitionContext(DefinitionContext * context,
                                           std::vector<ObjectDefinition *> & objects,
                                           bool isTopLevel = true)
{
    ASSERT(context);

    for (Definition * definition : context->definitions[ID_NAMESPACE])
    {
        if (definition->type == OBJECT_DEFINITION)
        {
            ObjectDefinition * objectDefinition = AsObjectDefinition(definition);
            if (objectDefinition->objStorageType == LOCAL_OBJECT)
            {
                objects.push_back(objectDefinition);
            }
        }
    }

    if (context->firstChild)
        GetAllLocalObjectsInDefinitionContext(context->firstChild, objects, false);

    if (!isTopLevel && context->next)
        GetAllLocalObjectsInDefinitionContext(context->next, objects, false);
}

void ForExpressionTreeInFunctionBody(FunctionContext * context,
                                     Node * body,
                                     x64StackLayout * stackLayout,
                                     void(*func)(x64StackLayout *, Node *))
{
    ASSERT(body);
    if (body->type > BEGIN_EXPRESSION && body->type < END_EXPRESSION)
    {
        (*func)(stackLayout, body);
    }
    else
    {
        for (Node * child = body->down;
             child;
             child = child->right)
        {
            ForExpressionTreeInFunctionBody(context, child, stackLayout, func);
        }
    }
}

bool IsRegisterNode(Node * node)
{
    ASSERT(node);
    return node->type == EXPR_PIND || node->type == EXPR_CALL;
}

void UpdateMaxTempZoneSpillZoneCallZone(x64StackLayout * stackLayout, Node * exprTree)
{
    size_t tempZoneSize = 0;
    size_t callZoneSize = 0;
    size_t maxSpillRegisters = 0;

    // post-order
    std::vector<std::pair<Node *, bool>> st = { { exprTree, false } };
    while (!st.empty())
    {
        Node * node = st.back().first;
        if (!st.back().second)
        {
            st.back().second = true;

            std::vector<Node *> children;
            for (Node * child = node->down; child; child = child->right)
            {
                children.push_back(child);
            }
            for (auto it = children.rbegin(); it != children.rend(); ++it)
            {
                st.push_back({ *it, false });
            }
        }
        else
        {
            // compute tempZoneSize
            if (node->expr.loc.type == NEED_ALLOC)
            {
                size_t align = TypeAlignment(node->expr.type);
                size_t size = TypeSize(node->expr.type);

                ASSERT(align <= 8);

                tempZoneSize = (tempZoneSize + size + align - 1) / align * align;
            }
            // compute callZoneSize
            if (node->type == EXPR_CALL)
            {
                size_t newCallZoneSize = ComputeCallZoneSize(node);

                callZoneSize = callZoneSize < newCallZoneSize
                    ? newCallZoneSize
                    : callZoneSize;
            }
            // compute maxSpillRegisters
            {
                size_t spillRegisters = 0;
                for (Node * child = node->down;
                     child;
                     child = child->right)
                {
                    if (IsRegisterNode(child))
                        ++spillRegisters;
                }
                maxSpillRegisters = Max(maxSpillRegisters, spillRegisters);
            }

            st.pop_back();
        }
    }

    stackLayout->maxTempZoneSize = Max(stackLayout->maxTempZoneSize, tempZoneSize);
    stackLayout->maxSpillZoneSize = Max(stackLayout->maxSpillZoneSize, maxSpillRegisters * 8);
    stackLayout->maxCallZoneSize = Max(stackLayout->maxCallZoneSize, callZoneSize);
}

void FillLocalAndTempLocationInExpressionTree(x64StackLayout * stackLayout, Node * exprTree)
{
    size_t offset = stackLayout->tempZoneBeginOffset;

    // post-order
    std::vector<std::pair<Node *, bool>> st = { { exprTree, false } };
    while (!st.empty())
    {
        Node * node = st.back().first;
        if (!st.back().second)
        {
            st.back().second = true;

            std::vector<Node *> children;
            for (Node * child = node->down; child; child = child->right)
            {
                children.push_back(child);
            }
            for (auto it = children.rbegin(); it != children.rend(); ++it)
            {
                st.push_back({ *it, false });
            }
        }
        else
        {
            // fill location
            if (node->expr.loc.type == NEED_ALLOC)
            {
                size_t align = TypeAlignment(node->expr.type);
                size_t size = TypeSize(node->expr.type);

                ASSERT(align <= 8);

                offset = (offset + size + align - 1) / align * align;

                node->expr.loc.type = ESP_OFFSET;
                node->expr.loc.offsetValue = stackLayout->stackFrameSize - offset;
            }
            else if (node->expr.loc.type == SAME_AS_FIRST_CHILD)
            {
                node->expr.loc = node->down->expr.loc;
            }
            else if (node->expr.loc.type == SAME_AS_FIRST_GRANDCHILD)
            {
                node->expr.loc = node->down->down->expr.loc;
            }
            else if (node->expr.loc.type == SEARCH_LOCAL_DEFINITION_TABLE)
            {
                node->expr.loc.type = ESP_OFFSET;
                node->expr.loc.offsetValue =
                    stackLayout->stackFrameSize - stackLayout->localObjectOffsets.at(node->expr.loc.definitionValue);
            }

            st.pop_back();
        }
    }

    ASSERT(offset <= (stackLayout->tempZoneBeginOffset + stackLayout->maxTempZoneSize));
}

Location GetSpillLocation(x64StackLayout * stackLayout, size_t i)
{
    ASSERT(stackLayout->stackFrameSize != 0);
    ASSERT((i * 8) < stackLayout->maxSpillZoneSize);
    Location location;
    location.type = ESP_OFFSET;
    location.offsetValue = stackLayout->stackFrameSize - stackLayout->spillZoneEndOffset + i * 8;
    return location;
}

x64StackLayout * PrepareStack(FunctionContext * context)
{
    // step:
    //      1. scan for non-volatile registers, fill {dirtyRegisters, registerZoneEndOffset}
    //      2. scan for local objects, fill {localZoneSize, localZoneEndOffset, localObjectOffsets}
    //      3. scan for temp objects, fill {maxTempZoneSize, tempZoneBeginOffset}
    //      4. scan for spill nodes, fill {maxSpillZoneSize, spillZoneEndOffset}
    //      5. scan for call nodes, fill {maxCallZoneSize, callZoneEndOffset}
    //      6. fill {spillZoneEndOffset}
    //      7. fill {stackFrameSize, stackAllocSize}
    //      8. fill {local location in expression tree, temp location in expression tree}
    //          * both depends on stackFrameSize

    x64StackLayout * stackLayout = CreateStackLayout();

    // "offset to previous stack frame"
    size_t offset = 8;

    // saved rbp, non-volatile registers
    stackLayout->dirtyRegisters = RBP_MASK | RSI_MASK | RDI_MASK;
    offset = stackLayout->registerZoneEndOffset = offset + 8 * CountBits(stackLayout->dirtyRegisters);

    // local zone
    std::vector<ObjectDefinition *> objectDefinitions;
    GetAllLocalObjectsInDefinitionContext(context->functionDefinitionContext, objectDefinitions);
    for (ObjectDefinition * objectDefinition : objectDefinitions)
    {
        size_t objectSize = TypeSize(objectDefinition->objType);
        size_t objectAlign = TypeAlignment(objectDefinition->objType);
        stackLayout->localZoneSize = (stackLayout->localZoneSize + objectSize + objectAlign - 1) / objectAlign * objectAlign;
        stackLayout->localObjectOffsets.emplace(&objectDefinition->def, offset + stackLayout->localZoneSize);
    }
    offset = stackLayout->localZoneEndOffset = offset + stackLayout->localZoneSize;

    // gap
    offset = (offset + 7) / 8 * 8;

    ForExpressionTreeInFunctionBody(context,
                                    context->functionBody,
                                    stackLayout,
                                    UpdateMaxTempZoneSpillZoneCallZone);

    // temp zone
    stackLayout->tempZoneBeginOffset = offset;
    offset += stackLayout->maxTempZoneSize;

    // gap
    offset = (offset + 7) / 8 * 8;

    // spill zone
    stackLayout->maxSpillZoneSize += 8; // for tmpLoc
    offset = stackLayout->spillZoneEndOffset = offset + stackLayout->maxSpillZoneSize;

    // gap + call zone
    offset = stackLayout->callZoneEndOffset = (offset + stackLayout->maxCallZoneSize + 15) / 16 * 16;

    // stack layout
    stackLayout->stackFrameSize = offset;
    stackLayout->stackAllocSize = offset - stackLayout->registerZoneEndOffset;

    // fill expression node location
    ForExpressionTreeInFunctionBody(context,
                                    context->functionBody,
                                    stackLayout,
                                    FillLocalAndTempLocationInExpressionTree);

    return stackLayout;
}

std::string StackLayoutDebugString(x64StackLayout * stackLayout)
{
    ASSERT(stackLayout->stackFrameSize > 0);

    std::string s;

    s = std::string() +
        "; return address:        [rbp + 8]\n" +
        "; non-volatile register: [rsp + " + std::to_string(stackLayout->stackFrameSize - stackLayout->registerZoneEndOffset) + ", rbp + 8)\n" +
        "; local zone:            [rsp + " + std::to_string(stackLayout->stackFrameSize - stackLayout->localZoneEndOffset) + ", rsp + " + std::to_string(stackLayout->stackFrameSize - stackLayout->localZoneEndOffset + stackLayout->localZoneSize) + ")\n" +
        "; temp  zone:            [rsp + " + std::to_string(stackLayout->stackFrameSize - stackLayout->tempZoneBeginOffset - stackLayout->maxTempZoneSize) + ", rsp + " + std::to_string(stackLayout->stackFrameSize - stackLayout->tempZoneBeginOffset) + ")\n" +
        "; spill zone:            [rsp + " + std::to_string(stackLayout->stackFrameSize - stackLayout->spillZoneEndOffset) + ", rsp + " + std::to_string(stackLayout->stackFrameSize - stackLayout->spillZoneEndOffset + stackLayout->maxSpillZoneSize) + ")\n" +
        "; call  zone:            [rsp + " + std::to_string(stackLayout->stackFrameSize - stackLayout->callZoneEndOffset) + ", rsp + " + std::to_string(stackLayout->stackFrameSize - stackLayout->callZoneEndOffset + stackLayout->maxCallZoneSize) + ")\n";

    std::map<size_t, StringRef> localVariables;
    for (auto kv : stackLayout->localObjectOffsets)
    {
        localVariables.emplace(kv.second, kv.first->name);
    }
    for (auto it = localVariables.rbegin(); it != localVariables.rend(); ++it)
    {
        s += "; " + it->second.toString() + ": [rsp + " + std::to_string(stackLayout->stackFrameSize - it->first) + "]\n";
    }

    return s;
}

// Instruction helper

std::string RegisterTypeToString(RegisterType type, size_t width);
std::string X64LocationString(Location loc, size_t width);

struct Code
{
    x64Program * program;

    static std::string LEA(RegisterType reg, Location loc)
    {
        // 1. lea rax (return value address), loc (return loc)
        // 2. lea rax, loc (mdup)

        ASSERT(loc.type != REGISTER_INDIRECT);
        ASSERT(reg == RAX);

        return "lea " +
            RegisterTypeToString(reg, 8) + ", " +
            X64LocationString(loc, 8) + "\n";
    }

    static std::string LEA_RCX(RegisterType reg, Location loc)
    {
        // 1. lea rax (argument address), [rcx]/loc (argument loc)
        // 2. lea rax, [rcx]/loc (maddr)
        // 3. lea rax/rcx, [rcx]/loc (mcopy)
        // 4. lea rcx, [rcx]/loc (mdup)

        ASSERT(reg == RAX || reg == RCX);

        return "lea " +
            RegisterTypeToString(reg, 8) + ", " +
            X64LocationString(loc, 8) + "\n";
    }

    static std::string MOV1(RegisterType reg, Location loc, size_t width)
    {
        // 3. mov rax, loc (b2i, b2f)

        ASSERT(loc.type != REGISTER_INDIRECT);

        return "mov " +
            RegisterTypeToString(reg, width) + ", " +
            X64LocationString(loc, width) + "\n";
    }

    static std::string MOV1_RCX(RegisterType reg, Location loc, size_t width)
    {
        // 1. mov rax (argument value), [rcx]/loc (argument loc)
        // 2. mov r11, [rcx]/loc (function loc)
        // 3. mov rax, [rcx]/loc (i2i, i2f, i2b)
        // 4. mov rax/rdx/cl, [rcx]/loc (unary int, arith int, shift int, rel int)
        // 5. mov rax, [rcx]/loc (pointer expr value)
        // 6. mov rcx, [rcx]/loc (pval)
        // 7. mov rax, [rcx]/loc (mcopy)
        // 8. mov rax, [rcx]/loc (mdup)
        // 9. mov rax, [rcx]/loc (madd)

        return "mov " +
            RegisterTypeToString(reg, width) + ", " +
            X64LocationString(loc, width) + "\n";
    }

    static std::string MOV2(Location loc, RegisterType reg, size_t width)
    {
        // 1. mov loc (argument loc), rax (argument address)
        // 2. mov loc (argument loc), rax (argument value)
        // 3. mov loc (return   loc), rax (return value)
        // 4. mov loc (return   loc), rax (return value address)
        // 5. mov loc (expr value), rax
        // 6. mov loc, rax (cvt expr result value)
        // 7. mov loc, rax (bool expr result value)
        // 8. mov loc, rax (int expr value)
        // 9. mov loc, rax (pointer expr value)
        // 10. mov loc, rax (maddr)
        // 11. mov loc, rax (mdup)
        // 12. mov loc, rax (madd)

        ASSERT(loc.type != REGISTER_INDIRECT);

        return "mov " +
            X64LocationString(loc, width) + ", " +
            RegisterTypeToString(reg, width) + "\n";
    }

    static std::string MOV3_RCX(Location loc, RegisterType reg, size_t width)
    {
        // 1. mov [rcx]/loc, rax (mcopy)

        ASSERT(reg == RAX);

        return "mov " +
            X64LocationString(loc, width) + ", " +
            RegisterTypeToString(reg, width) + "\n";
    }

    static std::string MOVSX(RegisterType dst, size_t dstWidth, RegisterType src, size_t srcWidth)
    {
        // 1. movsx rax, ax (expr value)

        ASSERT(dstWidth > srcWidth);

        return (srcWidth == 4 ? "movsxd " : "movsx ") +
            RegisterTypeToString(dst, dstWidth) + ", " +
            RegisterTypeToString(src, srcWidth) + "\n";
    }

    static std::string MOVSS1_RCX(RegisterType xmm, Location loc, size_t width)
    {
        // 1. movss xmm, [rcx]/loc (expr value)

        ASSERT(XMM0 <= xmm && xmm <= XMM3);
        ASSERT(width == 4 || width == 8);

        return (width == 4 ? "movss " : "movsd ") +
            RegisterTypeToString(xmm, width) + ", " +
            X64LocationString(loc, width) + "\n";
    }

    static std::string MOVSS2(Location loc, RegisterType xmm, size_t width)
    {
        // 1. movss loc (return loc), xmm (return value)
        // 2. movss loc, xmm (cvt expr result value)
        // 3. movss loc, xmm (float expr result value)

        ASSERT(XMM0 <= xmm && xmm <= XMM3);
        ASSERT(width == 4 || width == 8);

        return (width == 4 ? "movss " : "movsd ") +
            X64LocationString(loc, width) + ", " +
            RegisterTypeToString(xmm, width) + "\n";
    }

    static std::string CALL(Location loc)
    {
        // 1. call label
        // 2. call r11

        ASSERT(loc.type == LABEL ||
               (loc.type == REGISTER && loc.registerType == R11));

        return "call " +
            (loc.type == LABEL ? loc.labelValue->toString() : "r11") + "\n";
    }

    static std::string SUB_RCX(RegisterType reg, Location loc, size_t width)
    {
        // 1. sub reg, loc (bool expr value)
        // 2. sub reg, [rcx]/loc (pointer expr value)

        return "sub " +
            RegisterTypeToString(reg, width) + ", " +
            X64LocationString(loc, width) + "\n";
    }

    static std::string ADD_RCX(RegisterType reg, Location loc, size_t width)
    {
        // 1. add reg, [rcx]/loc (pointer expr value)

        return "add " +
            RegisterTypeToString(reg, width) + ", " +
            X64LocationString(loc, width) + "\n";
    }
};

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
    char hex[19];
    sprintf_s(hex, 19, "0%0.16llxh", i);
    return std::string(hex, hex + 18);
}

std::string IntegerToHexString(int i)
{
    char hex[11];
    sprintf_s(hex, 11, "0%0.8xh", i);
    return std::string(hex, hex + 10);
}

std::string IntegerToHexString(int64_t i)
{
    char hex[19];
    sprintf_s(hex, 19, "0%0.16llxh", i);
    return std::string(hex, hex + 18);
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
    ASSERT(loc.type >= REGISTER && loc.type <= REGISTER_INDIRECT);
    std::string s;
    switch (loc.type)
    {
        case REGISTER:      s = RegisterTypeToString(loc.registerType, width); break;
        case ESP_OFFSET:    s = SizeToX64TypeString(width) + " PTR [rsp + " + std::to_string(loc.offsetValue) + "]"; break;
        case BP_OFFSET:     s = SizeToX64TypeString(width) + " PTR [rbp + " + std::to_string(loc.offsetValue) + "]"; break;
        case LABEL:         s = SizeToX64TypeString(width) + " PTR " + loc.labelValue->toString(); break;
        case INLINE:        s = IntegerToHexString(loc.inlineValue); break;
        case REGISTER_INDIRECT:
                            s = SizeToX64TypeString(width) + " PTR [" + RegisterTypeToString(loc.registerType, 8) + "]"; break;
        default:            break;
    }
    return s;
}

// register mask (only non-volatile registers)

void AssertRegisterMask(x64StackLayout * stackLayout, size_t mask)
{
    ASSERT(CountBits(mask) == 1);
    ASSERT((stackLayout->dirtyRegisters & mask) != 0);
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

    // <label> <align-string> 0<size/align>H DUP (?)
    program->bssSegment +=
        label.toString() + " " + SizeToX64TypeString(align) + " " + IntegerToHexString(size / align) + " DUP (?)\n";
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
    ASSERT(functionDefinition->funcStorageType != FunctionStorageType::FORCE_PRIVATE_FUNCTION);
    if (functionDefinition->funcStorageType == FunctionStorageType::PUBLIC_FUNCTION)
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

std::string X64StringLiteral(const StringRef & stringValue)
{
    std::string s;
    s.reserve(stringValue.size());

    for (size_t i = 1; i + 1 < stringValue.size();)
    {
        char c = stringValue[i];
        if (c == '\\')
        {
            switch (stringValue[i + 1])
            {
                case 'n': s.push_back('\n'); break;
                case '"': s.push_back('"'); break;
                default: ASSERT(false); break;
            }
            i += 2;
        }
        else
        {
            s.push_back(c);
            i += 1;
        }
    }

    return ByteArrayToHexString((void *)s.data(), s.size() + 1);
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
        program->constSegment += stringLabel.toString() + " DB " + X64StringLiteral(stringValue) + " ; " + stringValue.toString() + "\n";
        program->dataSegment += label.toString() + " DQ " + stringLabel.toString() + "\n";
    }

    // float constant   -> <label> DD 0<hex>r ; <value>
    program->constSegment += "ALIGN 16\n";
    for (auto kv : context->hashToFloatConstant)
    {
        const StringRef & label = kv.second.label;
        const float &     value = kv.second.value;
        program->constSegment +=
            label.toString() + " DD " +
            "0" + FloatToHexString(value) + "r," +
            "0" + FloatToHexString(value) + "r," +
            "0" + FloatToHexString(value) + "r," +
            "0" + FloatToHexString(value) + "r" +
            " ; " + std::to_string(value) + "\n";
    }
}

std::string TranslateExpression(FunctionContext * context,
                                x64StackLayout * stackLayout,
                                Node * expression);
// save child (spillIndex)
std::string TranslateChildrenExpressionAndSaveSpill(FunctionContext * context,
                                                    x64StackLayout * stackLayout,
                                                    Node * expression)
{
    // handle eval order, shortcut effect, save register value to spill zone
    std::string s;

    size_t spillIndex = 1; // 0 for tmpLoc
    for (Node * child = expression->down;
         child;
         child = child->right)
    {
        s += TranslateExpression(context, stackLayout, child);
        if (IsRegisterNode(child))
        {
            // mov spillSlot, rcx
            s += "mov " + X64LocationString(GetSpillLocation(stackLayout, spillIndex++), 8) + ", " + CX(8) + " ; save spill\n";
        }
    }

    return s;
}
// load child (spillIndex)
std::string LoadSpillAndGetLocation(x64StackLayout * stackLayout,
                                    Node * expression,
                                    size_t childIndex,
                                    Location * childLocation)
{
    size_t spillIndex = 0;

    size_t i = 0;
    Node * child = expression->down;
    while (true)
    {
        ASSERT(child);

        if (IsRegisterNode(child))
            ++spillIndex;

        if (i == childIndex)
            break;

        ++i;
        child = child->right;
    }

    if (IsRegisterNode(child))
    {
        ASSERT(spillIndex > 0); // 0 for tmpLoc

        childLocation->type = REGISTER_INDIRECT;
        childLocation->registerType = RCX;
        // mov rcx, spillSlot
        return "mov " + CX(8) + ", " + X64LocationString(GetSpillLocation(stackLayout, spillIndex), 8) + " ; load spill\n";
    }
    else
    {
        *childLocation = child->expr.loc;
        ASSERT(childLocation->type != NO_WHERE);
        return "";
    }
}

std::string TranslateCallExpression(x64StackLayout * stackLayout,
                                    Node * expression,
                                    Type * outType,
                                    size_t outSize,
                                    Location outLoc)
{
    // Assume function and arguments are matched, and argument conversion already applied.

    // 1. maybe copy return value as first parameter
    // 2. copy param to reg/stack
    // 3. maybe use register call, issue call
    // 4. copy return value to outLoc

    std::string s;

    FunctionType * functionType = nullptr;
    if (expression->down->down && IsFunction(expression->down->down->expr.type))
    {
        functionType = AsFunction(expression->down->down->expr.type);
    }
    else
    {
        Type * pointerToFuncType = expression->down->expr.type;
        ASSERT(IsPointerToFunction(pointerToFuncType));
        functionType = AsFunction(AsPointer(pointerToFuncType)->target);
    }

    ParameterPassingCallerProtocol callerProtocol(functionType);
    size_t argumentIndex = 0;

    if (callerProtocol.IsReturnValueAddressAsFirstParameter())
    {
        size_t width = 8;

        // lea rax, outLoc
        // mov ..., rax
        s +=
            Code::LEA(RAX, outLoc) +
            Code::MOV2(callerProtocol.GetParameterLocation(argumentIndex, nullptr), RAX, width);

        argumentIndex += 1;
    }

    // 1 to skip function node
    size_t childIndex = 1;
    for (Node * child = expression->down->right;
         child;
         child = child->right)
    {
        Location    paramLoc;
        Type *      paramType = child->expr.type;
        size_t      paramSize = TypeSize(paramType);

        s += LoadSpillAndGetLocation(stackLayout, expression, childIndex, &paramLoc);

        if (callerProtocol.IsParameterPassedByAddress(argumentIndex, paramType))
        {
            size_t width = 8;

            // lea rax, paramLoc
            // mov ..., rax
            s +=
                Code::LEA_RCX(RAX, paramLoc) +
                Code::MOV2(callerProtocol.GetParameterLocation(argumentIndex, paramType), RAX, width);
        }
        else if (callerProtocol.IsParameterPassedByXMM(argumentIndex, paramType))
        {
            size_t width = paramSize;

            // movss xmm, paramLoc
            if (IsMemoryLocation(paramLoc))
            {
                s += Code::MOVSS1_RCX(callerProtocol.GetParameterLocation(argumentIndex, paramType).registerType,
                                      paramLoc,
                                      width);
            }
            else
            {
                Location tmpLoc = GetSpillLocation(stackLayout, 0);
                s +=
                    "mov " + RegisterTypeToString(RAX, width) + ", " + X64LocationString(paramLoc, width) + "\n" +
                    "mov " + X64LocationString(tmpLoc, width) + ", " + RegisterTypeToString(RAX, width) + "\n" +
                    Code::MOVSS1_RCX(callerProtocol.GetParameterLocation(argumentIndex, paramType).registerType, tmpLoc, width);
            }

            if (argumentIndex < 4 && callerProtocol.IsVarArgument(argumentIndex))
            {
                ASSERT(width == 8);
                s += "mov ";
                switch (argumentIndex)
                {
                    case 0: s += "rcx"; break;
                    case 1: s += "rdx"; break;
                    case 2: s += "r8"; break;
                    case 3: s += "r9"; break;
                }
                s += ", " + X64LocationString(paramLoc, width) + "\n";
            }
        }
        else
        {
            size_t width = paramSize;
            // HACK: r8, r9 bug

            Location paramSaveLoc = callerProtocol.GetParameterLocation(argumentIndex, paramType);

            // mov rax, paramLoc
            // mov ..., rax
            if (paramSaveLoc.type == REGISTER && (paramSaveLoc.registerType == R8 || paramSaveLoc.registerType == R9) && width < 8)
            {
                s +=
                    Code::MOV1_RCX(RAX, paramLoc, width) +
                    Code::MOVSX(RAX, 8, RAX, width) +
                    Code::MOV2(paramSaveLoc, RAX, 8);
            }
            else
            {
                s +=
                    Code::MOV1_RCX(RAX, paramLoc, width) +
                    Code::MOV2(paramSaveLoc, RAX, width);
            }
        }

        argumentIndex += 1;
        childIndex += 1;
    }

    Location funcLocation;

    if (expression->down->down && IsFunction(expression->down->down->expr.type))
    {
        funcLocation = expression->down->down->expr.loc;
    }
    else
    {
        s += LoadSpillAndGetLocation(stackLayout, expression, 0, &funcLocation);
    }

    if (funcLocation.type == LocationType::LABEL)
    {
        // call label
        s += Code::CALL(funcLocation);
    }
    else
    {
        // mov r11, inLoc
        // call r11
        s +=
            Code::MOV1_RCX(R11, funcLocation, 8) +
            Code::CALL(funcLocation);
    }

    if (IsIntegral(outType) || IsPointer(outType))
    {
        // mov outLoc, rax
        s += Code::MOV2(outLoc, RAX, outSize);
    }
    else if (IsFloating(outType))
    {
        // movss outLoc, xmm0
        s += Code::MOVSS2(outLoc, XMM0, outSize);
    }
    else if (IsVoid(outType))
    {
        // do nothing
    }
    else
    {
        // TODO: support large return value
        ASSERT(false);
    }

    return s;
}

std::string TranslateExpression(FunctionContext * context,
                                x64StackLayout * stackLayout,
                                Node * expression)
{
    // Input: expression tree (all UAC/promotions/positive already convert to cast nodes)
    // Output: code string
    // side effect: use stack/reg to compute value, store in stack/reg

    ASSERT(expression && expression->type > BEGIN_EXPRESSION && expression->type < END_EXPRESSION);
    std::string s = "; " + NodeDebugString(expression) + "\n";

    if (IsArray(expression->expr.type) || IsFunction(expression->expr.type))
    {
        ASSERT(expression->down == nullptr);
        return s;
    }

    ASSERT(!IsVoid(expression->expr.type) || expression->type == EXPR_CALL);

    Type *      outType = expression->expr.type;
    size_t      outSize = IsVoid(expression->expr.type) ? 0 : TypeSize(outType);
    Location    outLoc = expression->expr.loc;

    Location    tmpLoc = GetSpillLocation(stackLayout, 0);

    // Each expression code block should
    // 1. visit children (eval order, shortcut effect, save register value to spill zone)
    // 2. get and check children loc/type/size
    // 3. generate code
    // 4. mask non-volatile registers

    if (expression->type == EXPR_DATA)
    {
        // do nothing
        ASSERT(IsValidLocation(expression->expr.loc));
    }
    else if (expression->type == EXPR_CALL)
    {
        s += TranslateChildrenExpressionAndSaveSpill(context, stackLayout, expression);

        s += TranslateCallExpression(stackLayout,
                                     expression,
                                     outType,
                                     outSize,
                                     outLoc);
    }
    else if (expression->type == EXPR_CVT_REINTERP)
    {
        s += TranslateChildrenExpressionAndSaveSpill(context, stackLayout, expression);

        // do nothing
    }
    else if (expression->type == EXPR_CVT_SI2SI)
    {
        s += TranslateChildrenExpressionAndSaveSpill(context, stackLayout, expression);

        ASSERT(IsIntegral(outType));

        Location    inLoc = expression->down->expr.loc;
        Type *      inType = expression->down->expr.type;
        size_t      inSize = TypeSize(inType);

        ASSERT(IsIntegral(inType));
        ASSERT(outSize != inSize);

        // large from small -> movsx
        // small from large -> mov
        // equal            -> mov

        if (outSize > inSize)
        {
            // mov ax, inLoc
            // movsx rax, ax
            // mov outLoc, rax
            s +=
                Code::MOV1_RCX(RAX, inLoc, inSize) +
                Code::MOVSX(RAX, outSize, RAX, inSize) +
                Code::MOV2(outLoc, RAX, outSize);
        }
        else
        {
            // mov rax, inLoc
            // mov outLoc, ax
            s +=
                Code::MOV1_RCX(RAX, inLoc, inSize) +
                Code::MOV2(outLoc, RAX, outSize);
        }
    }
    else if (expression->type == EXPR_CVT_F2SI)
    {
        s += TranslateChildrenExpressionAndSaveSpill(context, stackLayout, expression);

        ASSERT(IsIntegral(outType));

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
            s +=
                Code::MOVSS1_RCX(XMM0, inLoc, inSize) +
                "cvtss2si " + AX(inSize) + ", " + XMM(0) + "\n";
            {
                if (outSize > inSize)
                {
                    s += Code::MOVSX(RAX, outSize, RAX, inSize);
                }
                
                s += Code::MOV2(outLoc, RAX, outSize);
            }
        }
        else if (inSize == 8)
        {
            // movsd xmm0, inLoc
            // cvtsd2si rax, xmm0
            // cvt_i2i outLoc, rax
            s +=
                Code::MOVSS1_RCX(XMM0, inLoc, inSize) +
                "cvtsd2si " + AX(inSize) + ", " + XMM(0) + "\n";
            {
                ASSERT(outSize <= inSize);
                s += Code::MOV2(outLoc, RAX, outSize);
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
        s += TranslateChildrenExpressionAndSaveSpill(context, stackLayout, expression);

        ASSERT(IsIntegral(outType));

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
            s +=
                Code::MOV1(RAX, inLoc, inSize) +
                Code::MOVSX(RAX, outSize, RAX, inSize) +
                Code::MOV2(outLoc, RAX, outSize);
        }
        else
        {
            // mov rax, inLoc
            // mov outLoc, rax
            s +=
                Code::MOV1(RAX, inLoc, inSize) +
                Code::MOV2(outLoc, RAX, outSize);
        }
    }
    else if (expression->type == EXPR_CVT_F2F)
    {
        s += TranslateChildrenExpressionAndSaveSpill(context, stackLayout, expression);

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
            s +=
                Code::MOVSS1_RCX(XMM1, inLoc, inSize) +
                "cvtss2sd " + XMM(0) + ", " + XMM(1) + "\n" +
                Code::MOVSS2(outLoc, XMM0, outSize);
        }
        else if (inSize == 8 && outSize == 4)
        {
            // movsd xmm1, inLoc
            // cvtsd2ss xmm0, xmm1
            // movss outLoc, xmm0
            s +=
                Code::MOVSS1_RCX(XMM1, inLoc, inSize) +
                "cvtsd2ss " + XMM(0) + ", " + XMM(1) + "\n" +
                Code::MOVSS2(outLoc, XMM0, outSize);
        }
        else
        {
            // SSE don't support long double
            ASSERT(false);
        }
    }
    else if (expression->type == EXPR_CVT_SI2F)
    {
        s += TranslateChildrenExpressionAndSaveSpill(context, stackLayout, expression);

        ASSERT(IsFloating(outType));

        Location    inLoc = expression->down->expr.loc;
        Type *      inType = expression->down->expr.type;
        size_t      inSize = TypeSize(inType);

        ASSERT(IsIntegral(inType));

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
            s += Code::MOV1_RCX(RAX, inLoc, inSize);
            {
                if (width > inSize)
                {
                    s += Code::MOVSX(RAX, width, RAX, inSize);
                }
            }
            s += "cvtsi2ss " + XMM(0) + ", " + AX(width) + "\n";
            s += Code::MOVSS2(outLoc, XMM0, outSize);
        }
        else if (outSize == 8)
        {
            size_t width = 8;

            // mov ax, inLoc
            // cvt_i2i rax, ax
            // cvtsi2sd xmm0, rax
            // movsd outLoc, xmm0
            s += Code::MOV1_RCX(RAX, inLoc, inSize);
            {
                if (width > inSize)
                {
                    s += Code::MOVSX(RAX, width, RAX, inSize);
                }
            }
            s += "cvtsi2sd " + XMM(0) + ", " + AX(width) + "\n";
            s += Code::MOVSS2(outLoc, XMM0, outSize);
        }
        else
        {
            // SSE don't support long double
            ASSERT(false);
        }
    }
    else if (expression->type == EXPR_CVT_B2F)
    {
        s += TranslateChildrenExpressionAndSaveSpill(context, stackLayout, expression);

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
            s += 
                Code::MOV1(RAX, inLoc, inSize) +
                Code::MOVSX(RAX, outSize, RAX, inSize) +
                "cvtsi2ss " + XMM(0) + ", " + AX(inSize) + "\n" +
                Code::MOVSS2(outLoc, XMM0, outSize);
        }
        else if (outSize == 8)
        {
            // mov ax, inLoc
            // movsx rax, ax
            // cvtsi2sd xmm0, rax
            // movsd outLoc, xmm0
            s +=
                Code::MOV1(RAX, inLoc, inSize) +
                Code::MOVSX(RAX, outSize, RAX, inSize) +
                "cvtsi2sd " + XMM(0) + ", " + AX(inSize) + "\n" +
                Code::MOVSS2(outLoc, XMM0, outSize);
        }
        else
        {
            // SSE don't support long double
            ASSERT(false);
        }
    }
    else if (expression->type == EXPR_CVT_I2B)
    {
        s += TranslateChildrenExpressionAndSaveSpill(context, stackLayout, expression);

        ASSERT(IsBool(outType));

        Location    inLoc = expression->down->expr.loc;
        Type *      inType = expression->down->expr.type;
        size_t      inSize = TypeSize(inType);

        ASSERT(IsIntegral(inType));

        // mov rax, inLoc
        // cmp rax, 0
        // mov rcx, 1
        // cmovne rax, rcx
        // mov outLoc, rax
        s +=
            Code::MOV1_RCX(RAX, inLoc, inSize) +
            "cmp " + AX(inSize) + ", " + IntegerToHexString(0) + "\n" +
            "mov " + CX(2) + ", " + IntegerToHexString(1) + "\n" +
            "cmovne " + AX(2) + ", " + CX(2) + "\n" +
            Code::MOV2(outLoc, RAX, outSize);
    }
    else if (expression->type == EXPR_CVT_F2B)
    {
        s += TranslateChildrenExpressionAndSaveSpill(context, stackLayout, expression);

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
            s +=
                Code::MOVSS1_RCX(XMM0, inLoc, inSize) +
                "movss " + XMM(1) + ", " + X64LocationString(LocateFloat(context->constantContext, 0.0f), inSize) + "\n" +
                "cmpness " + XMM(0) + ", " + XMM(1) + "\n" +
                "movss " + X64LocationString(tmpLoc, inSize) + ", " + XMM(0) + "\n" +
                "mov " + AX(inSize) + ", " + X64LocationString(tmpLoc, inSize) + "\n" +
                Code::MOV2(outLoc, RAX, outSize);
        }
        else if (inSize == 8)
        {
            s +=
                Code::MOVSS1_RCX(XMM0, inLoc, inSize) +
                "movsd " + XMM(1) + ", " + X64LocationString(LocateFloat(context->constantContext, 0.0f), inSize) + "\n" +
                "cmpnesd " + XMM(0) + ", " + XMM(1) + "\n" +
                "movsd " + X64LocationString(tmpLoc, inSize) + ", " + XMM(0) + "\n" +
                "mov " + AX(inSize) + ", " + X64LocationString(tmpLoc, inSize) + "\n" +
                Code::MOV2(outLoc, RAX, outSize);
        }
        else
        {
            // SSE don't support long double
            ASSERT(false);
        }
    }
    else if (expression->type == EXPR_BOOL_NOT)
    {
        s += TranslateChildrenExpressionAndSaveSpill(context, stackLayout, expression);

        ASSERT(IsBool(outType));

        Location    inLoc = expression->down->expr.loc;
        Type *      inType = expression->down->expr.type;
        size_t      inSize = TypeSize(inType);

        ASSERT(IsBool(inType));

        // mov rax, 1
        // sub rax, inLoc
        // mov outloc, rax
        s +=
            "mov " + AX(1) + ", " + IntegerToHexString(1) + "\n" +
            Code::SUB_RCX(RAX, inLoc, 1) +
            Code::MOV2(outLoc, RAX, 1);
    }
    else if (expression->type == EXPR_BOOL_AND ||
             expression->type == EXPR_BOOL_OR)
    {
        s += TranslateChildrenExpressionAndSaveSpill(context, stackLayout, expression);

        ASSERT(IsBool(outType));

        Location    inLoc1 = expression->down->expr.loc;
        Type *      inType1 = expression->down->expr.type;
        size_t      inSize1 = TypeSize(inType1);
        
        Location    inLoc2 = expression->down->right->expr.loc;
        Type *      inType2 = expression->down->right->expr.type;
        size_t      inSize2 = TypeSize(inType2);

        // No need to load spill

        ASSERT(IsBool(inType1) && IsBool(inType2));

        std::string op;
        switch (expression->type)
        {
            case EXPR_BOOL_AND: op = "and "; break;
            case EXPR_BOOL_OR:  op = "or "; break;
            default: ASSERT(false); break;
        }

        ASSERT(inLoc1.type != REGISTER_INDIRECT &&
               inLoc2.type != REGISTER_INDIRECT &&
               outLoc.type != REGISTER_INDIRECT);
        // mov rax, inLoc1
        // op  rax, inLoc2
        // mov outLoc, rax
        s += "mov " + AX(1) + ", " + X64LocationString(inLoc1, 1) + "\n" +
             op     + AX(1) + ", " + X64LocationString(inLoc2, 1) + "\n" +
             "mov " + X64LocationString(outLoc, 1) + ", " + AX(1) + "\n";
    }
    else if (expression->type == EXPR_INOT)
    {
        s += TranslateChildrenExpressionAndSaveSpill(context, stackLayout, expression);

        ASSERT(IsInt(outType) && outSize >= 4);

        Location    inLoc = expression->down->expr.loc;
        Type *      inType = expression->down->expr.type;
        size_t      inSize = TypeSize(inType);

        ASSERT(IsInt(inType) && inSize == outSize);

        size_t      width = outSize;

        ASSERT(width == inSize);

        // mov rax, inLoc
        // op  rax
        // mov outLoc, rax
        s +=
            Code::MOV1_RCX(RAX, inLoc, width) +
            "not " + AX(width) + "\n" +
            Code::MOV2(outLoc, RAX, width);
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
        s += TranslateChildrenExpressionAndSaveSpill(context, stackLayout, expression);

        ASSERT(IsInt(outType));

        Location    inLoc1 = {NO_WHERE}; // may spill, set later
        Type *      inType1 = expression->down->expr.type;
        size_t      inSize1 = TypeSize(inType1);

        Location    inLoc2 = {NO_WHERE}; // may spill, set later
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
            RegisterType resultReg = expression->type == EXPR_IMOD ? RDX : RAX;

            // mov rdx, 0
            // mov rax, inLoc1
            // op  inLoc2
            // mov outLoc, result
            s += "mov " + DX(8) + ", " + IntegerToHexString(0) + "\n";
            s += LoadSpillAndGetLocation(stackLayout, expression, 0, &inLoc1);
            s += Code::MOV1_RCX(RAX, inLoc1, width);
            s += LoadSpillAndGetLocation(stackLayout, expression, 1, &inLoc2);
            if (inLoc2.type == INLINE)
            {
                s += "mov " + CX(width) + ", " + X64LocationString(inLoc2, width) + "\n";
                inLoc2.type = REGISTER;
                inLoc2.registerType = RCX;
            }
            s +=
                op + X64LocationString(inLoc2, width) + "\n" +
                Code::MOV2(outLoc, resultReg, width);
        }
        else
        {
            // mov rax, inLoc1
            // op  rax, inLoc2
            // mov outLoc, rax
            s += LoadSpillAndGetLocation(stackLayout, expression, 0, &inLoc1);
            s += Code::MOV1_RCX(RAX, inLoc1, width);
            s += LoadSpillAndGetLocation(stackLayout, expression, 1, &inLoc2);
            s +=
                op + AX(width) + ", " + X64LocationString(inLoc2, width) + "\n" +
                Code::MOV2(outLoc, RAX, width);
        }
    }
    else if (expression->type == EXPR_ISHL ||
             expression->type == EXPR_ISHR)
    {
        s += TranslateChildrenExpressionAndSaveSpill(context, stackLayout, expression);

        ASSERT(IsInt(outType));

        Location    inLoc1 = {NO_WHERE}; // may spill, set later
        Type *      inType1 = expression->down->expr.type;
        size_t      inSize1 = TypeSize(inType1);

        Location    inLoc2 = {NO_WHERE}; // may spill, set later
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
        // mov al, inLoc2
        // mov <tmp>, al
        // mov rax, inLoc1
        // mov cl, <tmp>
        // op  rax, cl
        // mov outLoc, rax
        s += LoadSpillAndGetLocation(stackLayout, expression, 1, &inLoc2);
        s +=
            Code::MOV1_RCX(RAX, inLoc2, 1) +
            Code::MOV2(tmpLoc, RAX, 1);
        s += LoadSpillAndGetLocation(stackLayout, expression, 0, &inLoc1);
        s +=
            Code::MOV1_RCX(RAX, inLoc1, width) +
            "xor rcx, rcx\n" +
            Code::MOV1(RCX, tmpLoc, 1) +
            op + AX(width) + ", " + CX(1) + "\n" +
            Code::MOV2(outLoc, RAX, width);
    }
    else if (expression->type == EXPR_IEQ ||
             expression->type == EXPR_INE ||
             expression->type == EXPR_ILT ||
             expression->type == EXPR_ILE ||
             expression->type == EXPR_IGE ||
             expression->type == EXPR_IGT)
    {
        s += TranslateChildrenExpressionAndSaveSpill(context, stackLayout, expression);

        ASSERT(IsBool(outType));

        Location    inLoc1 = {NO_WHERE}; // may spill, set later
        Type *      inType1 = expression->down->expr.type;
        size_t      inSize1 = TypeSize(inType1);

        Location    inLoc2 = {NO_WHERE}; // may spill, set later
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
        s += LoadSpillAndGetLocation(stackLayout, expression, 0, &inLoc1);
        s += Code::MOV1_RCX(RAX, inLoc1, width);
        s += LoadSpillAndGetLocation(stackLayout, expression, 1, &inLoc2);
        s +=
            Code::MOV1_RCX(RDX, inLoc2, width) +
            "cmp " + AX(width) + ", " + DX(width) + "\n" +
            "mov " + AX(1) + ", " + IntegerToHexString(0) + "\n" +
            "mov " + CX(2) + ", " + IntegerToHexString(1) + "\n" +
            cmov   + AX(2) + ", " + CX(2) + "\n" +
            Code::MOV2(outLoc, RAX, 1);
    }
    else if (expression->type == EXPR_FNEG)
    {
        s += TranslateChildrenExpressionAndSaveSpill(context, stackLayout, expression);

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
        int flt = 0x80000000;
        s +=
            Code::MOVSS1_RCX(XMM0, inLoc, width) +
            "xorps " + XMM(0) + ", " + X64LocationString(LocateFloat(context->constantContext, *(reinterpret_cast<float *>(&flt))), width) + "\n" +
            Code::MOVSS2(outLoc, XMM0, width);
    }
    else if (expression->type == EXPR_FADD ||
             expression->type == EXPR_FSUB ||
             expression->type == EXPR_FMUL ||
             expression->type == EXPR_FDIV)
    {
        s += TranslateChildrenExpressionAndSaveSpill(context, stackLayout, expression);

        ASSERT(IsFloating(outType) && outSize == 4);

        Location    inLoc1 = {NO_WHERE}; // may spill, set later
        Type *      inType1 = expression->down->expr.type;
        size_t      inSize1 = TypeSize(inType1);

        Location    inLoc2 = {NO_WHERE}; // may spill, set later
        Type *      inType2 = expression->down->right->expr.type;
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
        s += LoadSpillAndGetLocation(stackLayout, expression, 0, &inLoc1);
        s += Code::MOVSS1_RCX(XMM0, inLoc1, width);
        s += LoadSpillAndGetLocation(stackLayout, expression, 1, &inLoc2);
        s +=
            Code::MOVSS1_RCX(XMM1, inLoc2, width) +
            op + XMM(0) + ", " + XMM(1) + "\n" +
            Code::MOVSS2(outLoc, XMM0, width);
    }
    else if (expression->type == EXPR_FEQ ||
             expression->type == EXPR_FNE ||
             expression->type == EXPR_FLT || 
             expression->type == EXPR_FLE || 
             expression->type == EXPR_FGE || 
             expression->type == EXPR_FGT)
    {
        s += TranslateChildrenExpressionAndSaveSpill(context, stackLayout, expression);

        ASSERT(IsBool(outType));

        Location    inLoc1 = {NO_WHERE}; // may spill, set later
        Type *      inType1 = expression->down->expr.type;
        size_t      inSize1 = TypeSize(inType1);

        Location    inLoc2 = {NO_WHERE}; // may spill, set later
        Type *      inType2 = expression->down->right->expr.type;
        size_t      inSize2 = TypeSize(inType2);

        ASSERT(IsFloating(inType1) && IsFloating(inType2));

        size_t      width = inSize1;

        ASSERT(width == inSize2);

        std::string op;
        switch (expression->type)
        {
            case EXPR_FEQ: op = "cmpeqss "; break;
            case EXPR_FNE: op = "cmpneqss "; break;
            case EXPR_FLT: op = "cmpltss "; break;
            case EXPR_FLE: op = "cmpless "; break;
            case EXPR_FGE: op = "cmpnltss "; break;
            case EXPR_FGT: op = "cmpnless "; break;
            default: ASSERT(false); break;
        }

        // movss xmm0, inLoc1
        // movss xmm1, inLoc2
        // op    xmm0, xmm1
        // movss <tmp>, xmm0
        // mov   rax, <tmp>
        // cmp   rax, 0
        // mov   rcx, 1
        // cmovne rax, rcx
        // mov   outLoc, rax
        s += LoadSpillAndGetLocation(stackLayout, expression, 0, &inLoc1);
        s += Code::MOVSS1_RCX(XMM0, inLoc1, width);
        s += LoadSpillAndGetLocation(stackLayout, expression, 1, &inLoc2);
        s +=
            Code::MOVSS1_RCX(XMM1, inLoc2, width) +
            op + XMM(0) + ", " + XMM(1) + "\n" +
            Code::MOVSS2(tmpLoc, XMM0, width) +
            "mov "   + AX(1) + ", " + X64LocationString(tmpLoc, 1) + "\n" +
            "cmp "   + AX(1) + ", " + IntegerToHexString(0) + "\n" +
            "mov "   + CX(2) + ", " + IntegerToHexString(1) + "\n" +
            "cmovne "+ AX(2) + ", " + CX(2) + "\n" +
            "mov "   + X64LocationString(outLoc, 1) + ", " + AX(1) + "\n";
    }
    else if (expression->type == EXPR_PADDSI ||
             expression->type == EXPR_PADDUI ||
             expression->type == EXPR_PSUBSI ||
             expression->type == EXPR_PSUBUI)
    {
        s += TranslateChildrenExpressionAndSaveSpill(context, stackLayout, expression);

        ASSERT(IsPointerToObject(outType));

        Location    inLoc1 = {NO_WHERE}; // may spill, set later
        Type *      inType1 = expression->down->expr.type;
        size_t      inSize1 = TypeSize(inType1);

        Location    inLoc2 = {NO_WHERE}; // may spill, set later
        Type *      inType2 = expression->down->right->expr.type;
        size_t      inSize2 = TypeSize(inType2);

        ASSERT(IsPointerToObject(inType1) && IsIntegral(inType2));

        size_t      width = outSize;

        ASSERT(width == inSize1 && width == inSize2);

        int64_t     shiftStep = (expression->type == EXPR_PADDSI || expression->type == EXPR_PADDUI)
                                ? static_cast<int64_t>(TypeSize(AsPointer(outType)->target))
                                : -static_cast<int64_t>(TypeSize(AsPointer(outType)->target));

        // mov rax, inLoc2
        // mov rcx, shiftStep
        // imul rcx
        // add rax, inLoc1
        // mov outLoc, rax
        s += LoadSpillAndGetLocation(stackLayout, expression, 1, &inLoc2);
        s +=
            Code::MOV1_RCX(RAX, inLoc2, width) +
            "mov " + CX(width) + ", " + IntegerToHexString(shiftStep) + "\n" +
            "imul " + CX(width) + "\n";
        s += LoadSpillAndGetLocation(stackLayout, expression, 0, &inLoc1);
        s +=
            Code::ADD_RCX(RAX, inLoc1, width) +
            Code::MOV2(outLoc, RAX, width);
    }
    else if (expression->type == EXPR_PDIFF)
    {
        s += TranslateChildrenExpressionAndSaveSpill(context, stackLayout, expression);

        ASSERT(IsPtrDiffType(outType));

        Location    inLoc1 = {NO_WHERE}; // may spill, set later
        Type *      inType1 = expression->down->expr.type;
        size_t      inSize1 = TypeSize(inType1);

        Location    inLoc2 = {NO_WHERE}; // may spill, set later
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
        s += LoadSpillAndGetLocation(stackLayout, expression, 0, &inLoc1);
        s += Code::MOV1_RCX(RAX, inLoc1, width);
        s += LoadSpillAndGetLocation(stackLayout, expression, 1, &inLoc2);
        s +=
            Code::SUB_RCX(RAX, inLoc2, width) +
            "mov " + CX(width) + ", " + IntegerToHexString(objSize) + "\n" +
            "div " + CX(width) + "\n" +
            Code::MOV2(outLoc, RAX, width);
    }
    else if (expression->type == EXPR_PIND)
    {
        s += TranslateChildrenExpressionAndSaveSpill(context, stackLayout, expression);

        Location    inLoc = expression->down->expr.loc;
        Type *      inType = expression->down->expr.type;
        size_t      inSize = TypeSize(inType);

        ASSERT(IsPointer(inType));

        // mov rcx, inLoc
        s += Code::MOV1_RCX(RCX, inLoc, inSize);
    }
    else if (expression->type == EXPR_PEQ ||
             expression->type == EXPR_PNE ||
             expression->type == EXPR_PLT ||
             expression->type == EXPR_PLE ||
             expression->type == EXPR_PGE ||
             expression->type == EXPR_PGT)
    {
        s += TranslateChildrenExpressionAndSaveSpill(context, stackLayout, expression);

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
        s += LoadSpillAndGetLocation(stackLayout, expression, 0, &inLoc1);
        s += Code::MOV1_RCX(RAX, inLoc1, width);
        s += LoadSpillAndGetLocation(stackLayout, expression, 1, &inLoc2);
        s +=
            Code::MOV1_RCX(RDX, inLoc2, width) +
            "cmp " + AX(width) + ", " + DX(width) + "\n" +
            "mov " + AX(1) + ", " + IntegerToHexString(0) + "\n" +
            "mov " + CX(2) + ", " + IntegerToHexString(1) + "\n" +
            cmov   + AX(2) + ", " + CX(2) + "\n" +
            Code::MOV2(outLoc, RAX, 1);
    }
    else if (expression->type == EXPR_PNEW)
    {
        s += TranslateChildrenExpressionAndSaveSpill(context, stackLayout, expression);

        ASSERT(IsPointer(outType));

        Location    inLoc = expression->down->expr.loc;
        Type *      inType = expression->down->expr.type;
        size_t      inSize = (IsArray(inType) || IsFunction(inType)) ? 0 : TypeSize(inType);

        size_t      width = outSize;

        // lea rax, inLoc
        // mov outLoc, rax
        s +=
            Code::LEA_RCX(RAX, inLoc) +
            Code::MOV2(outLoc, RAX, width);
    }
    else if (expression->type == EXPR_MCOPY)
    {
        s += TranslateChildrenExpressionAndSaveSpill(context, stackLayout, expression);

        Location    inLoc1 = {NO_WHERE}; // may spill, set later
        Type *      inType1 = expression->down->expr.type;
        size_t      inSize1 = TypeSize(inType1);

        Location    inLoc2 = {NO_WHERE}; // may spill, set later
        Type *      inType2 = expression->down->right->expr.type;
        size_t      inSize2 = TypeSize(inType2);

        ASSERT(TypeEqual(outType, inType1));
        ASSERT(outSize == inSize1 && outSize == inSize2);

        int         objSize = static_cast<int>(inSize1);

        // 1,2,4,8 -> rax
        // *       -> rep movsb

        if (CountBits(objSize) == 1 && objSize <= 8)
        {
            size_t      width = objSize;

            // mov rax, inLoc2
            // mov inLoc1, rax
            s += LoadSpillAndGetLocation(stackLayout, expression, 1, &inLoc2);
            s += Code::MOV1_RCX(RAX, inLoc2, width);
            s += LoadSpillAndGetLocation(stackLayout, expression, 0, &inLoc1);
            s += Code::MOV3_RCX(inLoc1, RAX, width);
        }
        else
        {
            size_t      width = 8;

            // lea rax, inLoc1
            // lea rcx, inLoc2
            // mov rdi, rax
            // mov rsi, rcx
            // mov rcx, objSize
            // rep movsb
            s += LoadSpillAndGetLocation(stackLayout, expression, 0, &inLoc1);
            s += Code::LEA_RCX(RAX, inLoc1);
            s += LoadSpillAndGetLocation(stackLayout, expression, 1, &inLoc2);
            s += Code::LEA_RCX(RCX, inLoc2);
            s +=
                "mov " + DI(width) + ", " + AX(width) + "\n" +
                "mov " + SI(width) + ", " + CX(width) + "\n" +
                "mov " + CX(width) + ", " + IntegerToHexString(objSize) + "\n" +
                "rep movsb" + "\n";

            AssertRegisterMask(stackLayout, RDI_MASK);
            AssertRegisterMask(stackLayout, RSI_MASK);
        }
    }
    else if (expression->type == EXPR_MDUP)
    {
        s += TranslateChildrenExpressionAndSaveSpill(context, stackLayout, expression);

        Location    inLoc = expression->down->expr.loc;
        Type *      inType = expression->down->expr.type;
        size_t      inSize = TypeSize(inType);

        ASSERT(TypeEqual(outType, inType));
        ASSERT(outSize == inSize);

        // 1,2,4,8 -> rax
        // *       -> rep movsb

        if (CountBits(inSize) == 1 && inSize <= 8)
        {
            size_t      width = inSize;

            if (IsXMMLocation(inLoc))
            {
                // movss <tmp>, inLoc
                // mov eax, <tmp>
                // mov outLoc, rax
                s +=
                    Code::MOVSS2(tmpLoc, inLoc.registerType, width) +
                    Code::MOV1(RAX, tmpLoc, width) +
                    Code::MOV2(outLoc, RAX, width);
            }
            else
            {
                // mov rax, inLoc
                // mov outLoc, rax
                s +=
                    Code::MOV1_RCX(RAX, inLoc, width) +
                    Code::MOV2(outLoc, RAX, width);
            }
        }
        else
        {
            size_t      width = 8;

            ASSERT(outLoc.type != REGISTER_INDIRECT);

            // lea rax, outLoc
            // lea rcx, inLoc
            // mov rdi, rax
            // mov rsi, rcx
            // mov rcx, objSize
            // rep movsb
            s +=
                Code::LEA(RAX, outLoc) +
                Code::LEA_RCX(RCX, inLoc) +
                "mov " + DI(width) + ", " + AX(width) + "\n" +
                "mov " + SI(width) + ", " + CX(width) + "\n" +
                "mov " + CX(width) + ", " + IntegerToHexString(inSize) + "\n" +
                "rep movsb" + "\n";

            AssertRegisterMask(stackLayout, RDI_MASK);
            AssertRegisterMask(stackLayout, RSI_MASK);
        }
    }
    else if (expression->type == EXPR_MADDSI ||
             expression->type == EXPR_MADDUI)
    {
        s += TranslateChildrenExpressionAndSaveSpill(context, stackLayout, expression);

        ASSERT(IsPointer(outType));

        Location    inLoc1 = {NO_WHERE}; // may spill, set later
        Type *      inType1 = expression->down->expr.type;
        size_t      inSize1 = TypeSize(inType1);

        Location    inLoc2 = {NO_WHERE}; // may spill, set later
        Type *      inType2 = expression->down->right->expr.type;
        size_t      inSize2 = TypeSize(inType2);

        ASSERT(IsPointer(inType1) && IsInt(inType2));

        size_t      width = outSize;

        ASSERT(width == inSize1 && width == inSize2);

        // mov rax, inLoc1
        // add rax, inLoc2
        // mov outLoc, rax
        s += LoadSpillAndGetLocation(stackLayout, expression, 0, &inLoc1);
        s += Code::MOV1_RCX(RAX, inLoc1, width);
        s += LoadSpillAndGetLocation(stackLayout, expression, 1, &inLoc2);
        s +=
            Code::ADD_RCX(RAX, inLoc2, width) +
            Code::MOV2(outLoc, RAX, width);
    }
    else if (expression->type == EXPR_CONDITION)
    {
        // TODO: impl condition expr
        ASSERT(false);

        s += TranslateChildrenExpressionAndSaveSpill(context, stackLayout, expression);

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

                AssertRegisterMask(stackLayout, RDI_MASK);
                AssertRegisterMask(stackLayout, RSI_MASK);
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

                AssertRegisterMask(stackLayout, RDI_MASK);
                AssertRegisterMask(stackLayout, RSI_MASK);
            }
        }
        s += L1 + ":\n";
    }
    else if (expression->type == EXPR_ELIST)
    {
        s += TranslateChildrenExpressionAndSaveSpill(context, stackLayout, expression);

        Node *      lastChild = LastChild(expression);
        Location    inLoc = lastChild->expr.loc;
        Type *      inType = lastChild->expr.type;
        size_t      inSize = TypeSize(inType);

        ASSERT(TypeEqual(outType, inType));
        ASSERT(outSize == inSize);

        // mdup outLoc, inLoc
        {
            // 1,2,4,8 -> rax
            // *       -> rep movsb

            if (CountBits(inSize) == 1 && inSize <= 8)
            {
                size_t      width = inSize;

                if (IsXMMLocation(inLoc))
                {
                    // movss <tmp>, inLoc
                    // mov eax, <tmp>
                    // mov outLoc, rax
                    s +=
                        Code::MOVSS2(tmpLoc, inLoc.registerType, width) +
                        Code::MOV1(RAX, tmpLoc, width) +
                        Code::MOV2(outLoc, RAX, width);
                }
                else
                {
                    // mov rax, inLoc
                    // mov outLoc, rax
                    s +=
                        Code::MOV1_RCX(RAX, inLoc, width) +
                        Code::MOV2(outLoc, RAX, width);
                }
            }
            else
            {
                size_t      width = 8;

                ASSERT(outLoc.type != REGISTER_INDIRECT);

                // lea rax, outLoc
                // lea rcx, inLoc
                // mov rdi, rax
                // mov rsi, rcx
                // mov rcx, objSize
                // rep movsb
                s +=
                    Code::LEA(RAX, outLoc) +
                    Code::LEA_RCX(RCX, inLoc) +
                    "mov " + DI(width) + ", " + AX(width) + "\n" +
                    "mov " + SI(width) + ", " + CX(width) + "\n" +
                    "mov " + CX(width) + ", " + IntegerToHexString(inSize) + "\n" +
                    "rep movsb" + "\n";

                AssertRegisterMask(stackLayout, RDI_MASK);
                AssertRegisterMask(stackLayout, RSI_MASK);
            }
        }
    }
    else
    {
        std::cout << "Translation: Skip expr " << NodeDebugString(expression) << std::endl;
    }

    return s;
}

std::string TranslateReturnStatement(FunctionContext * context,
                                     x64StackLayout * stackLayout,
                                     Node * returnStatement)
{
    std::string s;

    ASSERT(returnStatement->down);
    Node * expr = returnStatement->down;

    if (expr->type != EMPTY_EXPRESSION)
    {
        s += TranslateExpression(context, stackLayout, expr);
        
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
    s += "add rsp, " + IntegerToHexString(stackLayout->stackAllocSize) + "\n";
    s += "pop rdi\n";
    s += "pop rsi\n";
    s += "pop rbp\n";
    s += "ret\n";

    return s;
}

std::string TranslateStatement(FunctionContext * context,
                               x64StackLayout * stackLayout,
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
            s += TranslateStatement(context, stackLayout, child);
        }
    }
    else if (statement->type == STMT_EXPRESSION)
    {
        ASSERT(statement->down);
        Node * expr = statement->down;
        
        if (expr->type != EMPTY_EXPRESSION)
        {
            s += TranslateExpression(context, stackLayout, expr);
        }
    }
    else if (statement->type == STMT_IF)
    {
        ASSERT(statement->down && statement->down->right);
        Node * expr = statement->down;
        Node * ifBody = expr->right;
        Node * elseBody = ifBody->right;

        s += TranslateExpression(context, stackLayout, expr);

        s += "cmp al, 0\n";
        if (!elseBody)
        {
            std::string L0 = std::string("@L") + std::to_string(context->nextUniqueLabel++);

            s += "je " + L0 + "\n";
            s += TranslateStatement(context, stackLayout, ifBody);
            s += L0 + ":\n";
        }
        else
        {
            std::string L0 = std::string("@L") + std::to_string(context->nextUniqueLabel++);
            std::string L1 = std::string("@L") + std::to_string(context->nextUniqueLabel++);

            s += "je " + L0 + "\n";
            s += TranslateStatement(context, stackLayout, ifBody);
            s += "jmp " + L1 + "\n";
            s += L0 + ":\n";
            s += TranslateStatement(context, stackLayout, elseBody);        
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
        s += TranslateExpression(context, stackLayout, expr);
        s += "cmp al, 0\n";
        s += "je " + L1 + "\n";
        s += TranslateStatement(context, stackLayout, body);
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
        
        s += TranslateStatement(context, stackLayout, body);
        
        s += TranslateExpression(context, stackLayout, expr);
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
            s += TranslateExpression(context, stackLayout, preExpr);
        }

        s += L0 + ":\n";
        s += TranslateExpression(context, stackLayout, loopExpr);
        s += "cmp al, 0\n";
        s += "je " + L1 + "\n";

        s += TranslateStatement(context, stackLayout, body);
        if (postExpr->type != EMPTY_EXPRESSION)
        {
            s += TranslateExpression(context, stackLayout, postExpr);
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
        s += TranslateReturnStatement(context, stackLayout, statement);
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

        s += TranslateExpression(context, stackLayout, expr);

        const std::vector<std::string> & labels     = context->targetToLabels[statement];
        const std::vector<Node *> & defaultAndCases = context->switchToChildren[statement];
        for (size_t i = 1; i < defaultAndCases.size(); ++i)
        {
            u64 caseValue = defaultAndCases[i]->stmt.caseValue;
            s += "cmp rax, " + std::to_string(caseValue) + "\n";
            s += "je " + labels[i + 1];
        }

        s += "jmp " + labels[0] + "\n";

        s += TranslateStatement(context, stackLayout, body);

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
                              FunctionContext * context,
                              x64StackLayout * stackLayout)
{
    std::string & text = program->textSegment;

    // <label> PROC
    text += context->functionName + " PROC\n";

    // prolog: save arg, save reg, alloc stack
    text += StackLayoutDebugString(stackLayout);
    text += "push rbp\n";
    text += "mov rbp, rsp\n";
    {
        ParameterPassingCalleeProtocol calleeProtocol(context->functionType);
        bool isRVA = calleeProtocol.IsReturnValueAddressAsFirstParameter();
        size_t begin = isRVA ? 1 : 0;
        size_t end = Min(4ull, calleeProtocol.ParameterCount());
        for (size_t callstackIndex = begin, argumentIndex = 0;
             callstackIndex < end;
             ++callstackIndex, ++argumentIndex)
        {
            Location argumentLoc = calleeProtocol.GetParameterLocation(callstackIndex);
            if (argumentLoc.type == REGISTER)
            {
                Location argumentSaveLoc;
                argumentSaveLoc.type = BP_OFFSET;
                argumentSaveLoc.offsetValue = 16 + 8 * callstackIndex;
                
                if (calleeProtocol.IsParameterPassedByXMM(callstackIndex))
                {
                    size_t width = TypeSize(context->functionType->memberType[argumentIndex]);
                    ASSERT(width == 4 || width == 8);
                    text += (width == 4 ? "movss " : "movsd ") + X64LocationString(argumentSaveLoc, width) + ", " + X64LocationString(argumentLoc, width) + " ; copy register argument\n";
                }
                else
                {
                    // HACK: r8, r9 bug
                    text += "mov " + X64LocationString(argumentSaveLoc, 8) + ", " + X64LocationString(argumentLoc, 8) + " ; copy register argument\n";
                }
            }
        }
    }
    text += "push rsi\n";
    text += "push rdi\n";
    text += "sub rsp, " + IntegerToHexString(stackLayout->stackAllocSize) + "\n";

    // body (including epilog)
    text += TranslateStatement(context, stackLayout, context->functionBody);

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
    int flt = 0x80000000;
    LocateFloat(constantContext, 0.0f);
    LocateFloat(constantContext, *(reinterpret_cast<float *>(&flt)));
    TranslateConstantContext(&program, constantContext);

    // function -> proc
    for (FunctionContext * functionContext : functionContexts)
    {
        x64StackLayout * stackLayout = PrepareStack(functionContext);
        TranslateFunctionContext(&program, functionContext, stackLayout);
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