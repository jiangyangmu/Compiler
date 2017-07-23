#pragma once

#include "common.h"

class SyntaxNode;
class Object;
class Environment;

/*
enum ETypeOperations
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
    T_STRUCT,
    T_ENUM,
    T_FUNCTION,
    T_TYPEDEF,
    T_LABEL
};

// Type: Everything about common behavior
// 1. knows everything about type
// 2. knows sizeof(object)
class Type
{
    friend TypeFactory;

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
};

class VoidType : public Type
{
   public:
    VoidType(int qualifiers = 0) : Type(T_VOID, TP_INCOMPLETE | qualifiers)
    {
    }
};
class CharType : public Type
{
   public:
    CharType(int qualifiers = 0)
        : Type(T_CHAR,
               TP_IS_INTEGRAL | TP_IS_ARITHMETIC | TP_IS_SCALAR | qualifiers, 1,
               1)
    {
    }
};
class IntegerType : public Type
{
    const char *_desc;
    bool _signed;

   public:
    IntegerType(int qualifiers = 0, const char *desc)
        : Type(T_INT,
               TP_IS_INTEGRAL | TP_IS_ARITHMETIC | TP_IS_SCALAR | qualifiers, 4,
               4),
          _desc(desc),
          _signed(true)
    {
        // desc is valid combination
        assert(desc != nullptr);
        // sign
        if (*desc == 's')
            ++desc;
        else if (*desc == 'u')
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
};
class FloatingType : public Type
{
    const char *_desc;

   public:
    FloatingType(int qualifiers = 0, const char *desc)
        : Type(T_FLOAT, TP_IS_ARITHMETIC | TP_IS_SCALAR | qualifiers, 4, 4),
          _desc(desc)
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
};
// derived types
class PointerType : public Type
{
    const Type *_t;

   public:
    PointerType(int qualifiers = 0)
        : Type(T_POINTER, TP_IS_SCALAR | qualifiers, sizeof(void *),
               sizeof(void *)), _t(nullptr)
    {
    }
    void targetType(const Type *t)
    {
        _t = t;
    }
};
class ArrayType : public Type
{
    const Type *_t;
    size_t _n;

   public:
    ArrayType()
        : Type(T_ARRAY, TP_CONST | TP_INCOMPLETE), _t(nullptr), _n(0)
    {
    }
    ArrayType(size_t n) : Type(T_ARRAY, TP_CONST), _t(nullptr), _n(n)
    {
    }
    void elementType(const Type *t)
    {
        _t = t;
    }
};
class FuncType : public Type
{
    const Type *_t;
    vector<const Type *> _param_t;
   public:
    FuncType() : Type(T_FUNCTION, TP_CONST | TP_INCOMPLETE), _t(nullptr)
    {
    }
    void returnType(const Type *t)
    {
        _t = t;
    }
    void paramType(const Type *p)
    {
        _param_t.push_back(p);
    }
};
// tag types
class EnumType : public Type
{
    StringRef _tag;

   public:
    EnumType(int qualifiers = 0, StringRef tag)
        : Type(T_ENUM, TP_INCOMPLETE | TP_IS_INTEGRAL | TP_IS_ARITHMETIC |
                           TP_IS_SCALAR | qualifiers,
               4),
          _tag(tag)
    {
    }
};
class StructType : public Type
{
    StringRef _tag;

   public:
    StructType(int qualifiers = 0, StringRef tag)
        : Type(T_STRUCT, TP_INCOMPLETE | qualifiers), _tag(tag)
    {
    }
};
class StructTypeImpl : public Type
{
    vector<StringRef> _member_name;
    vector<const Type *> _member_type;
   public:
    StructTypeImpl()
        : Type(T_STRUCT)
    {
    }
    void member(StringRef name, const Type *t)
    {
        _member_name.push_back(name);
        _member_type.push_back(t);
    }
};
class TypedefType : public Type
{
    StringRef _name;

   public:
    // typedef 'name' must be defined before construct this
    TypedefType(String name) : Type(T_TYPEDEF, 0), _name(name)
    {
    }
};
class TypedefTypeImpl : public Type
{
    const Type *_t;
   public:
    TypedefTypeImpl(const Type *t)
        : Type(T_TYPEDEF, 0), _t(t)
    {
    }
};
// label
class LabelType : public Type
{
   public:
    LabelType() : Type(T_LABEL, 0)
    {
    }
};

class TypeFactory
{
   public:
    static const Type *Void(int qualifiers)
    {
        return new VoidType(qualifiers);
    }
    static const Type *Char(int qualifiers)
    {
        return new CharType(qualifiers);
    }
    static const Type *Integer(int qualifiers, const char *desc)
    {
        return new IntegerType(qualifiers, desc);
    }
    static const Type *Floating(int qualifiers, const char *desc)
    {
        return new FloatingType(qualifiers, desc);
    }
    static const Type *Pointer(int qualifiers)
    {
        return new PointerType(qualifiers);
    }
    static const Type *Array()
    {
        return new ArrayType();
    }
    static const Type *Array(size_t n)
    {
        return new ArrayType(n);
    }
    static Type *Function()
    {
        return new FuncType();
    }
    static void FunctionReturning(FuncType *f, const Type *t)
    {
        f->returnType(t);
    }
    static void FunctionParameter(FuncType *f, const Type *t)
    {
        f->paramType(t);
    }
    static void FunctionBodyBegin(FuncType *f)
    {
    }
    static void FunctionBodyEnd(FuncType *f)
    {
        f->unsetProp(TP_INCOMPLETE);
    }
    static const Type *StructTag(int qualifiers, StringRef tag)
    {
        return new StructType(qualifiers, tag);
    }
    static Type *Struct()
    {
        return new StructTypeImpl();
    }
    static void StructMember(StructType *s, StringRef name, const Type *t)
    {
        s->member(name, t);
    }
    // ....
};

// // ------------------------------------------------------------------------
// // old code

// class TypeFactory;
// class IntegerType;
// class FloatingType;
// class PointerType;
// class ArrayType;
// class FunctionType;
// class StructType;
// class EnumType;

// // Type: Everything about common behavior
// // 1. knows everything about type
// // 2. knows sizeof(object)
// class TypeBase
// {
//     friend TypeFactory;
//     friend StructType;
//     friend EnumType;

//    protected:
//     ETypeClass _headtype, _tailtype;
//     bool _complete, _is_arithmetic, _is_integral;
//     string _desc;  // complete type info description string

//     TypeBase(ETypeClass type, bool complete)
//         : _headtype(type),
//           _tailtype(type),
//           _complete(complete),
//           _is_arithmetic(false),
//           _is_integral(false)
//     {
//     }

//    public:
//     bool equal(const TypeBase &o) const
//     {
//         return _desc == o._desc;
//     }
//     ETypeClass type() const
//     {
//         return _headtype;
//     }
//     ETypeClass tailType() const
//     {
//         return _tailtype;
//     }

//     // bool lvalue() const;
//     bool hasOperation(ETypeOperations op) const;
//     bool isCompatible(const TypeBase &o) const
//     {
//         return type() == o.type();
//     }
//     bool isArithmetic() const
//     {
//         return _is_arithmetic;
//     }
//     bool isIntegral() const
//     {
//         return _is_integral;
//     }

//     bool isIncomplete(const Environment *env, bool recursive = true) const;
//     bool isIncompleteSimple() const
//     {
//         return !_complete;
//     }

//     string toString() const
//     {
//         return _desc;
//     }

//     // debug
//     static string DebugTypeClass(ETypeClass tc)
//     {
//         string s = "<unknown>";
//         switch (tc)
//         {
//             case T_NONE: s = "NONE"; break;
//             case T_VOID: s = "VOID"; break;
//             case T_CHAR: s = "CHAR"; break;
//             case T_INT: s = "INT"; break;
//             case T_FLOAT: s = "FLOAT"; break;
//             case T_POINTER: s = "POINTER"; break;
//             case T_ARRAY: s = "ARRAY"; break;
//             case T_STRUCT: s = "STRUCT"; break;
//             case T_ENUM: s = "ENUM"; break;
//             case T_FUNCTION: s = "FUNCTION"; break;
//             case T_LABEL: s = "LABEL"; break;
//             default: break;
//         }
//         return s;
//     }
//     static string DebugType(const TypeBase *t)
//     {
//         if (t == nullptr)
//             return "<nullptr>";
//         else
//             return DebugTypeClass(t->type());
//     }
// };

// class VoidType : public TypeBase
// {
//     friend TypeBase;
//     friend TypeFactory;
// };
// class CharType : public TypeBase
// {
//     friend TypeBase;
//     friend TypeFactory;
// };
// class IntegerType : public TypeBase
// {
//     friend TypeBase;
//     friend TypeFactory;
// };
// class FloatingType : public TypeBase
// {
//     friend TypeBase;
//     friend TypeFactory;
// };
// // TODO: conversion: get target type
// class PointerType : public TypeBase
// {
//     friend TypeBase;
//     friend TypeFactory;
// };
// class ArrayType : public TypeBase
// {
//     friend TypeBase;
//     friend TypeFactory;
// };
// class FunctionType : public TypeBase
// {
//     friend TypeBase;
//     friend TypeFactory;
// };
// class StructType : public TypeBase
// {
//     friend TypeBase;
//     friend TypeFactory;

//     bool isIncomplete(const Environment *env, bool recursive = true) const;
//     const TypeBase *getDefinition(const Environment *env, bool recursive) const;
//     StringRef getTag() const;
// };
// class EnumType : public TypeBase
// {
//     friend TypeBase;
//     friend TypeFactory;

//     bool isIncomplete(const Environment *env, bool recursive = true) const;
//     const TypeBase *getDefinition(const Environment *env, bool recursive) const;
//     StringRef getTag() const;
// };
// class TypedefType : public TypeBase
// {
//     friend TypeBase;
//     friend TypeFactory;
// };

// // Type System
// // TODO: conversion: get primitive type
// class TypeFactory
// {
//     static vector<TypeBase *> references;

//    public:
//     static size_t size()
//     {
//         return references.size();
//     }
//     static ETypeClass TypeFromChar(char c)
//     {
//         ETypeClass t = T_NONE;
//         switch (c)
//         {
//             case 'v': t = T_VOID; break;
//             case 'c': t = T_CHAR; break;
//             case 'i': t = T_INT; break;
//             case 'f': t = T_FLOAT; break;
//             case 'P': t = T_POINTER; break;
//             case 'F': t = T_FUNCTION; break;
//             case 'A': t = T_ARRAY; break;
//             case 'E': t = T_ENUM; break;
//             case 'S': t = T_STRUCT; break;
//             case 'T': t = T_TYPEDEF; break;
//             default: SyntaxError("TypeFactory: unknown type class"); break;
//         }
//         return t;
//     }
//     static TypeBase *newInstance()
//     {
//         TypeBase *t = new TypeBase(T_NONE, true);
//         references.push_back(t);
//         return t;
//     }
//     static TypeBase *newInstanceFromCStr(const char *s, size_t length)
//     {
//         assert(s != nullptr);
//         TypeBase *t = new TypeBase(T_NONE, true);
//         t->_desc = string(s, length);
//         references.push_back(t);
//         return t;
//     }
//     static TypeBase *cloneType(const TypeBase &t)
//     {
//         TypeBase *c = new TypeBase(T_NONE, false);
//         *c = t;
//         references.push_back(c);
//         return c;
//     }
//     // used by TypeFactory::Sizeof()
//     // just recreate, no unbox.
//     static TypeBase *ReadType(const char *&s, const Environment *env)
//     {
//         TypeBase *t = nullptr;
//         size_t i;
//         int ret;
//         const char *p;
//         switch (TypeFromChar(*s))
//         {
//             case T_VOID:
//                 t = Void();
//                 ++s;
//                 break;
//             case T_CHAR:
//                 t = Char();
//                 ++s;
//                 break;
//             case T_INT:
//                 t = Integer(*(s + 1) - '0', *s == 'i');
//                 s += 2;
//                 break;
//             case T_FLOAT:
//                 t = Float(*(s + 1) - '0');
//                 s += 2;
//                 break;
//             case T_POINTER:
//                 t = Pointer(*(s + 1) == 'C');
//                 s += (*(s + 1) == 'C') ? 2 : 1;
//                 AppendRight(*t, *ReadType(s, env), env);
//                 break;
//             case T_ARRAY:
//                 ++s;
//                 if (*s == '*')
//                 {
//                     t = ArrayIncomplete();
//                     ++s;
//                 }
//                 else
//                 {
//                     ret = sscanf(s, "%lu", &i);
//                     assert(ret > 0);
//                     while (isdigit(*s))
//                         ++s;
//                     t = Array(i);
//                 }
//                 AppendRight(*t, *ReadType(s, env), env);
//                 break;
//             case T_FUNCTION:
//                 t = Function();
//                 FunctionBegin(t);
//                 ++s;
//                 while (*s != '$')
//                 {
//                     FunctionParameter(t, *ReadType(s, env));
//                 }
//                 ++s;
//                 FunctionEnd(t);
//                 AppendRight(*t, *ReadType(s, env), env);
//                 break;
//             case T_STRUCT:
//                 ++s;
//                 for (p = s; *p != '$'; ++p)
//                 {
//                 }
//                 t = StructTag(StringRef(s, p - s));
//                 s = p + 1;
//                 break;
//             case T_ENUM:
//                 ++s;
//                 for (p = s; *p != '$'; ++p)
//                 {
//                 }
//                 t = EnumTag(StringRef(s, p - s));
//                 s = p + 1;
//                 break;
//             case T_TYPEDEF:
//             // typedef will unbox automatically, should not
//             // appear here.
//             default:
//                 SyntaxError("ReadType: unknown type: " +
//                             StringRef(s, strlen(s)).toString());
//                 break;
//         }
//         return t;
//     }
//     static TypeBase *ReadTypedef(const TypeBase &t, const Environment *env)
//     {
//         assert(t.type() == T_TYPEDEF);
//         const char *s = t._desc.data() + 1;
//         return ReadType(s, env);
//     }

//     // Merge definitions: struct, enum
//     // TODO: array, union, function, typedef(can't merge)
//     static bool MergeDefinition(TypeBase &left, const TypeBase &right)
//     {
//         if (!left.isCompatible(right))
//         {
//             SyntaxWarning("Failed to merge type, not compatible");
//             return false;
//         }
//         if (left._complete && right._complete)
//         {
//             SyntaxWarning("Failed to merge type, duplicate definition");
//             return false;
//         }
//         // left=incomplete, right=complete/incomplete
//         if (right._complete)
//             left = right;
//         else  // left=complete, right=incomplete
//             ;

//         return true;
//     }
//     // Append types (pointer to, array of, function return)
//     static void AppendRight(TypeBase &left, const TypeBase &right,
//                             const Environment *env)
//     {
//         switch (left.tailType())
//         {
//             case T_INT:
//             case T_FLOAT:
//             case T_STRUCT:
//             case T_ENUM:
//             case T_TYPEDEF:
//                 SyntaxError("TypeFactory: " +
//                             TypeBase::DebugTypeClass(left.tailType()) +
//                             " is not derived type");
//                 break;
//             case T_POINTER:
//             case T_FUNCTION:
//                 left._desc += right._desc;
//                 left._tailtype = right._tailtype;
//                 break;
//             case T_ARRAY:
//                 if (right.isIncomplete(env))
//                     SyntaxError("ArrayType: element type is incomplete.");
//                 left._desc += right._desc;
//                 left._tailtype = right._tailtype;
//                 break;
//             case T_NONE: left = right; break;
//             default:
//                 SyntaxError("TypeFactory: append unknown type: " +
//                             left.toString());
//                 break;
//         }
//     }

//     // Type create: only create type object here
//     // AppendRight(): typedef of
//     static TypeBase *Typedef(TypeBase *target)
//     {
//         TypeBase *t = new TypeBase(T_TYPEDEF, true);
//         t->_desc += 'T';
//         t->_desc += target->_desc;
//         references.push_back(t);
//         return t;
//     }
//     static TypeBase *Void()
//     {
//         TypeBase *t = new TypeBase(T_VOID, false);
//         t->_desc += 'v';
//         references.push_back(t);
//         return t;
//     }
//     static TypeBase *Char()
//     {
//         TypeBase *t = new TypeBase(T_CHAR, true);
//         t->_desc += 'c';
//         t->_is_arithmetic = true;
//         t->_is_integral = true;
//         references.push_back(t);
//         return t;
//     }
//     static TypeBase *Integer(size_t length, bool is_signed)
//     {
//         TypeBase *t = new TypeBase(T_INT, true);
//         assert(length <= 8);
//         t->_desc += is_signed ? 'i' : 'u';
//         t->_desc += (char)('0' + length);
//         t->_is_arithmetic = true;
//         t->_is_integral = true;
//         references.push_back(t);
//         return t;
//     }
//     static TypeBase *Float(size_t length)
//     {
//         TypeBase *t = new TypeBase(T_FLOAT, true);
//         assert(length <= 8);
//         t->_desc += 'f';
//         t->_desc += (char)('0' + length);
//         t->_is_arithmetic = true;
//         references.push_back(t);
//         return t;
//     }
//     // AppendRight(): pointer to
//     static TypeBase *Pointer(bool is_const)
//     {
//         TypeBase *t = new TypeBase(T_POINTER, true);
//         t->_desc += 'P';
//         if (is_const)
//             t->_desc += 'C';
//         references.push_back(t);
//         return t;
//     }
//     static TypeBase *StringLiteral()
//     {
//         TypeBase *t = new TypeBase(T_POINTER, true);
//         // TODO: should be const char * const
//         t->_desc += "PCc";
//         references.push_back(t);
//         return t;
//     }
//     // AppendRight(): array of
//     static TypeBase *Array(size_t length)
//     {
//         TypeBase *t = new TypeBase(T_ARRAY, true);
//         t->_desc += 'A';
//         t->_desc += to_string(length);
//         references.push_back(t);
//         return t;
//     }
//     static TypeBase *ArrayIncomplete()
//     {
//         TypeBase *t = new TypeBase(T_ARRAY, false);
//         t->_desc += 'A';
//         t->_desc += '*';
//         references.push_back(t);
//         return t;
//     }
//     // array to pointer
//     static TypeBase *ArrayDecay(TypeBase *t)
//     {
//         assert(t->type() == T_ARRAY);
//         TypeBase *decay = Pointer(false);
//         const char *s = t->_desc.data() + 1;
//         if (*s == '*')
//             ++s;
//         else
//         {
//             while (isdigit(*s))
//                 ++s;
//         }
//         AppendRight(*decay, *ReadType(s, nullptr), nullptr);
//         references.push_back(decay);
//         return decay;
//     }
//     // AppendRight(): function return
//     static TypeBase *Function()
//     {
//         TypeBase *t = new TypeBase(T_FUNCTION, false);
//         t->_desc += 'F';
//         t->_desc += '$';
//         references.push_back(t);
//         return t;
//     }
//     static void FunctionBegin(TypeBase *t)
//     {
//         t->_desc.clear();
//         t->_desc += 'F';
//     }
//     static void FunctionParameter(TypeBase *t, const TypeBase &param)
//     {
//         t->_desc += param._desc;
//     }
//     static void FunctionEnd(TypeBase *t)
//     {
//         t->_desc += '$';
//     }
//     static void FunctionBodyBegin(TypeBase *t)
//     {
//     }
//     static void FunctionBodyEnd(TypeBase *t)
//     {
//         t->_complete = true;
//     }
//     static void FunctionDeclarationCheck(TypeBase *t, const Environment *env,
//                                          StringRef name)
//     {
//         assert(t && t->type() == T_FUNCTION);
//         const char *s = t->_desc.data() + 1;
//         while (*s != '$')
//         {
//             assert(*s != '\0');
//             TypeBase *param = ReadType(s, env);
//             if (param->isIncomplete(env))
//                 SyntaxError("function '" + name.toString() +
//                             "' has incomplete parameter type: " +
//                             param->toString());
//         }
//         ++s;
//         assert(*s != '\0');
//         TypeBase *ret = ReadType(s, env);
//         if (ret->isIncomplete(env) && ret->type() != T_VOID)
//             SyntaxError("function '" + name.toString() +
//                         "' has incomplete return type: " + ret->toString());
//         if (ret->type() == T_FUNCTION || ret->type() == T_ARRAY)
//             SyntaxError("function '" + name.toString() +
//                         "' has invalid return type: " + ret->toString());
//     }
//     static TypeBase *StructTag(StringRef tag)
//     {
//         TypeBase *t =
//             new TypeBase(T_STRUCT, false);  // update when call incomplete()
//         t->_desc += 'S';
//         t->_desc += tag.toString();
//         t->_desc += '$';
//         references.push_back(t);
//         return t;
//     }
//     static TypeBase *Struct()
//     {
//         TypeBase *t = new TypeBase(T_STRUCT, false);
//         references.push_back(t);
//         return t;
//     }
//     static void StructBegin(TypeBase *t)
//     {
//         t->_desc += 'S';
//     }
//     static void StructBodyBegin(TypeBase *t)
//     {
//     }
//     static void StructMember(TypeBase *t, const TypeBase &member,
//                              const Environment *env)
//     {
//         if (member.isIncomplete(env))
//             SyntaxError("StructType: member type is incomplete.");
//         t->_desc += member._desc;
//     }
//     static void StructBodyEnd(TypeBase *t)
//     {
//         t->_complete = true;
//     }
//     static void StructEnd(TypeBase *t)
//     {
//         t->_desc += '$';
//     }

//     static TypeBase *EnumTag(StringRef tag)
//     {
//         TypeBase *t =
//             new TypeBase(T_ENUM, false);  // update when call incomplete()
//         t->_desc += 'E';
//         t->_desc += tag.toString();
//         t->_desc += '$';
//         references.push_back(t);
//         return t;
//     }
//     static TypeBase *Enum(StringRef tag)
//     {
//         TypeBase *t = new TypeBase(T_ENUM, true);
//         t->_desc += 'E';
//         t->_desc += tag.toString();
//         t->_desc += '$';
//         references.push_back(t);
//         return t;
//     }

//     // Primitive Types
//     static const TypeBase *Primitive_Int()
//     {
//         static TypeBase *t = nullptr;
//         if (t == nullptr)
//             t = Integer(4, true);
//         return t;
//     }
//     static const TypeBase *Primitive_UInt()
//     {
//         static TypeBase *t = nullptr;
//         if (t == nullptr)
//             t = Integer(4, false);
//         return t;
//     }
//     static const TypeBase *Primitive_Long()
//     {
//         static TypeBase *t = nullptr;
//         if (t == nullptr)
//             t = Integer(sizeof(long), true);
//         return t;
//     }
//     static const TypeBase *Primitive_ULong()
//     {
//         static TypeBase *t = nullptr;
//         if (t == nullptr)
//             t = Integer(sizeof(long), false);
//         return t;
//     }
//     static const TypeBase *Primitive_Char()
//     {
//         static TypeBase *t = nullptr;
//         if (t == nullptr)
//             t = Char();
//         return t;
//     }

//     // Helpers for information extraction
//     static size_t ArraySize(const TypeBase &t, const Environment *env);
//     static size_t StructSize(const TypeBase &t, const Environment *env);
//     static size_t Sizeof(const TypeBase &t, const Environment *env);
//     static void ConstructObject(Object *&o, const TypeBase &t,
//                                 const Environment *env);
// };
