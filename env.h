#pragma once

#include "common.h"
// #include "lexer.h"
// #include "type.h"
#include "symbol.h"
// #include "codegen.h"
#include "ir.h"

// Environment do:
//
// 0. manage symbols
//
// 1. manage objects and their locations
// 2. manage temporaries and their locations
//    (temporary count is computed by sn_expression)
// 3. manage constants and their locations
//
class Environment : public TreeLike<Environment>
{
    friend class TypeTester;

    static int idgen;
    int id;
    std::vector<Symbol *> symbols;
    IRStorage storage;

   public:
    Environment() : id(idgen++) {}
    void debugPrint() const;
    std::string DebugString() const;

    // symbol management
    Symbol *findSymbol(ESymbolNamespace space, StringRef name) const;
    Symbol *findSymbolRecursive(ESymbolNamespace space, StringRef name) const;
    // Symbol *findDefinition(StringRef name) const;
    // Symbol *recursiveFindDefinition(StringRef name) const;
    // Symbol *recursiveFindTypename(StringRef name) const;
    void addSymbol(Symbol *s);

    static const Symbol *SameNameSymbolInFileScope(const Environment *env,
                                                   const Type *type,
                                                   const StringRef name);

    // Delegate below functions to IRStorage
    // > object management
    // > constant management
    // > temporary management (only live within a statement)
    // > code generation
    IRStorage &getStorage()
    {
        return storage;
    }

    // Translation
    void traverse(IRTranslator &t) const;
};
