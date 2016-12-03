#pragma once

#include "common.h"
#include "lexer.h"
#include "symbol.h"
#include "type.h"

class Environment : public TreeLike<Environment>
{
    static int idgen;
    int id;
    SymbolTable symbols;

   public:
    TypeTable factory;
    Environment() : id(idgen++) {}

    Symbol *find(ESymbolCategory category, StringRef name) const;
    void add(Symbol *s);
    void debugPrint(Lexer &lex) const;

    static void ParseLocalDeclaration(Lexer &lex, Environment *env);
    static void ParseGlobalDeclaration(Lexer &lex, Environment *env);
    static TypeBase *ParseTypename(Lexer &lex, Environment *env);
};
