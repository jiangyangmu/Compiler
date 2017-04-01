#pragma once

#include "lexer.h"
#include "type.h"
#include "object.h"

class TypeBase;

enum ESymbolNamespace
{
    SC_ID, // object, enum-constant, typedef-name
    SC_LABEL,
    SC_TAG,
};

// Symbol: Everything about name, namespace
struct Symbol
{
    ESymbolNamespace space;
    StringRef name;
    TypeBase *type;
    Object *obj; // if type is incomplete, obj == nullptr

    // friend std::ostream &operator<<(std::ostream &o, const Symbol &s);
    Symbol() : type(nullptr), obj(nullptr) {}
};

class SymbolFactory
{
    static vector<Symbol *> references;
    static Symbol * get()
    {
        references.push_back(new Symbol());
        return references.back();
    }
   public:
    static Symbol *newInstance()
    {
        Symbol *s = get();
        s->type = TypeFactory::newInstance();
        return s;
    }
    static Symbol *newInstance(const Symbol &t)
    {
        Symbol *s = get();
        (*s) = t;
        return s;
    }
    static Symbol *newTag(StringRef name, TypeBase *type)
    {
        Symbol *s = get();
        s->space = SC_TAG;
        s->name = name;
        s->type = type;
        return s;
    }
    static void check()
    {
        for (Symbol *s : references)
        {
            if (s->obj && (s->type == nullptr || s->type->isIncomplete()))
            {
                SyntaxError("Object '" + s->name.toString() + "' is never completed.");
            }
        }
    }
};
