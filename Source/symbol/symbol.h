#pragma once

#include "../lex/tokenizer.h"
#include "../type/CType.h"

struct Symbol
{
    enum SymbolType
    {
        ID,
        ALIAS,
        TAG,
        LABEL
    };
    enum StorageType
    {
        NONE,
        THIS_TRANSLATION_UNIT,
        OTHER_TRANSLATION_UNIT,
        CHECK_FILE_SCOPE_SYMBOL,
        LOCAL
    };
    
    StringRef   name;
    bool        needExport;
    SymbolType  symbolType;
    TYPE_HANDLE objectType;
    StorageType storageType;

public:
    Symbol()
        : needExport(false)
        , objectType(0) {
    }
    std::string toString() const {
        std::string s;

        s += "\"" + name.toString() + "\"";

        while (s.size() < 10)
            s.append(10 - s.size(), ' ');

        if (symbolType == SymbolType::ID)
            s += "ID    ";
        else if (symbolType == SymbolType::ALIAS)
            s += "ALIAS ";
        else
            s += "TAG   ";

        if (storageType == THIS_TRANSLATION_UNIT)
            s += "global ";
        else if (storageType == OTHER_TRANSLATION_UNIT)
            s += "extern ";
        else if (storageType == CHECK_FILE_SCOPE_SYMBOL)
            s += "file   ";
        else if (storageType == LOCAL)
            s += "local  ";
        else
            s += "none   ";

        if (needExport)
            s += "export ";
        else
            s += "       ";

        s += "(" + TypeToString(objectType) + ")";
        return s;
    }
};

// Build

class SymbolBuilder
{
public:
    explicit SymbolBuilder() : symbol(nullptr) {}
    SymbolBuilder & Create()
    {
        CHECK(!symbol);
        symbol = new Symbol();
        return *this;
    }
    SymbolBuilder & WithId(StringRef name)
    {
        symbol->name = name;
        return *this;
    }
    SymbolBuilder & WithSymbolType(Symbol::SymbolType symbolType)
    {
        symbol->symbolType = symbolType;
        return *this;
    }
    SymbolBuilder & WithObjectType(TYPE_HANDLE objectType)
    {
        symbol->objectType = objectType;
        return *this;
    }
    SymbolBuilder & WithStorageType(Symbol::StorageType storageType)
    {
        symbol->storageType = storageType;
        return *this;
    }
    SymbolBuilder & WithExport(bool needExport)
    {
        symbol->needExport = needExport;
        return *this;
    }
    Symbol * Finish()
    {
        Symbol * s = symbol;
        symbol = nullptr;
        return s;
    }

private:
    Symbol * symbol;
};

// Manage

// Manage a set of symbols, alert duplicate definition, merge compatible definitions.
class SymbolTable
{
public:
    void AddSymbol(Symbol * symbol)
    {
        // TODO: Alert duplicate definition, merge compatible definitions.
        m_Symbols.push_back(symbol);
    }
    Symbol * FindSymbol(StringRef id, Symbol::SymbolType type) const
    {
        Symbol * s = nullptr;
        for (Symbol * symbol : m_Symbols)
        {
            if (id == symbol->name && type == symbol->symbolType)
            {
                s = symbol;
                break;
            }
        }
        return s;
    }

    const std::vector<Symbol *> & AllSymbols() const
    {
        return m_Symbols;
    }
private:
    std::vector<Symbol *> m_Symbols;
};

// Query

