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
    static int idgen;
    int id;
    vector<Symbol *> symbols;
    // vector<StringRef> slabels, elabels;

   public:
    Environment() : id(idgen++)
    {
    }
    void debugPrint(int indent = 0) const;

    // symbol management
    Symbol *findSymbol(ESymbolNamespace space, StringRef name) const;
    // Symbol *recursiveFind(ESymbolNamespace space, StringRef name) const;
    // Symbol *findDefinition(StringRef name) const;
    // Symbol *recursiveFindDefinition(StringRef name) const;
    // Symbol *recursiveFindTypename(StringRef name) const;
    void addSymbol(Symbol *s);
    static const Symbol *SameNameSymbolInFileScope(const Environment *env,
                                                   const Type *type,
                                                   const StringRef name);

    // object management
    OperandAddress findObjectAddress(StringRef name) const;

    // constant management
    // int findConstantLocation(...) const;

    // temporary management (only live within a statement)
    // int findTemporaryLocation(size_t index) const;
    // OperandAddress allocTemporary();
    // void freeTemporary(Operandaddress &addr);
    // void freeAllTemporary();

    // code generation
    // void emit();
    // void pushLabel(StringRef start, StringRef end);
    // void popLabel();
    // StringRef startLabel() const;
    // StringRef endLabel() const;
};
