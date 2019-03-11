#pragma once

#include <vector>

#include "Symbol.h"

// Gather type system, symbol system, code system.

class Environment
{
public:
    explicit Environment() : m_Parent(nullptr) {}
    
    void SetParent(Environment * parent)
    {
        ASSERT(parent && !m_Parent);
        m_Parent = parent;
        parent->m_Children.push_back(this);
    }
    const Environment * GetParent() const
    {
        return m_Parent;
    }

    void AddSymbol(Symbol * symbol)
    {
        m_SymbolTable.AddSymbol(symbol);
    }
    Symbol * LookupSymbol(StringRef id, Symbol::SymbolType type) const
    {
        Symbol * s = m_SymbolTable.FindSymbol(id, type);
        return (s == nullptr && m_Parent != nullptr)
            ? m_Parent->LookupSymbol(id, type)
            : s;
    }
    const std::vector<Symbol *> & AllSymbols() const
    {
        return m_SymbolTable.AllSymbols();
    }


private:
    friend void PrintEnvironment(Environment * env);
    friend const std::vector<Symbol *> FindAllFunctionSymbols(Environment * env);

    Environment * m_Parent;
    std::vector<Environment *> m_Children;

    SymbolTable m_SymbolTable;
};

const std::vector<Symbol *> FindAllFunctionDefinitions(Environment * env)
{
    CHECK(env && !env->GetParent());

    std::vector<Symbol *> funcDefs;
    for (Symbol * sym : env->AllSymbols())
    {
        CHECK(sym);
        if (IsFunction(sym->objectType) && Function_GetBody(sym->objectType))
        {
            funcDefs.push_back(sym);
        }
    }
    return funcDefs;
}

void PrintEnvironment(Environment * env)
{
    CHECK(env);

    int indent = 0;
    for (const Environment * e = env; e->GetParent(); e = e->GetParent())
        indent += 2;

    std::string indentStr(indent, ' ');

    std::cout << indentStr << "{" << std::endl;
    for (Symbol * sym : env->AllSymbols())
    {
        CHECK(sym);
        std::cout << indentStr << " " << sym->toString() << std::endl;
    }
    for (Environment * child : env->m_Children)
    {
        CHECK(child);
        PrintEnvironment(child);
    }
    std::cout << indentStr << "}" << std::endl;
}
