#pragma once

#include "common.h"

// TODO: type compatibility

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

    void setQualifier(int qualifiers)
    {
        // TODO: check input
        _prop |= qualifiers;
    }
    int getQualifier() const
    {
        return _prop & (TP_CONST | TP_VOLATILE);
    }
    void unsetQualifier(int qualifiers)
    {
        // TODO: check input
        _prop &= ~qualifiers;
    }
    void markComplete()
    {
        unsetProp(TP_INCOMPLETE);
    }

    virtual bool equal(const Type &o) const
    {
        return _tc == o._tc && _prop == o._prop && _size == o._size &&
               _align == o._align;
    }

    // from Stringable
    virtual std::string toString() const
    {
        return "";
        // std::string s;
        // s += '<';
        // if (isIncomplete()) s += 'I';
        // if (isConst()) s += 'C';
        // if (isVolatile()) s += 'B';
        // if (isLvalue()) s += 'L';
        // if (isIntegral()) s += 'i';
        // if (isArithmetic()) s += 'A';
        // if (isScalar()) s += 'S';
        // if (isObject()) s += 'O';
        // if (isFunction()) s += 'F';
        // if (s.empty()) s += '-';
        // s += ':';
        // s += std::to_string(getSize());
        // s += ':';
        // s += std::to_string(getAlignment());
        // s += '>';
        // return s;
    }
};

class VoidType : public Type
{
    friend TypeUtil;

   public:
    VoidType() : Type(T_VOID, TP_INCOMPLETE | TP_IS_OBJECT) {}

    // from Stringable
    virtual std::string toString() const
    {
        return "void" + Type::toString();
    }
};
class CharType : public Type
{
    friend TypeUtil;

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
        return "char" + Type::toString();
    }
};
class IntegerType : public Type
{
    friend TypeUtil;

    const char *_desc;
    bool _signed;

   public:
    IntegerType(const char *desc)
        : Type(T_INT,
               TP_IS_INTEGRAL | TP_IS_ARITHMETIC | TP_IS_SCALAR | TP_IS_OBJECT,
               4, 4),
          _desc(desc),
          _signed(true)
    {
        // assume desc is valid combination
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

    bool isSigned() const
    {
        return _signed;
    }

    virtual bool equal(const Type &o) const
    {
        if (!Type::equal(o))
            return false;
        const IntegerType &i = dynamic_cast<const IntegerType &>(o);
        return _signed == i._signed;
    }

    // from Stringable
    virtual std::string toString() const
    {
        std::string s;
        for (const char *p = _desc; *p != '\0'; ++p)
        {
            if (!s.empty())
                s += ' ';
            switch (*p)
            {
                case 'S': s += "signed"; break;
                case 'U': s += "unsigned"; break;
                case 'c': s += "char"; break;
                case 's': s += "short"; break;
                case 'l': s += "long"; break;
                case 'i': s += "int"; break;
                default: break;
            }
        }
        return s + Type::toString();
    }
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
    virtual std::string toString() const
    {
        std::string s;
        for (const char *p = _desc; *p != '\0'; ++p)
        {
            if (!s.empty())
                s += ' ';
            switch (*p)
            {
                case 'f': s += "float"; break;
                case 'd': s += "double"; break;
                case 'l': s += "long"; break;
                default: break;
            }
        }
        return s + Type::toString();
    }
};
// derived types
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
    virtual std::string toString() const
    {
        return "pointer to " + (_t ? _t->toString() : "null") + Type::toString();
    }
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
    virtual void setTargetType(Type *t)
    {
        DerivedType::setTargetType(t);
        if (_n != 0 && !t->isIncomplete())
        {
            _size = t->getSize() * _n;
            _align = t->getAlignment();
            unsetProp(TP_INCOMPLETE);
        }
    }

    virtual bool equal(const Type &o) const
    {
        if (!DerivedType::equal(o))
            return false;
        const ArrayType &a = dynamic_cast<const ArrayType &>(o);
        return _n == a._n;
    }

    // from Stringable
    virtual std::string toString() const
    {
        return "array of " + (_t ? _t->toString() : "null") + Type::toString();
    }
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

    virtual bool equal(const Type &o) const
    {
        if (!DerivedType::equal(o))
            return false;

        const FuncType &f = dynamic_cast<const FuncType &>(o);
        if (_var_arg != f._var_arg || _param_t.size() != f._param_t.size())
            return false;
        for (size_t i = 0; i < _param_t.size(); ++i)
        {
            assert(_param_t[i] && f._param_t[i]);
            if (!_param_t[i]->equal(*f._param_t[i]))
                return false;
        }
        return true;
    }

    // from Stringable
    virtual std::string toString() const
    {
        return "function returns " + (_t ? _t->toString() : "null") + Type::toString();
    }
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

    void setImpl(const Type *impl)
    {
        assert(impl != nullptr);  // && t->getClass() == T_ENUM_IMPL);

        if (impl->getClass() != _impl_type)
        {
            SyntaxError("TagType: tag type mismatch.");
        }

        _impl = impl;
        _size = impl->getSize();
        _align = impl->getAlignment();
        unsetProp(TP_INCOMPLETE);
        // DebugLog("TagType: set impl of: " + _name.toString());
    }
    const Type *getImpl() const
    {
        return _impl;
    }

    virtual bool equal(const Type &o) const
    {
        if (!Type::equal(o))
            return false;

        const TagType &t = dynamic_cast<const TagType &>(o);
        if (_name != t._name || _impl_type != t._impl_type)
            return false;

        assert(_impl && t._impl);
        return _impl->equal(*t._impl);
    }

    // from Stringable
    virtual std::string toString() const
    {
        std::string s;
        switch (_impl_type)
        {
            case T_ENUM: s += "enum"; break;
            case T_STRUCT: s += "struct"; break;
            case T_UNION: s += "union"; break;
            case T_TYPEDEF: s += "typedef"; break;
            default: s += "unknown_tag_type"; break;
        }
        s += ' ';
        s += _name.toString();
        s += Type::toString();
        s += " {";
        if (_impl != nullptr)
        {
            s += ' ';
            s += _impl->toString();
            s += ' ';
        }
        s += '}';
        return s;
    }
};
class EnumConstType : public Type
{
    friend TypeUtil;

    StringRef _name;
    int _value;  // c89: can only be 'int'

   public:
    EnumConstType(StringRef name, int value)
        : Type(T_ENUM_CONST, TP_IS_INTEGRAL | TP_IS_ARITHMETIC | TP_IS_SCALAR,
               4, 4),
          _name(name),
          _value(value)
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

    virtual bool equal(const Type &o) const
    {
        if (!Type::equal(o))
            return false;

        const EnumConstType &e = dynamic_cast<const EnumConstType &>(o);
        return _name == e._name && _value == e._value;
    }

    // from Stringable
    virtual std::string toString() const
    {
        return "enum-const";
    }
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

    virtual bool equal(const Type &o) const
    {
        SyntaxError("not implemented.");
        return false;
    }

    // from Stringable
    virtual std::string toString() const
    {
        std::string s;
        for (auto m : _members)
        {
            s += m->getName().toString();
            s += '(';
            s += std::to_string(m->getValue());
            s += ')';
            s += ',';
        }
        if (!s.empty())
            s.pop_back();
        return s + Type::toString();
    }
};
class StructTypeImpl : public Type
{
    friend TypeUtil;

    vector<StringRef> _member_name;
    vector<const Type *> _member_type;

   public:
    StructTypeImpl(bool share_storage)
        : Type(share_storage ? T_UNION : T_STRUCT, 0, 0, 1)
    {
    }
    void addMember(StringRef name, const Type *ct)
    {
        Type *t = const_cast<Type *>(ct);
        if (t->isIncomplete())
            SyntaxError("struct: can't add incomplete type.");
        _member_name.push_back(name);
        _member_type.push_back(t);
        if (_tc == T_STRUCT)
            _size += t->getSize();
        else
            _size = (_size >= t->getSize()) ? _size : t->getSize();
        _align = (_align >= t->getAlignment()) ? _align : t->getAlignment();
    }
    const Type *getMemberType(StringRef name) const
    {
        const Type *t = nullptr;
        size_t i = 0;
        for (StringRef nm : _member_name)
        {
            if (nm == name)
            {
                t = _member_type[i];
                break;
            }
            ++i;
        }
        if (i == _member_name.size())
        {
            SyntaxError("member not found.");
        }
        return t;
    }

    virtual bool equal(const Type &o) const
    {
        SyntaxError("not implemented.");
        return false;
    }

    // from Stringable
    virtual std::string toString() const
    {
        std::string s;
        for (auto t : _member_type)
        {
            s += t->toString();
            s += ':';
        }
        if (!s.empty())
            s.pop_back();
        return s + Type::toString();
    }
};
class TypedefTypeImpl : public Type
{
    friend TypeUtil;

    // const Type *_t;

   public:
    TypedefTypeImpl(const Type *t) : Type(T_TYPEDEF, 0)  //, _t(t)
    {
    }

    virtual bool equal(const Type &o) const
    {
        SyntaxError("not implemented.");
        return false;
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
    friend TypeUtil;

   public:
    LabelType() : Type(T_LABEL, 0) {}

    // from Stringable
    virtual std::string toString() const
    {
        return "label";
    }
};

class TypeUtil
{
   public:
    static Type *Concatenate(Type *front, Type *back);
    // unbox derived type
    static Type *TargetType(Type *aggregate);
    static Type *Merge(Type *t1, Type *t2);
    static Type *CloneTop(const Type *T);
    static bool Equal(const Type *t1, const Type *t2);
    static bool Compatible(const Type *t1, const Type *t2);
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
    static Type *IntegerConversion(Type *t);
    static Type *PointerConversion(Type *t);
    static Type *FloatingConversion(Type *t);
    // floating-int, int-floating
    static Type *FIIFConversion(Type *from, const Type *to);
};
