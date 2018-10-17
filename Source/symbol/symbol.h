#pragma once

#include "../lex/tokenizer.h"
#include "../type/type.h"

struct Symbol
{
    enum SymbolType
    {
        ID,
        ALIAS,
        TAG
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
    Type *      objectType;
    StorageType storageType;

public:
    Symbol()
        : needExport(false)
        , objectType(nullptr) {
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

        s += "(" + (objectType ? objectType->toString() : "") + ")";
        return s;
    }
};
