#pragma once

#include "common.h"

class SyntaxNode;
class Object;

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

enum ETypeClass
{
    T_NONE,
    T_INT,
    T_FLOAT,
    T_POINTER,
    T_ARRAY,
    T_STRUCT,
    T_ENUM,
    T_FUNCTION,
    T_LABEL
};

class TypeFactory;
class PointerType;

// Type: Everything about common behavior
// 1. knows everything about type
// 2. knows sizeof(object)
class TypeBase
{
    friend TypeFactory;

   protected:
    // StringRef desc;  // complete type info description string
    string desc;
    bool complete;

   public:
    bool equal(const TypeBase &o) const
    {
        return desc == o.desc;
    }
    bool isIncomplete() const
    {
        return !complete;
    }
    // bool lvalue() const;
    bool hasOperation(ETypeOperations op) const;
    bool isCompatible(const TypeBase &o) const
    {
        return type() == o.type();
    }
    ETypeClass type() const
    {
        char c = desc.empty() ? '\0' : desc[0];
        ETypeClass tc = T_NONE;
        switch (c)
        {
            case 'i': tc = T_INT; break;
            case 'f': tc = T_FLOAT; break;
            case 'P': tc = T_POINTER; break;
            case 'A': tc = T_ARRAY; break;
            case 'F': tc = T_FUNCTION; break;
            case 'S': tc = T_STRUCT; break;
            case 'E': tc = T_ENUM; break;
            default: break;
        }
        return tc;
    }
    TypeBase &operator+=(const char c)
    {
        desc += c;
        return *this;
    }
    TypeBase &operator+=(const string &s)
    {
        desc += s;
        return *this;
    }
    TypeBase &operator+=(const TypeBase &o)
    {
        desc += o.desc;
        return *this;
    }

    const PointerType * asPointerType() const;

    string toString() const
    {
        return desc;
    }
    static size_t Sizeof(const TypeBase &);
    static void ConstructObject(Object *&, const TypeBase &);

    // debug
    static string DebugTypeClass(ETypeClass tc)
    {
        string s = "<unknown>";
        switch (tc)
        {
            case T_NONE: s = "NONE"; break;
            case T_INT: s = "INT"; break;
            case T_FLOAT: s = "FLOAT"; break;
            case T_POINTER: s = "POINTER"; break;
            case T_ARRAY: s = "ARRAY"; break;
            case T_STRUCT: s = "STRUCT"; break;
            case T_ENUM: s = "ENUM"; break;
            case T_FUNCTION: s = "FUNCTION"; break;
            case T_LABEL: s = "LABEL"; break;
            default: break;
        }
        return s;
    }
    static string DebugType(const TypeBase *t)
    {
        if (t == nullptr)
            return "<nullptr>";
        else
            return DebugTypeClass(t->type());
    }

    // private:
    //  NON_COPYABLE(TypeBase)
};

class PointerType : public TypeBase
{
    friend TypeBase;

   public:
    const TypeBase *target() const;

   private:
    NON_COPYABLE(PointerType)
};

// Type System
class TypeFactory
{
    static vector<TypeBase *> references;

   public:
    static TypeBase *newInstance()
    {
        TypeBase *t = new TypeBase();
        t->complete = true;
        references.push_back(t);
        return t;
    }
    static TypeBase *newInstanceFromCStr(const char *s, size_t length)
    {
        assert(s != nullptr);
        TypeBase *t = new TypeBase();
        t->desc = string(s, length);
        t->complete = true;
        references.push_back(t);
        return t;
    }

    // Merge types
    static bool MergeRight(TypeBase &left, const TypeBase &right)
    {
        if (!left.isCompatible(right))
        {
            SyntaxWarning("Failed to merge type, not compatible");
            return false;
        }
        if (!(left.isIncomplete() || right.isIncomplete()))
        {
            SyntaxWarning("Failed to merge type, duplicate definition");
            return false;
        }

        if (left.isIncomplete())  // left=incomplete, right=complete/incomplete
            left = right;
        else  // left=complete, right=incomplete
            ;

        return true;
    }

    // Callback functions in building type
    static void Void(TypeBase &t)
    {
        t += 'v';
    }
    static void Char(TypeBase &t)
    {
        t += 'c';
    }
    static void Integer(TypeBase &t, bool is_signed, size_t length)
    {
        assert(length <= 8);
        t += is_signed ? 'i' : 'u';
        t += (char)('0' + length);
    }
    static void Float(TypeBase &t, size_t length)
    {
        assert(length <= 8);
        t += 'f';
        t += (char)('0' + length);
    }
    static void Pointer(TypeBase &t, bool is_const)
    {
        t += 'P';
        if (is_const)
            t += 'C';
    }
    static void FunctionStart(TypeBase &t)
    {
        t += 'F';
        t += '(';
    }
    static void FunctionParameter(TypeBase &t, const TypeBase &param)
    {
        t += param;
        t += ',';
    }
    static void FunctionEnd(TypeBase &t)
    {
        t += ')';
    }
    static void ArrayStart(TypeBase &t)
    {
        t += 'A';
    }
    static void ArrayParameter(TypeBase &t, size_t len)
    {
        t += '_';
        t += to_string(len);
    }
    static void ArrayEnd(TypeBase &t)
    {
        t += '$';
    }
    static void StructStart(TypeBase &t)
    {
        t += 'S';
        t.complete = false;
    }
    static void StructName(TypeBase &t, StringRef name)
    {
        // do nothing
    }
    static void StructBodyBegin(TypeBase &t)
    {
        t.complete = true;
    }
    static void StructMember(TypeBase &t, const TypeBase &member)
    {
        t += '_';
        t += member;
        t.complete = t.complete && member.complete;
    }
    static void StructBodyEnd(TypeBase &t)
    {
    }
    static void StructEnd(TypeBase &t)
    {
        t += '$';
    }
    static void EnumStart(TypeBase &t)
    {
        t += 'E';
        t.complete = false;
    }
    static void EnumName(TypeBase &t, StringRef name)
    {
        t += name.toString();
    }
    static void EnumBodyBegin(TypeBase &t)
    {
        // t += '{';
        t.complete = true;
    }
    static void EnumBodyEnd(TypeBase &t)
    {
        // t += '}';
    }
    static void EnumEnd(TypeBase &t)
    {
        t += '$';
    }

    // Helpers for information extraction
    static size_t ArraySize(const TypeBase &t);
    static size_t StructSize(const TypeBase &t);
};
