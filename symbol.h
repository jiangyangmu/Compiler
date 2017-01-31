#pragma once

#include "lexer.h"

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
        long addr; // struct offset
        long value; // enum value
        long position; // parameter position
    };
    friend std::ostream &operator<<(std::ostream &o, const Symbol &s);
};

// Symbol System
// 1. label <-> location
// 2. tag <-> struct/union/enum
// 3. id <-> object/enum-constant
class SymbolTable
{
    friend class Environment;

    vector<Symbol *> symbols;
    size_t size_;

   public:
    SymbolTable() : size_(0)
    {
    }
    void add(Symbol *t);
    Symbol *find(ESymbolCategory category, StringRef name) const;
    size_t size() const
    {
        return size_;
    }
    void debugPrint(Lexer &lex) const;
};
