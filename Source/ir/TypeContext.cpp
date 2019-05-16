#include "TypeContext.h"
#include "../Util/Common.h"
#include "../Util/Bit.h"

namespace Language {

u64 TypeId(Type * type)
{
    //u64 prop_id = (type->prop & 0xfULL) << 60;
    //return type->baseId | prop_id;
    return type->baseId;
}

VoidType * AsVoid(Type * type)
{
    ASSERT(type->name == VOID);
    return reinterpret_cast<VoidType *>(type);
}
BoolType * AsBool(Type * type)
{
    ASSERT(type->name == BOOL);
    return reinterpret_cast<BoolType *>(type);
}
CharType * AsChar(Type * type)
{
    ASSERT(type->name == CHAR);
    return reinterpret_cast<CharType *>(type);
}
IntType * AsInt(Type * type)
{
    ASSERT(type->name == INT);
    return reinterpret_cast<IntType *>(type);
}
FloatType * AsFloat(Type * type)
{
    ASSERT(type->name == FLOAT);
    return reinterpret_cast<FloatType *>(type);
}
ArrayType * AsArray(Type * type)
{
    ASSERT(type->name == ARRAY);
    return reinterpret_cast<ArrayType *>(type);
}
PointerType * AsPointer(Type * type)
{
    ASSERT(type->name == POINTER);
    return reinterpret_cast<PointerType *>(type);
}
FunctionType * AsFunction(Type * type)
{
    ASSERT(type->name == FUNCTION);
    return reinterpret_cast<FunctionType *>(type);
}
EnumType * AsEnum(Type * type)
{
    ASSERT(type->name == ENUM);
    return reinterpret_cast<EnumType *>(type);
}
StructType * AsStruct(Type * type)
{
    ASSERT(type->name == STRUCT);
    return reinterpret_cast<StructType *>(type);
}
UnionType * AsUnion(Type * type)
{
    ASSERT(type->name == UNION);
    return reinterpret_cast<UnionType *>(type);
}

u64 FunctionProtocolHash(FunctionType * type)
{
    return 0;
}

#define MAX_RESERVED_BASE_ID 10

u64 ChooseTypeBaseId(TypeContext * context, Type * type)
{
    // id = prop id | base id

    // base id (0 ~ 59 bit)
    //  shared: void, bool, char, int, float
    //  unique: enum, struct, union
    //  target id: pointer
    //  target id, size: array
    //  protocol: function

    // prop id (60 ~ 63 bit)
    //  60: incomplete
    //  61: const
    //  62: addressable
    //  63: assignable

    ASSERT(context->nextTypeBaseId > MAX_RESERVED_BASE_ID);

    u64 baseId = 0;
    if (type->name == VOID)
    {
        baseId = 1;
    }
    else if (type->name == BOOL)
    {
        baseId = 2;
    }
    else if (type->name == CHAR)
    {
        baseId = 3;
    }
    else if (type->name == INT)
    {
        // 4,5,6,7
        ASSERT(type->size <= 8 && CountBits(type->size) == 1);
        baseId = 4 + FastLSBIndex(type->size);
    }
    else if (type->name == FLOAT)
    {
        // 8,9,10
        baseId = 8;
        if (type->size == 8) baseId = 9;
        else if (type->size == 10) baseId = 10;
    }
    else if (type->name == ENUM ||
             type->name == STRUCT ||
             type->name == UNION)
    {
        baseId = context->nextTypeBaseId++;
    }
    else if (type->name == POINTER)
    {
        u64 targetTypeId = TypeId(AsPointer(type)->target);

        auto result = context->existingPointTypeIdMap.try_emplace(targetTypeId, context->nextTypeBaseId);
        if (result.second)
            ++context->nextTypeBaseId;

        baseId = result.first->second;
    }
    else if (type->name == ARRAY)
    {
        u64     targetTypeId = TypeId(AsArray(type)->target);
        size_t  arrayLength = AsArray(type)->length;

        auto result = context->existingArrayTypeIdMap.try_emplace(std::make_tuple(targetTypeId, arrayLength), context->nextTypeBaseId);
        if (result.second)
            ++context->nextTypeBaseId;

        baseId = result.first->second;
    }
    else if (type->name == FUNCTION)
    {
        u64 functionProtoHash = FunctionProtocolHash(AsFunction(type));

        auto result = context->existingFunctionTypeIdMap.try_emplace(functionProtoHash, context->nextTypeBaseId);
        if (result.second)
            ++context->nextTypeBaseId;

        baseId = result.first->second;
    }

    ASSERT(baseId > 0);

    return baseId;
}

VoidType * MakeVoid(TypeContext * context)
{
    VoidType * type = new VoidType;
    type->type.name = VOID;
    type->type.prop = 0;
    type->type.size = 0;
    type->type.align = 0;
    type->type.baseId = ChooseTypeBaseId(context, &type->type);
    context->types.push_back(&type->type);
    return type;
}

BoolType * MakeBool(TypeContext * context)
{
    BoolType * type = new BoolType;
    type->type.name = BOOL;
    type->type.prop = TP_IS_INTEGRAL | TP_IS_ARITHMETIC | TP_IS_SCALAR | TP_IS_OBJECT;
    type->type.size = 1;
    type->type.align = 1;
    type->type.baseId = ChooseTypeBaseId(context, &type->type);
    context->types.push_back(&type->type);
    return type;
}

CharType * MakeChar(TypeContext * context)
{
    CharType * type = new CharType;
    type->type.name = CHAR;
    type->type.prop = TP_IS_INTEGRAL | TP_IS_ARITHMETIC | TP_IS_SCALAR | TP_IS_OBJECT;
    type->type.size = 1;
    type->type.align = 1;
    type->type.baseId = ChooseTypeBaseId(context, &type->type);
    context->types.push_back(&type->type);
    return type;
}

IntType * MakeInt(TypeContext * context, size_t width, bool isSigned)
{
    ASSERT(width <= 8 && CountBits(width) == 1);
    IntType * type = new IntType;
    type->type.name = INT;
    type->type.prop = TP_IS_INTEGRAL | TP_IS_ARITHMETIC | TP_IS_SCALAR | TP_IS_OBJECT;
    type->type.size = width;
    type->type.align = width;
    type->type.baseId = ChooseTypeBaseId(context, &type->type);
    type->isSigned = isSigned;
    context->types.push_back(&type->type);
    return type;
}

FloatType * MakeFloat(TypeContext * context, size_t width)
{
    ASSERT(width == 4 || width == 8 || width == 10);

    FloatType * type = new FloatType;
    type->type.name = FLOAT;
    type->type.prop = TP_IS_ARITHMETIC | TP_IS_SCALAR | TP_IS_OBJECT;
    type->type.size = width;
    type->type.align = width == 10 ? 16 : width;
    type->type.baseId = ChooseTypeBaseId(context, &type->type);
    context->types.push_back(&type->type);
    return type;
}

// OPTIMIZE: add type id support for array
ArrayType * MakeArray(TypeContext * context, size_t length)
{
    ArrayType * type = new ArrayType;
    type->type.name = ARRAY;
    type->type.prop = TP_IS_OBJECT;
    type->type.size = 0; // to fill
    type->type.align = 0; // to fill
    type->type.baseId = 0; // to fill
    type->length = length;
    type->target = nullptr; // to fill
    context->types.push_back(&type->type);
    return type;
}

void ArraySetTarget(TypeContext * context, ArrayType * type, Type * target)
{
    type->target = target;
    type->type.size = type->length * target->size;
    type->type.align = target->align;
    type->type.baseId = 0; // ChooseTypeBaseId(context, &type->type);
}

// OPTIMIZE: add type id support for pointer
PointerType * MakePointer(TypeContext * context)
{
    PointerType * type = new PointerType;
    type->type.name = POINTER;
    type->type.prop = TP_IS_SCALAR | TP_IS_OBJECT;
    type->type.size = 8;
    type->type.align = 8;
    type->type.baseId = 0; // to fill
    type->target = nullptr; // to fill
    context->types.push_back(&type->type);
    return type;
}

void PointerSetTarget(TypeContext * context, PointerType * type, Type * target)
{
    type->target = target;
    type->type.baseId = 0; // ChooseTypeBaseId(context, &type->type);
}

PointerType * MakePointer(TypeContext * context, Type * target)
{
    PointerType * type = new PointerType;
    type->type.name = POINTER;
    type->type.prop = TP_IS_SCALAR | TP_IS_OBJECT;
    type->type.size = 8;
    type->type.align = 8;
    type->target = target;
    type->type.baseId = 0; // ChooseTypeBaseId(context, &type->type);
    context->types.push_back(&type->type);
    return type;
}

// OPTIMIZE: add type id support for function
FunctionType * MakeFunction(TypeContext * context)
{
    FunctionType * type = new FunctionType;
    type->type.name = FUNCTION;
    type->type.prop = 0;
    type->type.size = 0;
    type->type.align = 0;
    type->type.baseId = 0; // to fill
    type->target = nullptr; // to fill
    for (int i = 0; i < 10; ++i) { type->memberType[i] = nullptr; } // 2 to fill
    type->isVarList = false; // to fill
    context->types.push_back(&type->type);
    return type;
}

void FunctionSetTarget(FunctionType * type, Type * target)
{
    type->target = target;
}

void FunctionAddParameter(FunctionType * type, StringRef pname, Type * ptype)
{
    int i = 0;
    while (type->memberType[i] != nullptr) { ++i; ASSERT(i < 10); }
    type->memberName[i] = pname;
    ASSERT(ptype != nullptr);
    type->memberType[i] = ptype;
}

void FunctionSetVarList(FunctionType * type)
{
    type->isVarList = true;
}

void FunctionDone(TypeContext * context, FunctionType * type)
{
    type->type.baseId = 0; // ChooseTypeBaseId(context, &type->type);
}

EnumType * MakeEnum(TypeContext * context)
{
    EnumType * type = new EnumType;
    type->type.name = ENUM;
    type->type.prop = TP_IS_INTEGRAL | TP_IS_ARITHMETIC | TP_IS_SCALAR | TP_IS_OBJECT;
    type->type.size = 4;
    type->type.align = 4;
    type->type.baseId = ChooseTypeBaseId(context, &type->type);
    // 2 to fill
    context->types.push_back(&type->type);
    return type;
}

void EnumAddConst(EnumType * type, StringRef name, int value)
{
    int i = 0;
    while (!type->memberName[i].empty()) { ++i; ASSERT(i < 10); }
    type->memberName[i] = name;
    type->memberValue[i] = value;
}

void EnumChangeLastConstValue(EnumType * type, int value)
{
    int i = 0;
    while (!type->memberName[i].empty()) { ++i; ASSERT(i < 10); }
    type->memberValue[i] = value;
}

StructType * MakeStruct(TypeContext * context)
{
    StructType * type = new StructType;
    type->type.name = STRUCT;
    type->type.prop = TP_IS_OBJECT | TP_INCOMPLETE;
    type->type.size = 0; // to fill
    type->type.align = 0; // to fill
    type->type.baseId = ChooseTypeBaseId(context, &type->type);
    for (int i = 0; i < 10; ++i) { type->memberType[i] = nullptr; } // 3 to fill
    context->types.push_back(&type->type);
    return type;
}

void StructAddMember(StructType * type, StringRef mname, Type * mtype)
{
    ASSERT(IsIncomplete(&type->type));
    int i = 0;
    while (type->memberType[i] != nullptr) { ++i; ASSERT(i < 10); }
    type->memberName[i] = mname;
    type->memberType[i] = mtype;
    type->memberOffset[i] =
        (i == 0)
        ? 0
        : (((type->memberOffset[i - 1] + type->memberType[i - 1]->size) + type->memberType[i - 1]->align - 1) / type->memberType[i - 1]->align * type->memberType[i - 1]->align);
    type->type.align = Max(type->type.align, mtype->align);
    type->type.size = ((type->memberOffset[i] + mtype->size) + type->type.align - 1) / type->type.align * type->type.align;
}

void StructDone(StructType * type)
{
    ASSERT(IsIncomplete(&type->type));
    type->type.prop &= ~TP_INCOMPLETE;
}

UnionType * MakeUnion(TypeContext * context)
{
    UnionType * type = new UnionType;
    type->type.name = UNION;
    type->type.prop = TP_IS_OBJECT;
    type->type.size = 0; // to fill
    type->type.align = 0; // to fill
    type->type.baseId = ChooseTypeBaseId(context, &type->type);
    for (int i = 0; i < 10; ++i) { type->memberType[i] = nullptr; } // 2 to fill
    context->types.push_back(&type->type);
    return type;
}

void UnionAddMember(UnionType * type, StringRef mname, Type * mtype)
{
    int i = 0;
    while (type->memberType[i] != nullptr) { ++i; ASSERT(i < 10); }
    type->memberName[i] = mname;
    type->memberType[i] = mtype;
    type->type.align = Max(type->type.align, mtype->align);
    type->type.size = Max(type->type.size, mtype->size);
}

Type * CloneType(TypeContext * context, Type * type)
{
    Type * clone = nullptr;
    switch (type->name)
    {
        case VOID:      clone = &(new VoidType(*AsVoid(type)))->type; break;
        case BOOL:      clone = &(new BoolType(*AsBool(type)))->type; break;
        case CHAR:      clone = &(new CharType(*AsChar(type)))->type; break;
        case INT:       clone = &(new IntType(*AsInt(type)))->type; break;
        case FLOAT:     clone = &(new FloatType(*AsFloat(type)))->type; break;
        case ARRAY:     clone = &(new ArrayType(*AsArray(type)))->type; break;
        case POINTER:   clone = &(new PointerType(*AsPointer(type)))->type; break;
        case FUNCTION:  clone = &(new FunctionType(*AsFunction(type)))->type; break;
        case ENUM:      clone = &(new EnumType(*AsEnum(type)))->type; break;
        case STRUCT:    clone = &(new StructType(*AsStruct(type)))->type; break;
        case UNION:     clone = &(new UnionType(*AsUnion(type)))->type; break;
        default:        ASSERT(false); break;
    }
    return clone;
}

void SetTargetType(TypeContext * context, Type * type, Type * target)
{
    ASSERT(type->name == ARRAY ||
           type->name == POINTER ||
           type->name == FUNCTION);

    if (type->name == ARRAY)
    {
        ArraySetTarget(context, AsArray(type), target);
    }
    else if (type->name = POINTER)
    {
        PointerSetTarget(context, AsPointer(type), target);
    }
    else
    {
        FunctionSetTarget(AsFunction(type), target);
        FunctionDone(context, AsFunction(type));
    }
}

void TypeSetProperties(Type * type, int prop)
{
    ASSERT((prop & ~0xf) == 0);
    type->prop |= prop;
}

void TypeSetAssignable(Type * type)
{
    type->prop |= TP_ASSIGNABLE;
}
void TypeSetAddressable(Type * type)
{
    type->prop |= TP_ADDRESSABLE;
}

Type * IntegralPromotion(TypeContext * context, Type * type)
{
    // void/array/pointer/function/struct/union -> ERROR
    // bool/char = int-s1
    // enum = int-s4
    // int-s1/int-s2                            -> int-s4
    // int-u1/int-u2                            -> int-u4
    // int-s*                                   -> int-s* (* >= 4)
    // int-u*                                   -> int-u* (* >= 4)

    ASSERT(IsIntegral(type));

    Type * promoType = nullptr;

    if (type->name == BOOL ||
        type->name == CHAR ||
        type->name == ENUM)
    {
        promoType = &MakeInt(context, 4, true)->type;
    }
    else
    {
        ASSERT(IsInt(type));
        if (type->size < 4)
            promoType = &MakeInt(context, 4, AsInt(type)->isSigned)->type;
        else
            promoType = type;
    }

    return promoType;
}

Type * UsualArithmeticConversion(TypeContext * context, Type * left, Type * right)
{
    // * x void/array/pointer/function/struct/union -> ERROR
    // * x float-8                                  -> float-8
    // * x float-4                                  -> float-4
    // bool/char = int-s1
    // enum = int-s4
    // int-(s/u)A x int-(s/u)B                      -> Promo(int-(s/u)B) (A < B)
    // int-sN x int-sN                              -> Promo(int-sN)
    // int-uN x int-uN                              -> Promo(int-uN)
    // int-sN x int-uN                              -> Promo(int-s2N) (N < 8)
    // int-s8 x int-u8                              -> ERROR

    ASSERT(IsArithmetic(left) && IsArithmetic(right));

    if (IsFloating(left) || IsFloating(right))
    {
        return left->size >= right->size ? left : right;
    }
    else
    {
        ASSERT(IsIntegral(left) && IsIntegral(right));

        IntType * i1 = AsInt(IntegralPromotion(context, left));
        IntType * i2 = AsInt(IntegralPromotion(context, right));

        if (i1->type.size < i2->type.size)
        {
            IntType * t = i1;
            i1 = i2;
            i2 = t;
        }

        if (i1->type.size > i2->type.size)
        {
            return &i1->type;
        }
        else
        {
            if (i1->isSigned == i2->isSigned)
            {
                return &i1->type;
            }
            else
            {
                ASSERT(i1->type.size < 8);
                return &MakeInt(context, i1->type.size * 2, true)->type;
            }
        }
    }
}

Type * DefaultArgumentPromotion(TypeContext * context, Type * type)
{
    // void/array/function  -> ERROR
    // bool/char/enum       -> int-s8
    // int-s*               -> int-s8
    // int-u*               -> int-u8
    // float-*              -> float-8
    // pointer              -> pointer
    // struct               -> struct
    // union                -> union

    if (IsIntegral(type))
    {
        bool isSigned = IsInt(type) ? AsInt(type)->isSigned : true;
        return &MakeInt(context, 8, isSigned)->type;
    }
    else if (IsFloating(type))
    {
        return &MakeFloat(context, type->size)->type;
    }
    else
    {
        ASSERT(type->name == POINTER ||
               type->name == STRUCT ||
               type->name == UNION);
        return type;
    }
}

Type * DecayType(TypeContext * context, Type * type)
{
    // array of T           -> pointer to T (keep qualifiers)
    // function returns T   -> pointer to function returns T
    // *                    -> *

    if (type->name == ARRAY)
    {
        PointerType * pointerType = MakePointer(context);

        pointerType->target = AsArray(type)->target;
        pointerType->type.prop |= (type->prop & TP_CONST);

        return &pointerType->type;
    }
    else if (type->name == FUNCTION)
    {
        return &MakePointer(context, type)->type;
    }
    else
    {
        return type;
    }
}

Type * PtrDiffType(TypeContext * context)
{
    return &MakeInt(context, 8, true)->type;
}

bool IsPtrDiffType(Type * type)
{
    return IsInt(type) && type->size == 8;
}

TypeContext * CreateTypeContext()
{
    TypeContext * context = new TypeContext;
    
    context->nextTypeBaseId = MAX_RESERVED_BASE_ID + 1;

    return context;
}

bool TypeEqual(Type * a, Type * b)
{
    // void/bool/char/int/float -> prop + type id
    // enum/struct/union        -> prop + type id
    // array                    -> prop + length + target
    // pointer                  -> prop + target
    // function                 -> prop + protocol

    if (a->name != b->name)
        return false;

    int propSet = TP_INCOMPLETE /*| TP_CONST*/; // TODO: add TP_CONST
    if ((a->prop & propSet) != (b->prop & propSet))
        return false;

    if (a->name == ARRAY)
    {
        ArrayType * aArray = AsArray(a);
        ArrayType * bArray = AsArray(b);
        return (aArray->length == bArray->length) &&
               TypeEqual(aArray->target, bArray->target);
    }
    else if (a->name == POINTER)
    {
        PointerType * aPointer = AsPointer(a);
        PointerType * bPointer = AsPointer(b);
        return TypeEqual(aPointer->target, bPointer->target);
    }
    else if (a->name == FUNCTION)
    {
        FunctionType * aFunction = AsFunction(a);
        FunctionType * bFunction = AsFunction(b);

        if (aFunction->isVarList != bFunction->isVarList)
            return false;
        if (!TypeEqual(aFunction->target, bFunction->target))
            return false;
        for (Type * aParam = aFunction->memberType[0], * bParam = bFunction->memberType[0];
             aParam || bParam;
             ++aParam, ++bParam)
        {
            if (!aParam || !bParam)
                return false;
            if (!TypeEqual(aParam, bParam))
                return false;
        }
        return true;
    }
    else
    {
        return TypeId(a) == TypeId(b);
    }
}

size_t TypeSize(Type * type)
{
    ASSERT( (type->prop & TP_IS_OBJECT) || type->size > 0 );
    return type->size;
}

size_t TypeAlignment(Type * type)
{
    return type->align;
}

bool IsScalar(Type * type)
{
    return type->prop & TP_IS_SCALAR;
}

bool IsIntegral(Type * type)
{
    return type->prop & TP_IS_INTEGRAL;
}

bool IsFloating(Type * type)
{
    return type->name == FLOAT;
}

bool IsArithmetic(Type * type)
{
    return IsIntegral(type) || IsFloating(type);
}

bool IsConst(Type * type)
{
    return type->prop & TP_CONST;
}

bool IsIncomplete(Type * type)
{
    return type->prop & TP_INCOMPLETE;
}

bool IsAssignable(Type * type)
{
    return type->prop & TP_ASSIGNABLE;
}

bool IsAddressable(Type * type)
{
    return type->prop & TP_ADDRESSABLE;
}

bool IsVoid(Type * type)
{
    return type->name == VOID;
}

bool IsBool(Type * type)
{
    return type->name == BOOL;
}

bool IsInt(Type * type)
{
    return type->name == INT;
}

bool IsStructOrUnion(Type * type)
{
    return type->name == STRUCT || type->name == UNION;
}

bool IsArray(Type * type)
{
    return type->name == ARRAY;
}

bool IsPointer(Type * type)
{
    return type->name == POINTER;
}

bool IsPointerToObject(Type * type)
{
    return IsPointer(type) && (AsPointer(type)->target->prop & TP_IS_OBJECT);
}

bool IsPointerToFunction(Type * type)
{
    return IsPointer(type) && (AsPointer(type)->target->name == FUNCTION);
}

bool IsFunction(Type * type)
{
    return type->name == FUNCTION;
}

bool IsCallableObject(Type * type)
{
    return (type->name == FUNCTION) ||
           (type->name == POINTER && AsPointer(type)->target->name == FUNCTION);
}

Type * GetMemberType(Type * type, StringRef memberName)
{
    ASSERT(IsStructOrUnion(type) && !IsIncomplete(type));

    Type * memberType = nullptr;

    if (type->name == STRUCT)
    {
        StructType * structType = AsStruct(type);
        for (int i = 0; i < 10; ++i)
        {
            if (structType->memberType[i] &&
                structType->memberName[i] == memberName)
            {
                memberType = structType->memberType[i];
                break;
            }
        }
    }
    else
    {
        UnionType * unionType = AsUnion(type);
        for (int i = 0; i < 10; ++i)
        {
            if (unionType->memberType[i] &&
                unionType->memberName[i] == memberName)
            {
                memberType = unionType->memberType[i];
                break;
            }
        }
    }

    // TODO: handle 'const struct/union'

    ASSERT(memberType);

    return memberType;
}

size_t GetMemberOffset(Type * type, StringRef memberName)
{
    ASSERT(IsStructOrUnion(type) && !IsIncomplete(type));

    size_t offset = 10;

    if (type->name == STRUCT)
    {
        StructType * structType = AsStruct(type);
        for (int i = 0; i < 10; ++i)
        {
            if (structType->memberType[i] &&
                structType->memberName[i] == memberName)
            {
                offset = i;
                break;
            }
        }

        ASSERT(offset != 10);
        return structType->memberOffset[offset];
    }
    else
    {
        UnionType * unionType = AsUnion(type);
        for (int i = 0; i < 10; ++i)
        {
            if (unionType->memberType[i] &&
                unionType->memberName[i] == memberName)
            {
                offset = i;
                break;
            }
        }

        ASSERT(offset != 10);
        return unionType->memberOffset[offset];
    }
}

Type * GetTargetType(Type * type)
{
    ASSERT(type->name == ARRAY ||
           type->name == POINTER ||
           type->name == FUNCTION);

    if (type->name == ARRAY)
    {
        return AsArray(type)->target;
    }
    else if(type->name == POINTER)
    {
        return AsPointer(type)->target;
    }
    else
    {
        return AsFunction(type)->target;
    }
}

bool IsMatchedCall(FunctionType * func, std::vector<Type *> & arguments)
{
    size_t paramCount = 0;

    for (size_t i = 0; i < 10; ++i)
    {
        if (func->memberType[i])
            ++paramCount;
        else
            break;
    }

    if (paramCount > arguments.size())
        return false;
    if (paramCount < arguments.size() && !func->isVarList)
        return false;

    for (size_t i = 0; i < paramCount; ++i)
    {
        Type * t1 = func->memberType[i];
        Type * t2 = arguments[i];
        if (!TypeEqual(t1, t2))
            return false;
    }

    return true;
}

std::string TypeDebugString(Type * type)
{
    std::string s = "unknown";

    if (type->name == VOID)
    {
        s = "void";
    }
    else if (type->name == BOOL)
    {
        s = "bool";
    }
    else if (type->name == CHAR)
    {
        s = "char";
    }
    else if (type->name == INT)
    {
        s = "int" + std::to_string(TypeSize(type));
    }
    else if (type->name == FLOAT)
    {
        s = "float" + std::to_string(TypeSize(type));
    }
    else if (type->name == ARRAY)
    {
        s = "array [" + std::to_string(AsArray(type)->length) + "] of ";
        s += TypeDebugString(AsArray(type)->target);
    }
    else if (type->name == POINTER)
    {
        s = "pointer to ";
        s += TypeDebugString(AsPointer(type)->target);
    }
    else if (type->name == FUNCTION)
    {
        s = "function returns ";
        s += TypeDebugString(AsFunction(type)->target);
    }
    else if (type->name == ENUM)
    {
        s = "enum";
    }
    else if (type->name == STRUCT)
    {
        s = "struct";
    }
    else if (type->name == UNION)
    {
        s = "union";
    }

    return s;
}

void PrintType(Type * type)
{
    std::cout << TypeId(type) << "\t" << TypeDebugString(type) << std::endl;
}

void PrintTypeContext(TypeContext * context)
{
    for (Type * type : context->types)
    {
        PrintType(type);
    }
}

}