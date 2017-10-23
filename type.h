#pragma once

#include "common.h"

// TODO: type compatibility
// TODO: type equal-ness
// TODO: type incomplete-ness
// TODO: type lvalue

// enum ETypeOperations
/*
{
    TOp_ADD = 1,
    TOp_SUB = (1 << 1),
    TOp_INC = (1 << 2),
    TOp_DEC = (1 << 3),
    TOp_MUL = (1 << 4),
    TOp_DIV = (1 << 5),
    TOp_MOD = (1 << 6),
    TOp_EVAL = (1 << 7),
    TOp_ADDR = (1 << 8),
    TOp_ASSIGN = (1 << 9),
    TOp_INDEX = (1 << 10),
    TOp_OFFSET = (1 << 11),
    TOp_CALL = (1 << 12)
};
*/

enum ETypeProperty
{
    // properties
    TP_INCOMPLETE = 1,
    TP_CONST = (1 << 2),
    TP_VOLATILE = (1 << 3),
    TP_LVALUE = (1 << 4),
    // categorys
    TP_IS_INTEGRAL = (1 << 5),
    TP_IS_ARITHMETIC = (1 << 6),
    TP_IS_SCALAR = (1 << 7),
    // categorys - object type
    TP_IS_OBJECT = (1 << 8),
    TP_IS_FUNCTION = (1 << 9),
};

enum ETypeClass
{
    T_NONE,
    T_VOID,
    T_CHAR,
    T_INT,
    T_FLOAT,
    T_POINTER,
    T_ARRAY,
    T_TAG,
    T_STRUCT,
    T_UNION,
    T_ENUM,
    T_ENUM_CONST,
    T_FUNCTION,
    T_TYPEDEF,
    T_LABEL
};

class TypeUtil;

// Type: Everything about common behavior
// 1. knows everything about type
// 2. knows sizeof(object)
class Type : public Stringable
{
    friend TypeUtil;

   protected:
    ETypeClass _tc;
    // properties
    int _prop;
    // representation
    size_t _size;
    size_t _align;

    Type(ETypeClass tc, int prop, size_t size = 0, size_t align = 0)
        : _tc(tc), _prop(prop), _size(size), _align(align)
    {
    }
    void setProp(int p)
    {
        _prop |= p;
    }
    void unsetProp(int p)
    {
        _prop &= ~p;
    }

   public:
    bool isIncomplete() const
    {
        return _prop & TP_INCOMPLETE;
    }
    bool isConst() const
    {
        return _prop & TP_CONST;
    }
    bool isVolatile() const
    {
        return _prop & TP_VOLATILE;
    }
    bool isLvalue() const
    {
        return _prop & TP_LVALUE;
    }
    bool isIntegral() const
    {
        return _prop & TP_IS_INTEGRAL;
    }
    bool isArithmetic() const
    {
        return _prop & TP_IS_ARITHMETIC;
    }
    bool isScalar() const
    {
        return _prop & TP_IS_SCALAR;
    }
    bool isObject() const
    {
        return _prop & TP_IS_OBJECT;
    }
    bool isFunction() const
    {
        return _prop & TP_IS_FUNCTION;
    }
    bool isModifiable() const
    {
        return isLvalue() && !isIncomplete() && !isConst();
    }
    ETypeClass getClass() const
    {
        return _tc;
    }
    size_t getSize() const
    {
        return _size;
    }
    size_t getAlignment() const
    {
        return _align;
    }

    void setLvalue()
    {
        setProp(TP_LVALUE);
    }
    void unsetLvalue()
    {
        unsetProp(TP_LVALUE);
    }
    void setQualifier(int qualifiers)
    {
        _prop |= (qualifiers & (TP_CONST | TP_VOLATILE));
    }
    int getQualifier() const
    {
        return _prop & (TP_CONST | TP_VOLATILE);
    }
    void unsetQualifier(int qualifiers)
    {
        _prop &= ~(qualifiers & (TP_CONST | TP_VOLATILE));
    }
    void markComplete()
    {
        unsetProp(TP_INCOMPLETE);
    }

    virtual bool equal(const Type &o) const
    {
        // XXX: ignore Lvalue prop
        // TODO: maybe create another function same() ?
        int p = _prop & ~TP_LVALUE;
        int op = o._prop & ~TP_LVALUE;
        return _tc == o._tc && (p == op) && _size == o._size &&
               _align == o._align;
    }

    // from Stringable
    virtual std::string toString() const;
};
class IntegralType : public Type
{
   protected:
    bool _signed;

    IntegralType(ETypeClass tc, int prop, size_t size, size_t align)
        : Type(tc, prop | TP_IS_INTEGRAL | TP_IS_ARITHMETIC | TP_IS_SCALAR,
               size, align),
          _signed(false)
    {
    }

   public:
    bool isSigned() const
    {
        return _signed;
    }

    virtual bool equal(const Type &o) const
    {
        if (!Type::equal(o))
            return false;
        const IntegralType &p = dynamic_cast<const IntegralType &>(o);
        return isSigned() == p.isSigned();
    }
};
class DerivedType : public Type
{
   protected:
    Type *_t;

    DerivedType(ETypeClass tc, int prop, size_t size = 0, size_t align = 0)
        : Type(tc, prop, size, align), _t(nullptr)
    {
    }

   public:
    virtual void setTargetType(Type *t)
    {
        // assert(_t == nullptr);
        assert(t != nullptr);
        _t = t;
    }
    Type *getTargetType()
    {
        return _t;
    }

    virtual bool equal(const Type &o) const
    {
        if (!Type::equal(o))
            return false;
        const DerivedType &p = dynamic_cast<const DerivedType &>(o);
        assert(_t && p._t);
        return _t->equal(*p._t);
    }
};

class VoidType : public Type
{
    friend TypeUtil;

   public:
    VoidType() : Type(T_VOID, TP_INCOMPLETE | TP_IS_OBJECT) {}

    // from Stringable
    virtual std::string toString() const;
};
class CharType : public IntegralType
{
    friend TypeUtil;

   public:
    CharType() : IntegralType(T_CHAR, TP_IS_OBJECT, 1, 1)
    {
        _signed = true;
    }

    // from Stringable
    virtual std::string toString() const;
};
class IntegerType : public IntegralType
{
    friend TypeUtil;

    const char *_desc;

   public:
    IntegerType(const char *desc)
        : IntegralType(T_INT, TP_IS_OBJECT, 4, 4), _desc(desc)
    {
        // assume desc is valid combination
        assert(desc != nullptr);

        // sign
        _signed = true;
        if (*desc == 'S')
            ++desc;
        else if (*desc == 'U')
            _signed = false, ++desc;
        // size
        switch (*desc)
        {
            case 'c': _size = 1; break;
            case 's': _size = 2; break;
            case 'l': _size = sizeof(long); break;
            default: break;
        }
        _align = _size;
    }

    // from Stringable
    virtual std::string toString() const;
};
class FloatingType : public Type
{
    friend TypeUtil;

    const char *_desc;

   public:
    FloatingType(const char *desc)
        : Type(T_FLOAT, TP_IS_ARITHMETIC | TP_IS_SCALAR | TP_IS_OBJECT, 4, 4),
          _desc(desc)
    {
        // assume desc is valid combination
        assert(desc != nullptr);

        // size
        switch (*desc)
        {
            case 'd': _align = _size = 8; break;
            case 'l':  // long double
                _size = 10;
                _align = 16;
                break;
            default: break;
        }
    }

    // from Stringable
    virtual std::string toString() const;
};
// derived types
class PointerType : public DerivedType
{
    friend TypeUtil;

   public:
    PointerType()
        : DerivedType(T_POINTER, TP_IS_SCALAR | TP_IS_OBJECT, sizeof(void *),
                      sizeof(void *))
    {
    }

    // virtual bool equal(const Type &o) const;

    // from Stringable
    virtual std::string toString() const;
};
class ArrayType : public DerivedType
{
    friend TypeUtil;

    size_t _n;

   public:
    ArrayType()
        : DerivedType(T_ARRAY, TP_CONST | TP_INCOMPLETE | TP_IS_OBJECT), _n(0)
    {
    }
    ArrayType(size_t n)
        : DerivedType(T_ARRAY, TP_CONST | TP_INCOMPLETE | TP_IS_OBJECT), _n(n)
    {
    }

    // from DerivedType
    virtual void setTargetType(Type *t);

    virtual bool equal(const Type &o) const;

    // from Stringable
    virtual std::string toString() const;
};
class FuncType : public DerivedType
{
    friend TypeUtil;

    vector<const Type *> _param_t;  // can be nullptr for identifier_list
    vector<StringRef> _param_name;  // can be "" for parameter_type_list
    bool _var_arg;

   public:
    FuncType()
        : DerivedType(T_FUNCTION, TP_CONST | TP_INCOMPLETE | TP_IS_FUNCTION),
          _var_arg(false)
    {
    }
    void addParam(const Type *t, StringRef name);
    void fixParamType(size_t i, const Type *t);
    void setVarArg();

    virtual bool equal(const Type &o) const;

    // from Stringable
    virtual std::string toString() const;
};
// tag types & impl
class TagType : public Type
{
    friend TypeUtil;

    StringRef _name;
    ETypeClass _impl_type;  // expected impl type
    const Type *_impl;

   public:
    TagType(ETypeClass impl_type, StringRef name)
        : Type(T_TAG, TP_INCOMPLETE),
          _name(name),
          _impl_type(impl_type),
          _impl(nullptr)
    {
        switch (impl_type)
        {
            case T_ENUM:
            case T_STRUCT:
            case T_UNION: setProp(TP_IS_OBJECT);
            default: break;
        }
    }

    void setImpl(const Type *impl);
    const Type *getImpl() const;

    virtual bool equal(const Type &o) const;

    // from Stringable
    virtual std::string toString() const;
};
class EnumConstType : public IntegralType
{
    friend TypeUtil;

    StringRef _name;
    int _value;  // c89: can only be 'int'

   public:
    EnumConstType(StringRef name, int value)
        : IntegralType(T_ENUM_CONST, 0, 4, 4), _name(name), _value(value)
    {
    }

    StringRef getName() const
    {
        return _name;
    }
    int getValue() const
    {
        return _value;
    }

    virtual bool equal(const Type &o) const;

    // from Stringable
    virtual std::string toString() const;
};
class EnumTypeImpl : public Type
{
    friend TypeUtil;

    vector<const EnumConstType *> _members;

   public:
    EnumTypeImpl()
        : Type(T_ENUM, TP_IS_INTEGRAL | TP_IS_ARITHMETIC | TP_IS_SCALAR, 4, 4)
    {
    }

    void addMember(const EnumConstType *member)
    {
        _members.push_back(member);
    }

    virtual bool equal(const Type &o) const;

    // from Stringable
    virtual std::string toString() const;
};
class StructTypeImpl : public Type
{
    friend TypeUtil;

    vector<StringRef> _member_name;
    vector<const Type *> _member_type;
    vector<size_t> _member_offset;

   public:
    StructTypeImpl(bool share_storage)
        : Type(share_storage ? T_UNION : T_STRUCT, 0, 0, 1)
    {
    }
    void addMember(StringRef name, const Type *ct);
    const Type *getMemberType(StringRef name) const;
    size_t getMemberOffset(StringRef name) const;

    virtual bool equal(const Type &o) const;

    // from Stringable
    virtual std::string toString() const;
};
class TypedefTypeImpl : public Type
{
    friend TypeUtil;

    // const Type *_t;

   public:
    TypedefTypeImpl(const Type *t) : Type(T_TYPEDEF, 0)  //, _t(t)
    {
    }

    virtual bool equal(const Type &o) const;

    // from Stringable
    virtual std::string toString() const;
};
// label
class LabelType : public Type
{
    friend TypeUtil;

   public:
    LabelType() : Type(T_LABEL, 0) {}

    // from Stringable
    virtual std::string toString() const;
};

class TypeUtil
{
   public:
    static Type *Type_size_t();
    static Type *Concatenate(Type *front, Type *back);
    // unbox derived type
    static Type *TargetType(Type *aggregate);
    static const Type *TargetType(const Type *aggregate);
    static Type *Merge(Type *t1, Type *t2);
    static Type *CloneTop(const Type *T);
    static bool Equal(const Type *t1, const Type *t2);
    static bool Compatible(const Type *t1, const Type *t2, StringRef *reason = nullptr);
    // Is 'test' more strict qualified than 'base' ?
    static bool MoreStrictQualified(const Type *test, const Type *base);
    // for anonymous struct/union/enum
    static StringRef GenerateTag();
};

class TypeConversion
{
   public:
    // Expression-Specific Conversions

    static Type *CondExprConversion(Type *if_true, Type *if_false);

    // Common Conversions

    static Type *ByAssignmentConversion(Type *to, Type *from);
    static Type *DefaultArgumentPromotion(Type *t);
    static Type *UsualArithmeticConversion(Type *l, Type *r);
    // when read the value of a L-value
    static Type *ValueTransformConversion(Type *t);

    // Basic Conversions

    static Type *IntegerPromotion(Type *t);
    static Type *BooleanConversion(Type *t);
    static Type *IntegerConversion(Type *from, Type *to);
    static Type *PointerConversion(Type *t);
    static Type *FloatingConversion(Type *t);
    // floating-int, int-floating
    static Type *FIIFConversion(Type *from, const Type *to);
};
