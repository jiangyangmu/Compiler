#pragma once

#include "common.h"
// #include "lexer.h"
// #include "type.h"
#include "ir.h"
#include "symbol.h"

// s/Environment/SymbolTable/

class SymbolTable : public TreeLike<SymbolTable>
{
    friend class TypeTester;

    static int idgen;
    int id;
    std::vector<Symbol *> symbols;
    IRStorage *storage;

   public:
    SymbolTable(IRStorage *s) : id(idgen++), storage(s) {}
    void debugPrint() const;
    std::string DebugString() const;

    // symbol management
    Symbol *findSymbol(ESymbolNamespace space, StringRef name) const;
    Symbol *findSymbolRecursive(ESymbolNamespace space, StringRef name) const;
    // Symbol *findDefinition(StringRef name) const;
    // Symbol *recursiveFindDefinition(StringRef name) const;
    // Symbol *recursiveFindTypename(StringRef name) const;
    void addSymbol(Symbol *s);

    static const Symbol *SameNameSymbolInFileScope(const SymbolTable *env,
                                                   const Type *type,
                                                   const StringRef name);

    // Delegate below functions to IRStorage
    // > object management
    // > constant management
    // > temporary management (only live within a statement)
    // > code generation
    bool hasStorage() const
    {
        return (storage != nullptr);
    }
    IRStorage *getStorage()
    {
        assert(storage != nullptr);
        return storage;
    }

    // Translation
    void traverse(IRTranslator &t) const;
};
