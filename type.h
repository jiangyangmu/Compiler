#pragma once

#include "common.h"

// TODO: type compatibility
// TODO: type equility

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
    // friend TypeFactory;

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
    ETypeClass getClass() const
    {
        return _tc;
    }

    void setQualifier(int qualifiers)
    {
        // TODO: check input
        _prop |= qualifiers;
    }

    // from Stringable
    virtual std::string toString() const
    {
        return "~<Type~>";
    }
};

class VoidType : public Type
{
   public:
    VoidType() : Type(T_VOID, TP_INCOMPLETE | TP_IS_OBJECT)
    {
    }

    // from Stringable
    virtual std::string toString() const
    {
        return "~<void~>";
    }
};
class CharType : public Type
{
   public:
    CharType()
        : Type(T_CHAR,
               TP_IS_INTEGRAL | TP_IS_ARITHMETIC | TP_IS_SCALAR | TP_IS_OBJECT,
               1, 1)
    {
    }

    // from Stringable
    virtual std::string toString() const
    {
        return "~<char~>";
    }
};
class IntegerType : public Type
{
    // const char *_desc;
    bool _signed;

   public:
    IntegerType(const char *desc)
        : Type(T_INT,
               TP_IS_INTEGRAL | TP_IS_ARITHMETIC | TP_IS_SCALAR | TP_IS_OBJECT,
               4, 4),
          // _desc(desc),
          _signed(true)
    {
        // desc is valid combination
        assert(desc != nullptr);
        // sign
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
    virtual std::string toString() const
    {
        return "~<int~>";
    }
};
class FloatingType : public Type
{
    // const char *_desc;

   public:
    FloatingType(const char *desc)
        : Type(T_FLOAT, TP_IS_ARITHMETIC | TP_IS_SCALAR | TP_IS_OBJECT, 4, 4)
    // , _desc(desc)
    {
        // desc is valid combination
        assert(desc != nullptr);
        // size
        switch (*desc)
        {
            case 'd':
                _size = 8;
                break;
            // case 'l': _size = sizeof(long double); break;
            default: break;
        }
        _align = _size;
    }

    // from Stringable
    virtual std::string toString() const
    {
        return "~<float~>";
    }
};
// derived types
class PointerType : public Type
{
    friend TypeUtil;

    Type *_t;

   public:
    PointerType()
        : Type(T_POINTER, TP_IS_SCALAR | TP_IS_OBJECT, sizeof(void *),
               sizeof(void *)),
          _t(nullptr)
    {
    }
    void setTargetType(Type *t)
    {
        _t = t;
    }

    // from Stringable
    virtual std::string toString() const
    {
        return "~<pointer~> to " + (_t ? _t->toString() : "~<null~>");
    }
};
class ArrayType : public Type
{
    friend TypeUtil;

    Type *_t;
    // size_t _n;

   public:
    ArrayType()
        : Type(T_ARRAY, TP_CONST | TP_INCOMPLETE | TP_IS_OBJECT), _t(nullptr)
    // , _n(0)
    {
    }
    ArrayType(size_t n)
        : Type(T_ARRAY, TP_CONST | TP_IS_OBJECT), _t(nullptr)  // , _n(n)
    {
    }
    void setElementType(Type *t)
    {
        _t = t;
    }

    // from Stringable
    virtual std::string toString() const
    {
        return "~<array~> of " + (_t ? _t->toString() : "~<null~>");
    }
};
class FuncType : public Type
{
    friend TypeUtil;

    Type *_t;
    vector<const Type *> _param_t;  // can be nullptr for identifier_list
    vector<StringRef> _param_name;  // can be "" for parameter_type_list
    bool _var_arg;

   public:
    FuncType()
        : Type(T_FUNCTION, TP_CONST | TP_INCOMPLETE | TP_IS_FUNCTION),
          _t(nullptr),
          _var_arg(false)
    {
    }
    void setReturnType(Type *t)
    {
        _t = t;
    }
    void addParam(const Type *t, StringRef name)
    {
        _param_t.push_back(t);
        _param_name.push_back(name);
    }
    void fixParamType(size_t i, const Type *t)
    {
        assert(i < _param_t.size());
        _param_t[i] = t;
    }
    void setVarArg()
    {
        _var_arg = true;
    }

    // from Stringable
    virtual std::string toString() const
    {
        return "~<function~> returns " + (_t ? _t->toString() : "~<null~>");
    }
};
// tag types & impl
class TagType : public Type
{
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
    }

    void setImpl(const Type *impl)
    {
        assert(impl != nullptr);  // && t->getClass() == T_ENUM_IMPL);

        if (impl->getClass() != _impl_type)
        {
            SyntaxError("TagType: tag type mismatch.");
        }

        switch (impl->getClass())
        {
            case T_ENUM:
            case T_STRUCT:
            case T_UNION: setProp(TP_IS_OBJECT);
            case T_TYPEDEF: _impl = impl; break;
            default: break;
        }
        unsetProp(TP_INCOMPLETE);
    }

    // from Stringable
    virtual std::string toString() const
    {
        return "~<tag~>";
    }
};
class EnumConstType : public Type
{
    StringRef _name;
    // int _value;  // c89: can only be 'int'

   public:
    EnumConstType(StringRef name, int value)
        : Type(T_ENUM_CONST, TP_IS_INTEGRAL | TP_IS_ARITHMETIC | TP_IS_SCALAR,
               4, 4)  // , _value(value)
    {
    }

    // from Stringable
    virtual std::string toString() const
    {
        return "~<enum-const~>";
    }
};
class EnumTypeImpl : public Type
{
    vector<const EnumConstType *> _members;

   public:
    EnumTypeImpl() : Type(T_ENUM, 0)
    {
    }
    void addMember(const EnumConstType *member)
    {
        _members.push_back(member);
    }

    // from Stringable
    virtual std::string toString() const
    {
        return "~<enum-impl~>";
    }
};
class StructTypeImpl : public Type
{
    vector<StringRef> _member_name;
    vector<const Type *> _member_type;

   public:
    StructTypeImpl(bool share_storage)
        : Type(share_storage ? T_UNION : T_STRUCT, 0)
    {
    }
    void addMember(StringRef name, const Type *t)
    {
        _member_name.push_back(name);
        _member_type.push_back(t);
    }

    // from Stringable
    virtual std::string toString() const
    {
        return (getClass() == T_UNION) ? "~<union-impl~>" : "~<struct-impl~>";
    }
};
class TypedefTypeImpl : public Type
{
    // const Type *_t;

   public:
    TypedefTypeImpl(const Type *t) : Type(T_TYPEDEF, 0)  //, _t(t)
    {
    }

    // from Stringable
    virtual std::string toString() const
    {
        return "~<typedef-impl~>";
    }
};
// label
class LabelType : public Type
{
   public:
    LabelType() : Type(T_LABEL, 0)
    {
    }

    // from Stringable
    virtual std::string toString() const
    {
        return "~<label~>";
    }
};

class TypeUtil
{
   public:
    static Type *Concatenate(Type *front, Type *back);
    static Type *Merge(Type *t1, Type *t2);
    static bool Equal(const Type *t1, const Type *t2);
    // for anonymous struct/union/enum
    static StringRef GenerateTag();
};
