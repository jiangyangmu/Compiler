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
    void addSymbol(Symbol * sym) {
        symbols_.push_back(sym);
    }
    Symbol * findSymbol(StringRef id) const {
        Symbol * s = nullptr;
        for (Symbol * symbol : symbols_)
        {
            if (id == symbol->name)
            {
                s = symbol;
                break;
            }
        }
        if (s == nullptr && parent_ != nullptr)
            s = parent_->findSymbol(id);
        return s;
    }
    std::vector<Symbol *> allSymbols() const {
        return symbols_;
    }

    // Symbol * findSymbolByName(StringRef name);
    void setParent(Environment * parent) {
        CHECK(parent != nullptr);
        parent->children_.push_back(this);
        parent_ = parent;
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
