#pragma once

#include "common.h"
#include "lexer.h"
#include "symbol.h"
#include "type.h"
#include "codegen.h"

class Environment : public TreeLike<Environment>
{
    static int idgen;
    int id;
    SymbolTable symbols;
    vector<StringRef> slabels, elabels;

   public:
    TypeTable factory;
    Environment() : id(idgen++) {}

    Symbol *find(ESymbolCategory category, StringRef name) const;
    void add(Symbol *s);
    size_t allSymbolSize() const;
    void debugPrint(Lexer &lex) const;

    // code & label
    void emit();
    void pushLabel(StringRef start, StringRef end);
    void popLabel();
    StringRef startLabel() const;
    StringRef endLabel() const;

    static void ParseLocalDeclaration(Lexer &lex, Environment *env);
    static void ParseGlobalDeclaration(Lexer &lex, Environment *env);
    static TypeBase *ParseTypename(Lexer &lex, Environment *env);
};
