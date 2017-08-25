#pragma once

#include "common.h"
// #include "lexer.h"
// #include "type.h"
#include "symbol.h"
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
    // vector<StringRef> slabels, elabels;

   public:
    Environment() : id(idgen++)
    {
    }
    void debugPrint(int indent = 0) const;

    // symbol, namespace, scope
    Symbol *findSymbol(ESymbolNamespace space, StringRef name) const;
    // Symbol *recursiveFind(ESymbolNamespace space, StringRef name) const;
    // Symbol *findDefinition(StringRef name) const;
    // Symbol *recursiveFindDefinition(StringRef name) const;
    // Symbol *recursiveFindTypename(StringRef name) const;
    void addSymbol(Symbol *s);
    static const Symbol *SameNameSymbolInFileScope(const Environment *env,
                                                   const Type *type,
                                                   const StringRef name);

    // code generation
    // void emit();
    // void pushLabel(StringRef start, StringRef end);
    // void popLabel();
    // StringRef startLabel() const;
    // StringRef endLabel() const;
};
