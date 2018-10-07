#pragma once

#include "../lex/tokenizer.h"
#include "../type/type.h"

// Symbol of SymbolTable

enum ESymbolNamespace {
    SYMBOL_NAMESPACE_id, // object, enum-constant, typedef-name
    SYMBOL_NAMESPACE_label,
    SYMBOL_NAMESPACE_tag,
    SYMBOL_NAMESPACE_member,
};

// linkage means "do these declarations refer to the same object?"
enum ESymbolLinkage {
    SYMBOL_LINKAGE_invalid, // TODO: remove this
    SYMBOL_LINKAGE_unique, // none linkage: auto/register/typedef
    SYMBOL_LINKAGE_external, // the same in whole program
    SYMBOL_LINKAGE_internal, // the same in translation unit
};

// only used to compute ESymbolLinkage
enum ESymbolScope {
    SYMBOL_SCOPE_none, // TODO: remove this
    SYMBOL_SCOPE_file,
    SYMBOL_SCOPE_func,
    SYMBOL_SCOPE_block,
    SYMBOL_SCOPE_func_proto,
};

class SymbolBuilder;

struct Symbol {
    enum Namespace { ID, LABEL, TAG, MEMBER };

    Namespace space; // from syntax-tree context
    ESymbolLinkage linkage; // from SymbolBuilder
    ESymbolScope scope; // from syntax-tree context
    StringRef name;
    Type * type;

    // private:
    //    Symbol() {
    //    }

public:
    std::string toString() const {
        std::string s;
        if (space == Namespace::LABEL)
        {
            s += name.toString();
        }
        else
        {
            s += (space == Namespace::ID) ? "ID " : "TAG ";
            s += "\"" + name.toString() + "\":(" +
                (type ? type->toString() : "") + ")";
        }
        return s;
    }
};

 class SymbolBuilder {
    Token::Type _storage_token; // none/typedef/extern/static/auto/register
    ESymbolScope _scope; // func/file/block/func_proto
    Symbol::Namespace _namespace;
    ESymbolLinkage _linkage_of_same_name_object_in_file_scope;

    Type * _type;
    StringRef _name;

    ESymbolLinkage __compute_linkage() const {
        assert(_type != nullptr);

        bool is_an_object = (_type->isObject() || _type->isFunction());

        if (is_an_object == false || _scope == SYMBOL_SCOPE_func_proto ||
            (_type->isObject() && _storage_token != Token::KW_EXTERN &&
             _scope == SYMBOL_SCOPE_block))
        {
            return SYMBOL_LINKAGE_unique;
        }

        if (is_an_object)
        {
            if (_storage_token == Token::KW_STATIC &&
                _scope == SYMBOL_SCOPE_file)
            {
                return SYMBOL_LINKAGE_internal;
            }
            if (_storage_token == Token::KW_EXTERN ||
                (_type->isFunction() && _storage_token == Token::UNKNOWN &&
                 _scope == SYMBOL_SCOPE_file))
            {
                return _linkage_of_same_name_object_in_file_scope;
            }
            if (_type->isObject() && _storage_token == Token::UNKNOWN &&
                _scope == SYMBOL_SCOPE_file)
            {
                return SYMBOL_LINKAGE_external;
            }
        }

        SyntaxError("SymbolBuilder: can't determine linkage");
        return SYMBOL_LINKAGE_invalid;
    }

 public:
    SymbolBuilder()
        : _storage_token(Token::UNKNOWN)
        , _scope(SYMBOL_SCOPE_none)
        , _namespace(Symbol::Namespace::ID)
        , _linkage_of_same_name_object_in_file_scope(SYMBOL_LINKAGE_invalid)
        , _type(nullptr) {
    }
    void setStorageSpecifier(Token::Type s) {
        // TODO: verify input
        _storage_token = s;
    }
    void setNamespace(Symbol::Namespace space) {
        _namespace = space;
    }
    void setScope(ESymbolScope s) {
        _scope = s;
    }
    void setType(Type * t) {
        _type = t;
    }
    void setName(StringRef name,
                 ESymbolLinkage losnoifs = SYMBOL_LINKAGE_external) {
        _name = name;
        _linkage_of_same_name_object_in_file_scope = losnoifs;
    }

    Symbol * build() const {
        assert(_type != nullptr);

        Symbol * s = new Symbol();
        s->name = _name;
        s->linkage = __compute_linkage();
        s->type = _type;
        s->space = _namespace;
        return s;
    }
};
