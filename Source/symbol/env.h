#pragma once

#include "symbol.h"

#include <vector>

// Manage a tree of symboltables
// Provide fast search of symbol

class Environment {
public:
    Environment()
        : parent_(nullptr) {
    }
    void addSymbol(Symbol * symbol) {
        //Symbol * same_symbol = nullptr;
        //for (Symbol * sym : symbols_)
        //{
        //    if (sym->name == symbol->name &&
        //        sym->symbolType == symbol->symbolType)
        //    {
        //        same_symbol = sym;
        //        break;
        //    }
        //}
        //CHECK(same_symbol == nullptr);
        symbols_.push_back(symbol);
    }
    Symbol * findSymbol(StringRef id, Symbol::SymbolType type) const {
        Symbol * s = nullptr;
        for (Symbol * symbol : symbols_)
        {
            if (id == symbol->name && type == symbol->symbolType)
            {
                s = symbol;
                break;
            }
        }
        if (s == nullptr && parent_ != nullptr)
            s = parent_->findSymbol(id, type);
        return s;
    }
    std::vector<Symbol *> allSymbols() const {
        return symbols_;
    }

    void setParent(Environment * parent) {
        CHECK(parent != nullptr);
        parent->children_.push_back(this);
        parent_ = parent;
    }
    Environment * getParent() {
        return parent_;
    }
    std::string toString() const {
        return toString(this, "");
    }

private:
    static std::string toString(const Environment * env, std::string indent) {
        std::string s;
        CHECK(env != nullptr);
        s += indent + "{\n";
        for (Symbol * sym : env->symbols_)
        {
            CHECK(sym != nullptr);
            s += indent + "  " + sym->toString() + "\n";
        }
        for (Environment * child : env->children_)
        {
            CHECK(child != nullptr);
            s += toString(child, indent + "  ");
        }
        s += indent + "}\n";
        return s;
    }
    Environment * parent_;
    std::vector<Environment *> children_;

    std::vector<Symbol *> symbols_;
};
