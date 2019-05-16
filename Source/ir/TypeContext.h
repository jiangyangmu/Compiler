#pragma once

#include "../Util/String.h"
#include "../Util/Integer.h"

#include <vector>
#include <map>
#include <tuple>

namespace Language {

// INT1, INT2, UINT1, UINT2, ...
// FLT4, FLT8, FLT10
enum TypeName
{
    VOID,
    BOOL,
    CHAR,
    INT,
    FLOAT,
    ARRAY,
    POINTER,
    FUNCTION,
    ENUM,
    STRUCT,
    UNION,
};

enum TypeProperty
{
    TP_INCOMPLETE       = 1,
    TP_CONST            = (1 << 1),
    TP_ADDRESSABLE      = (1 << 2),
    TP_ASSIGNABLE       = (1 << 3),

    TP_IS_INTEGRAL      = (1 << 4),
    TP_IS_ARITHMETIC    = (1 << 5),
    TP_IS_SCALAR        = (1 << 6),
    
    TP_IS_OBJECT        = (1 << 7),
};

struct Type
{
    TypeName        name;
    int             prop;
    size_t          size;
    size_t          align;
    u64             baseId;
};

struct VoidType
{
    Type type;
};

struct BoolType
{
    Type type;
};

struct CharType
{
    Type type;
};

struct IntType
{
    Type type;
    bool isSigned;
};

struct FloatType
{
    Type type;
};

struct ArrayType
{
    Type type;

    size_t length;
    Type * target;
};

struct PointerType
{
    Type type;

    Type * target;
};

struct FunctionType
{
    Type type;

    Type *      target;
    StringRef   memberName[10];
    Type *      memberType[10];
    bool        isVarList;
};

struct EnumType
{
    Type type;

    StringRef   memberName[10];
    int         memberValue[10];
};

struct StructType
{
    Type type;

    StringRef   memberName[10];
    Type *      memberType[10];
    size_t      memberOffset[10];
};

struct UnionType
{
    Type type;

    StringRef   memberName[10];
    Type *      memberType[10];
    size_t      memberOffset[10];
};

VoidType *      AsVoid(Type * type);
BoolType *      AsBool(Type * type);
CharType *      AsChar(Type * type);
IntType *       AsInt(Type * type);
FloatType *     AsFloat(Type * type);
ArrayType *     AsArray(Type * type);
PointerType *   AsPointer(Type * type);
FunctionType *  AsFunction(Type * type);
EnumType *      AsEnum(Type * type);
StructType *    AsStruct(Type * type);
UnionType *     AsUnion(Type * type);

// Construct type object

struct TypeContext;

VoidType *      MakeVoid(TypeContext * context);
BoolType *      MakeBool(TypeContext * context);
CharType *      MakeChar(TypeContext * context);
IntType *       MakeInt(TypeContext * context, size_t width = 4, bool isSigned = true);
FloatType *     MakeFloat(TypeContext * context, size_t width = 4);

ArrayType *     MakeArray(TypeContext * context, size_t length); // length, target
void            ArraySetTarget(TypeContext * context, ArrayType * type, Type * target);

PointerType *   MakePointer(TypeContext * context);  // target
void            PointerSetTarget(TypeContext * context, PointerType * type, Type * target);
PointerType *   MakePointer(TypeContext * context, Type * target);

FunctionType *  MakeFunction(TypeContext * context); // protocol
void            FunctionSetTarget(FunctionType * type, Type * target);
void            FunctionAddParameter(FunctionType * type, StringRef pname, Type * ptype);
void            FunctionSetVarList(FunctionType * type);
void            FunctionDone(TypeContext * context, FunctionType * type);

EnumType *      MakeEnum(TypeContext * context);     // enum const name/value
void            EnumAddConst(EnumType * type, StringRef name, int value);
void            EnumChangeLastConstValue(EnumType * type, int value);

StructType *    MakeStruct(TypeContext * context);   // member id/type/offset
void            StructAddMember(StructType * type, StringRef mname, Type * mtype);
void            StructDone(StructType * type);

UnionType *     MakeUnion(TypeContext * context);    // member id/type
void            UnionAddMember(UnionType * type, StringRef mname, Type * mtype);

Type *          CloneType(TypeContext * context, Type * type);

// array, pointer, function
void            SetTargetType(TypeContext * context, Type * type, Type * target);

void            TypeSetProperties(Type * type, int prop);
void            TypeSetAssignable(Type * type);
void            TypeSetAddressable(Type * type);

Type *          IntegralPromotion(TypeContext * context, Type * type);
Type *          UsualArithmeticConversion(TypeContext * context, Type * left, Type * right);
Type *          DefaultArgumentPromotion(TypeContext * context, Type * type);

// array of T           -> pointer to T (keep qualifiers)
// function returns T   -> pointer to function returns T
// *                    -> *
Type *          DecayType(TypeContext * context, Type * type);
Type *          PtrDiffType(TypeContext * context);
bool            IsPtrDiffType(Type * type);

// Query type object properties

u64             TypeId(Type * type);

bool            TypeEqual(Type * a, Type * b);
size_t          TypeSize(Type * type);
size_t          TypeAlignment(Type * type);

// categories
bool            IsScalar(Type * type);
bool            IsIntegral(Type * type);
bool            IsFloating(Type * type);
bool            IsArithmetic(Type * type);

// attributes
bool            IsConst(Type * type);
bool            IsIncomplete(Type * type);
bool            IsAssignable(Type * type);
bool            IsAddressable(Type * type);

// specific types
bool            IsVoid(Type * type);
bool            IsBool(Type * type);
bool            IsInt(Type * type);
bool            IsStructOrUnion(Type * type);
bool            IsArray(Type * type);
bool            IsPointer(Type * type);
bool            IsPointerToObject(Type * type);
bool            IsPointerToFunction(Type * type);
bool            IsFunction(Type * type);
bool            IsCallableObject(Type * type); // function, pointer to function

// specific type properties
Type *          GetMemberType(Type * type, StringRef memberName);
size_t          GetMemberOffset(Type * type, StringRef memberName);
Type *          GetTargetType(Type * type);
bool            IsMatchedCall(FunctionType * func, std::vector<Type *> & arguments);

// Manage runtime environment

struct TypeContext
{
    // all types
    std::vector<Type *> types;

    // type id allocation
    u64 nextTypeBaseId;
    // target type id -> pointer type base id
    std::map<u64, u64> existingPointTypeIdMap;
    // {target type id, array length} -> array type base id
    std::map<std::tuple<u64, u64>, u64> existingArrayTypeIdMap;
    // protocol hash -> function type base id
    std::map<u64, u64> existingFunctionTypeIdMap;
};

TypeContext *   CreateTypeContext();

// Debug

std::string     TypeDebugString(Type * type);
void            PrintType(Type * type);
void            PrintTypeContext(TypeContext * context);

} // namespace Language