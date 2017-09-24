#pragma once

#include "lexer.h"
#include "type.h"
// #include "object.h"

class Object
{
};

enum ESymbolNamespace
{
    SYMBOL_NAMESPACE_id,  // object, enum-constant, typedef-name
    SYMBOL_NAMESPACE_label,
    SYMBOL_NAMESPACE_tag,
};

// linkage means "do these declarations refer to the same object?"
enum ESymbolLinkage
{
    SYMBOL_LINKAGE_unique,  // none linkage: auto/register/typedef
    SYMBOL_LINKAGE_external, // the same in whole program
    SYMBOL_LINKAGE_internal, // the same in translation unit
};

// only used to compute ESymbolLinkage
enum ESymbolScope
{
    SYMBOL_SCOPE_none,
    SYMBOL_SCOPE_func,
    SYMBOL_SCOPE_file,
    SYMBOL_SCOPE_block,
    SYMBOL_SCOPE_func_proto
};

// enum ESymbolStorage
// {
//     SYMBOL_STORAGE_none,
//     SYMBOL_STORAGE_auto,
//     SYMBOL_STORAGE_static
// };

class SymbolBuilder;

// Symbol: Everything about name, namespace
struct Symbol : public Stringable
{
    friend SymbolBuilder;

    ESymbolNamespace space;
    ESymbolLinkage linkage;
    // ESymbolScope scope;
    StringRef name;
    Type *type;
    Object *obj;  // if type is incomplete, obj == nullptr

   private:
    Symbol() {}
    // friend std::ostream &operator<<(std::ostream &o, const Symbol &s);

   public:
    virtual std::string toString() const
    {
        std::string s;
        s += (space == SYMBOL_NAMESPACE_id)
                 ? " id  |"
                 : (space == SYMBOL_NAMESPACE_label ? " lab |" : " tag |");
        s += (linkage == SYMBOL_LINKAGE_unique)
                 ? " unique   |"
                 : (linkage == SYMBOL_LINKAGE_external ? " external |"
                                                       : " internal |");
        s += " \"";
        s.append(name.data(), name.size());
        s += "\" = ";
        s += type ? type->toString() : std::string("<null>");
        return s;
    }
};

class SymbolBuilder
{
    TokenType _storage_token;  // none/typedef/extern/static/auto/register
    ESymbolScope _scope;       // func/file/block/func_proto
    ESymbolNamespace _namespace;
    ESymbolLinkage _linkage_of_same_name_object_in_file_scope;

    Type *_type;
    StringRef _name;

    ESymbolLinkage __compute_linkage() const
    {
        assert(_type != nullptr);

        bool is_an_object = (_type->isObject() || _type->isFunction());

        if (is_an_object == false || _scope == SYMBOL_SCOPE_func_proto ||
            (_type->isObject() && _storage_token != EXTERN &&
             _scope == SYMBOL_SCOPE_block))
        {
            return SYMBOL_LINKAGE_unique;
        }

        if (is_an_object)
        {
            if (_storage_token == STATIC && _scope == SYMBOL_SCOPE_file)
            {
                return SYMBOL_LINKAGE_internal;
            }
            if (_storage_token == EXTERN ||
                (_type->isFunction() && _storage_token == NONE &&
                 _scope == SYMBOL_SCOPE_file))
            {
                return _linkage_of_same_name_object_in_file_scope;
            }
            if (_type->isObject() && _storage_token == NONE &&
                _scope == SYMBOL_SCOPE_file)
            {
                return SYMBOL_LINKAGE_external;
            }
        }

        SyntaxError("SymbolBuilder: can't determine linkage");
        return SYMBOL_LINKAGE_unique;
    }

   public:
    SymbolBuilder()
        : _storage_token(NONE),
          _scope(SYMBOL_SCOPE_none),
          _namespace(SYMBOL_NAMESPACE_id),
          _linkage_of_same_name_object_in_file_scope(SYMBOL_LINKAGE_unique),
          _type(nullptr)
    {
    }
    void setStorageSpecifier(TokenType s)
    {
        // TODO: verify input
        _storage_token = s;
    }
    void setNamespace(ESymbolNamespace space)
    {
        _namespace = space;
    }
    void setScope(ESymbolScope s)
    {
        _scope = s;
    }
    void setType(Type *t)
    {
        _type = t;
    }
    void setName(StringRef name,
                 ESymbolLinkage losnoifs = SYMBOL_LINKAGE_external)
    {
        _name = name;
        _linkage_of_same_name_object_in_file_scope = losnoifs;
    }

    Symbol *build() const
    {
        assert(_type != nullptr);

        Symbol *s = new Symbol();
        s->name = _name;
        s->linkage = __compute_linkage();
        s->type = _type;
        s->obj = nullptr;
        s->space = _namespace;
        return s;
    }
};
