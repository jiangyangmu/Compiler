#pragma once

#include <climits>
#include <vector>
using namespace std;

// #include "parser.h"
// #include "env.h"
#include "symbol.h"

class SyntaxNode;
class Environment;

// bool uadd_ok(unsigned int u1, unsigned int u2)
// {
//     unsigned int max_add = UINT_MAX - u1;
//     return max_add >= u2;
// }

// bool usub_ok(unsigned int u1, unsigned int u2)
// {
//     return u1 >= u2;
// }
// bool umul_ok(unsigned int u, unsigned int v)
// {
//     unsigned int uv = u * v;
//     return !u || (uv / u == v);
// }

// bool udiv_ok(unsigned int u1, unsigned int u2)
// {
//     return u2 != 0U;
// }
// bool umod_ok(unsigned int u1, unsigned int u2)
// {
//     return u2 != 0U;
// }
// bool tadd_ok(int i1, int i2)
// {
//     if (i2 > 0)
//         return (i1 + i2) > i1;
//     else if (i2 < 0)
//         return (i1 + i2) < i1;
//     else
//         return true;
// }

// bool tsub_ok(int i1, int i2)
// {
//     return (i2 > INT_MIN) && tadd_ok(i1, -i2);
// }
// int tmul_ok(int i1, int i2)
// {
//     int result = i1 * i2;
//     return !i1 || (result / i1 == i2);
// }

// int tdiv_ok(int i1, int i2)
// {
//     if (i2 == 0)
//         return false;
//     if (i1 == INT_MIN && i2 == -1)
//         return false;
//     return true;
// }

// [>
//     11 %  7 == 4
//    -11 %  7 == 3
//     11 % -7 == -3
//    -11 % -7 == -4
//  */
// int tmod_ok(int i1, int i2)
// {
//     return (i1 >= 0 && i2 > 0);
// }
// warning-list: compare unsigned int with int

// Type Representation
enum ETypeClass
{
    TC_VOID,
    TC_INT,    // TC_CHAR, TC_SHORT, TC_LONG, ...
    TC_FLOAT,  // TC_DOUBLE
    TC_POINTER,
    TC_ARRAY,
    TC_FUNC,
    TC_STRUCT,
    TC_UNION,
    TC_ENUM
};

class TypeBase : public ListLike<TypeBase>
{
   public:
    virtual ETypeClass type() const = 0;
    virtual long size() const = 0;
    virtual bool equal(TypeBase &other) const = 0;
};
class VoidType : public TypeBase
{
   public:
    virtual ETypeClass type() const
    {
        return TC_VOID;
    }
    virtual long size() const
    {
        return 0;
    }
    virtual bool equal(TypeBase &other) const
    {
        return other.type() == TC_VOID;
    }
};
// TC_Char: char
// TC_UChar: unsigned char
// TC_SChar: signed char
// TC_Short: (signed) short (int)
// TC_UShort: unsigned short (int)
// TC_Int: (signed) (int)
// TC_UInt: unsigned (int)
// TC_LONG: (signed) long (int)
// TC_ULONG: unsigned long (int)
class IntType : public TypeBase
{
   public:
    // IntType(int bytes, bool is_signed);
    virtual ETypeClass type() const
    {
        return TC_INT;
    }
    virtual long size() const
    {
        return 4;
    }
    virtual bool equal(TypeBase &other) const
    {
        return other.type() == TC_INT;
    }
};
// TC_FLOAT: float
// TC_DOUBLE: double
// TC_LDOUBLE: long double
class FloatType : public TypeBase
{
   public:
    virtual ETypeClass type() const
    {
        return TC_FLOAT;
    }
    virtual long size() const
    {
        return 4;
    }
    virtual bool equal(TypeBase &other) const
    {
        return other.type() == TC_FLOAT;
    }
};
class PointerType : public TypeBase
{
   public:
    TypeBase *target() const
    {
        assert(next() != nullptr);
        return next();
    }
    virtual ETypeClass type() const
    {
        return TC_POINTER;
    }
    virtual long size() const
    {
        return sizeof(void *);
    }
    virtual bool equal(TypeBase &other) const
    {
        if (other.type() != TC_POINTER)
            return false;
        PointerType &o = dynamic_cast<PointerType &>(other);
        return target()->equal(*(o.target()));
    }
};
class ArrayType : public TypeBase
{
   public:
    vector<int> axis;
    int dimen;
    int length;

    TypeBase *target() const
    {
        assert(next() != nullptr);
        return next();
    }
    virtual ETypeClass type() const
    {
        return TC_ARRAY;
    }
    virtual long size() const
    {
        return length * target()->size();
    }
    virtual bool equal(TypeBase &other) const
    {
        if (other.type() != TC_ARRAY)
            return false;
        ArrayType &o = dynamic_cast<ArrayType &>(other);
        return length == o.length && target()->equal(*(o.target()));
    }
};
class FuncType : public TypeBase
{
   public:
    SyntaxNode *body;
    Environment *env;

    TypeBase *rtype() const
    {
        assert(next() != nullptr);
        return next();
    }
    virtual ETypeClass type() const
    {
        return TC_FUNC;
    }
    virtual long size() const
    {
        return 0;
    }
    virtual bool equal(TypeBase &other) const
    {
        return other.type() == TC_FUNC;
    }
};
class StructType : public TypeBase
{
    vector<struct Symbol *> members;

   public:
    void addMember(struct Symbol *s)
    {
        for (auto &m : members)
        {
            if (m->name == s->name)
                SyntaxError("Duplicate member name in struct definition");
        }
        // calculate member location
        // TODO: padding & alignment
        if (members.empty())
            s->addr = 0;
        else
        {
            auto &last = members.back();
            s->addr = last->addr + last->type->size();
        }
        members.push_back(s);
    }
    virtual ETypeClass type() const
    {
        return TC_STRUCT;
    }
    virtual long size() const
    {
        return 4;
    }
    virtual bool equal(TypeBase &other) const
    {
        return other.type() == TC_STRUCT;
    }
    void debugPrint()
    {
        for (Symbol *m : members)
            cout << '\t' << (*m) << endl;
    }
};
class UnionType : public TypeBase
{
    // vector<Symbol> members;

   public:
    virtual ETypeClass type() const
    {
        return TC_UNION;
    }
    virtual long size() const
    {
        return 4;
    }
    virtual bool equal(TypeBase &other) const
    {
        return other.type() == TC_UNION;
    }
};
class EnumType : public TypeBase
{
    // vector<Symbol> members;

   public:
    virtual ETypeClass type() const
    {
        return TC_ENUM;
    }
    virtual long size() const
    {
        return 4;
    }
    virtual bool equal(TypeBase &other) const
    {
        return other.type() == TC_ENUM;
    }
};

// Type System
// parse & manage type tree
class TypeTable
{
    // ~~~~running~~~~: how to management function declaration & definition
    vector<TypeBase *> types;
    // static unordered_map<string, TypeBase *> functions;
    // <target type, self>
    // unordered_map<TypeBase *, TypeBase *> pointers, arrays;
    // unordered_map<string, TypeBase *> enums, structs, unions;

   public:
    // static struct
    // {
    //     IntType i8, ui8, i16, ui16, i32, ui32, i64, ui64;
    //     FloatType f32, f64;
    // } primitives;
    // void add(TypeBase *pt);
};
