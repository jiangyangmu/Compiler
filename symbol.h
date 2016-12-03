#pragma once

class TypeBase;

// Symbol Representation
// id: name, type, addr
// enum-const: name, type, value
// label: name, addr
// tag: name, type
enum ESymbolCategory
{
    SC_ID,
    SC_ENUM_CONST,
    SC_LABEL,
    SC_TAG
};

#include <iostream>

struct Symbol
{
    ESymbolCategory category;
    StringRef name;
    TypeBase *type;
    union {
        long addr;
        long value;
    };
    friend std::ostream &operator<<(std::ostream &o, const Symbol &s);
};

// Symbol System
// 1. label <-> location
// 2. tag <-> struct/union/enum
// 3. id <-> object/enum-constant
class SymbolTable
{
    vector<Symbol *> symbols;
    // size_t stack_size;

   public:
    void add(Symbol *t)
    {
        symbols.push_back(t);
    }
    Symbol *find(ESymbolCategory category, StringRef name) const
    {
        for (auto s = symbols.rbegin(); s != symbols.rend(); ++s)
        {
            if ((*s)->category == category && (*s)->name == name)
                return *s;
        }
        return nullptr;
    }
    void debugPrint(Lexer &lex) const;
};
