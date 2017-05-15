#pragma once

#include "common.h"
#include "lexer.h"
#include "symbol.h"
#include "type.h"
// #include "codegen.h"

// Environment do:
// 1. manage symbol, namespace, scope
// 2. manage location for break/continue
// Environment = Tree of Declarations
class Environment : public TreeLike<Environment>
{
    static int idgen;
    int id;
    vector<Symbol *> symbols;
    vector<StringRef> slabels, elabels;

   public:
    Environment() : id(idgen++) {}
    // string toString() const;
    void debugPrint(int indent = 0) const;

    // symbol, namespace, scope
    Symbol *find(ESymbolNamespace space, StringRef name) const;
    Symbol *recursiveFind(ESymbolNamespace space, StringRef name) const;
    Symbol *recursiveFindDefinition(StringRef name) const;
    void add(Symbol *s);

    // code generation
    void emit();
    void pushLabel(StringRef start, StringRef end);
    void popLabel();
    StringRef startLabel() const;
    StringRef endLabel() const;

    static void ParseLocalDeclaration(Lexer &lex, Environment *env);
    static void ParseGlobalDeclaration(Lexer &lex, Environment *env);
    // static TypeBase *ParseTypename(Lexer &lex, Environment *env);
};
