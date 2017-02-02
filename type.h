#pragma once

#include <climits>
#include <vector>
using namespace std;

// #include "parser.h"
// #include "env.h"
#include "common.h"
#include "symbol.h"

class SyntaxNode;
class Environment;

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

class IntType;

class OperationMask
{
    int mask;

   protected:
    OperationMask() : mask(0)
    {
    }
    void addOperation(ETypeOperations op)
    {
        mask |= op;
    }
    void removeOperation(ETypeOperations op)
    {
        mask &= ~op;
    }

   public:
    bool hasOperation(ETypeOperations op) const
    {
        return (mask & op);
    }
};
template <typename T>
class Additive : virtual public OperationMask
{
   public:
    Additive<T>()
    {
        addOperation(TOp_ADD);
        addOperation(TOp_SUB);
        addOperation(TOp_INC);
        addOperation(TOp_DEC);
    }
    virtual void add(T *){/* debug */};
    virtual void sub(T *){/* debug */};
    virtual void inc(T *){/* debug */};
    virtual void dec(T *){/* debug */};
};
template <typename T>
class Multiplicative : virtual public OperationMask
{
   public:
    Multiplicative<T>()
    {
        addOperation(TOp_MUL);
        addOperation(TOp_DIV);
        addOperation(TOp_MOD);
    }
    virtual void mul(T *){/* debug */};
    virtual void div(T *){/* debug */};
    virtual void mod(T *){/* debug */};
};
template <typename T>  // r-value
class Evaluable : virtual public OperationMask
{
   public:
    Evaluable<T>()
    {
        addOperation(TOp_EVAL);
    }
    virtual T *eval()
    { /* debug */
        return nullptr;
    };
};
template <typename T>  // l-value
class Addressable : virtual public OperationMask
{
   public:
    Addressable<T>()
    {
        addOperation(TOp_ADDR);
    }
    virtual void *addr()
    { /* debug */
        return nullptr;
    };
};
template <typename T>  // l-value
class Assignable : virtual public OperationMask
{
   public:
    Assignable<T>()
    {
        addOperation(TOp_ASSIGN);
    }
    virtual void assign(T *){/* debug */};
};
class Indexable : virtual public OperationMask
{
   public:
    Indexable()
    {
        addOperation(TOp_INDEX);
    }
    virtual void index(IntType *){/* debug */};
    virtual TypeBase *indexedType() const
    {
        return nullptr;
    }
};
template <typename T>  // struct member access
class Offsetable : virtual public OperationMask
{
   public:
    Offsetable<T>()
    {
        addOperation(TOp_OFFSET);
    }
    virtual void offset(StringRef){/* debug */};
};
class Callable : virtual public OperationMask
{
   public:
    Callable()
    {
        addOperation(TOp_CALL);
    }
};

// Group class
template <typename T>
class Scalar : public Additive<T>, public Multiplicative<T>
{
};
template <typename T>
class LValue : public Evaluable<T>, public Addressable<T>, public Assignable<T>
{
};
template <typename T>
class RValue : public Evaluable<T>, public Addressable<T>
{
};

// Type Representation
enum ETypeClass
{
    TC_VOID,
    TC_INT,
    TC_FLOAT,  // TC_DOUBLE
    TC_POINTER,
    TC_ARRAY,
    TC_FUNC,
    TC_STRUCT,
    TC_UNION,
    TC_ENUM
};

class TypeBase : public ListLike<TypeBase>, virtual public OperationMask
{
   public:
    // static TypeBase *merge(TypeBase *l, TypeBase *r);
    // static TypeBase *convert(TypeBase *from, TypeBase *to);
    virtual ETypeClass type() const = 0;
    virtual long size() const = 0;
    virtual bool equal(const TypeBase &other) const = 0;
    static string DebugTypeClass(ETypeClass tc)
    {
        switch (tc)
        {
            case TC_INT: return "int";
            case TC_FLOAT: return "float";
            case TC_POINTER: return "pointer";
            case TC_ARRAY: return "array";
            case TC_FUNC: return "func";
            case TC_STRUCT: return "struct";
            case TC_UNION: return "union";
            case TC_ENUM: return "enum";
            case TC_VOID: return "void";
            default: break;
        }
        return "<null>";
    }
    static string DebugType(TypeBase *t)
    {
        if (t == nullptr)
            return "<nullptr>";
        else
            return DebugTypeClass(t->type());
    }
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
    virtual bool equal(const TypeBase &other) const
    {
        return other.type() == TC_VOID;
    }
};
class IntType : public TypeBase, public LValue<IntType>, public Scalar<IntType>
{
    // NON_COPYABLE(IntType)
    friend class TypeTable;

    int bytes;
    bool signed_;

   protected:
    IntType() = default;
    IntType(const IntType &o)
    {
        bytes = o.bytes;
        signed_ = o.signed_;
    }
    IntType(IntType &&o)
    {
        bytes = o.bytes;
        signed_ = o.signed_;
    }
    IntType(int size, bool is_signed) : bytes(size), signed_(is_signed)
    {
        switch (size)
        {
            case 1:
            case 2:
            case 4:
            case 8: break;
            default:
                SyntaxError("Invalid IntType bytes:" + to_string(size));
                break;
        }
    }

   public:
    bool isSigned() const
    {
        return signed_;
    }
    virtual ETypeClass type() const
    {
        return TC_INT;
    }
    virtual long size() const
    {
        return bytes;
    }
    virtual bool equal(const TypeBase &other) const
    {
        if (type() == other.type())
        {
            const IntType &ref = dynamic_cast<const IntType &>(other);
            return bytes == ref.bytes && signed_ == ref.signed_;
        }
        return false;
    }
    string toString() const
    {
        string s;
        if (!signed_)
            s += "unsigned ";
        switch (bytes)
        {
            case 1: s += "char"; break;
            case 2: s += "short"; break;
            case 4: s += "int"; break;
            case 8: s += "long"; break;
            default: break;
        }
        return s;
    }
};
// TC_FLOAT: float
// TC_DOUBLE: double
// TC_LDOUBLE: long double
class FloatType : public TypeBase,
                  public LValue<FloatType>,
                  public Scalar<FloatType>
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
    virtual bool equal(const TypeBase &other) const
    {
        return other.type() == TC_FLOAT;
    }
};
class PointerType : public IntType, public Indexable
{
   public:
    PointerType()
    {
        removeOperation(TOp_MUL);
        removeOperation(TOp_DIV);
        removeOperation(TOp_MOD);
    }
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
    virtual bool equal(const TypeBase &other) const
    {
        if (other.type() != TC_POINTER)
            return false;
        const PointerType &o = dynamic_cast<const PointerType &>(other);
        return target()->equal(*(o.target()));
    }
};
class ConstPointerType : public TypeBase,
                         public Evaluable<PointerType>,
                         public Indexable
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
    virtual bool equal(const TypeBase &other) const
    {
        if (other.type() != TC_POINTER)
            return false;
        const PointerType &o = dynamic_cast<const PointerType &>(other);
        return target()->equal(*(o.target()));
    }
};
// Array is const-pointer, or an address constant
class ArrayType : public TypeBase, public RValue<ArrayType>, public Indexable
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
    virtual bool equal(const TypeBase &other) const
    {
        if (other.type() != TC_ARRAY)
            return false;
        const ArrayType &o = dynamic_cast<const ArrayType &>(other);
        return length == o.length && target()->equal(*(o.target()));
    }

    // from Indexable
    virtual TypeBase *indexedType() const
    {
        return target();
    }
};
// Function is const-pointer, or an address constant
class FuncType : public TypeBase, public RValue<FuncType>, public Callable
{
   public:
    SyntaxNode *body;
    Environment *env;

    FuncType() : body(nullptr), env(nullptr)
    {
    }
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
    virtual bool equal(const TypeBase &other) const
    {
        return other.type() == TC_FUNC;
    }
};
class StructType : public TypeBase,
                   public LValue<StructType>,
                   public Offsetable<StructType>
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
    Symbol *getMember(StringRef name) const
    {
        for (auto &m : members)
        {
            if (m->name == name)
                return m;
        }
        return nullptr;
    }
    virtual ETypeClass type() const
    {
        return TC_STRUCT;
    }
    virtual long size() const
    {
        return 4;
    }
    virtual bool equal(const TypeBase &other) const
    {
        return other.type() == TC_STRUCT;
    }
    void debugPrint()
    {
        for (Symbol *m : members)
            cout << '\t' << (*m) << endl;
    }
};
class UnionType : public TypeBase,
                  public LValue<UnionType>,
                  public Offsetable<StructType>
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
    virtual bool equal(const TypeBase &other) const
    {
        return other.type() == TC_UNION;
    }
};
class EnumType : public TypeBase, public LValue<EnumType>
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
    virtual bool equal(const TypeBase &other) const
    {
        return other.type() == TC_ENUM;
    }
};

// Type System
// parse & manage type tree
class TypeTable
{
    // ~~~~running~~~~: how to management function declaration & definition

   public:
    TypeBase *newIntegral(TokenType type, bool is_signed);
    TypeBase *newStringLiteral();
    static const IntType &Char();
    static const IntType &UChar();
    static const IntType &Short();
    static const IntType &UShort();
    static const IntType &Int();
    static const IntType &UInt();
    static const IntType &Long();
    static const IntType &ULong();
};
