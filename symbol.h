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
enum ESymbolLinkage
{
    SYMBOL_LINKAGE_unique,  // none linkage: auto/register/typedef
    SYMBOL_LINKAGE_external,
    SYMBOL_LINKAGE_internal,
};

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
    Symbol()
    {
    }
    // friend std::ostream &operator<<(std::ostream &o, const Symbol &s);

   public:
    virtual std::string toString() const
    {
        std::string s = "symbol [>\n";
        s += "name: ";
        s.append(name.data(), name.size());
        s += "\nspace: ";
        s += (space == SYMBOL_NAMESPACE_id) ? "id" : (space == SYMBOL_NAMESPACE_label ? "label" : "tag");
        s += "\nlinkage: ";
        s += (linkage == SYMBOL_LINKAGE_unique) ? "unique" : (linkage == SYMBOL_LINKAGE_external ? "external" : "internal");
        s += "\ntype: ";
        s += type ? type->toString() : std::string("<null>");
        s += "\nobj: ...";
        s += "\n<]\n";
        return s;
    }
};

class SymbolBuilder
{
    TokenType _storage_token;  // none/typedef/extern/static/auto/register
    ESymbolScope _scope;       // func/file/block/func_proto
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
          _linkage_of_same_name_object_in_file_scope(SYMBOL_LINKAGE_unique),
          _type(nullptr)
    {
    }
    void setStorageSpecifier(TokenType s)
    {
        // TODO: verify input
        _storage_token = s;
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
        s->space = NamespaceFromTypeClass(_type->getClass());
        return s;
    }

    static ESymbolNamespace NamespaceFromTypeClass(ETypeClass t)
    {
        ESymbolNamespace ns;
        switch (t)
        {
            case T_LABEL: ns = SYMBOL_NAMESPACE_label; break;
            case T_TAG: ns = SYMBOL_NAMESPACE_tag; break;
            case T_VOID:
            case T_CHAR:
            case T_INT:
            case T_FLOAT:
            case T_POINTER:
            case T_ARRAY:
            case T_FUNCTION:
            case T_ENUM_CONST: ns = SYMBOL_NAMESPACE_id; break;
            default: SyntaxError("SymbolBuilder: unexpect symbol type"); break;
        }
        return ns;
    }
};

/*
class SymbolFactory
{
    static vector<Symbol *> references;
    static Symbol * get()
    {
        references.push_back(new Symbol());
        return references.back();
    }
   public:
    static size_t size()
    {
        return references.size();
    }
    static Symbol *newInstance()
    {
        Symbol *s = get();
        s->type = TypeFactory::newInstance();
        return s;
    }
    static Symbol *newInstance(const Symbol &t)
    {
        Symbol *s = get();
        (*s) = t;
        return s;
    }
    static Symbol *newTag(StringRef name, Type *type)
    {
        Symbol *s = get();
        s->space = SYMBOL_NAMESPACE_tag;
        s->name = name;
        s->type = type;
        return s;
    }
    static void check()
    {
        // for (Symbol *s : references)
        // {
        //     if (s->obj && (s->type == nullptr || s->type->isIncomplete()))
        //     {
        //         SyntaxWarning("Object '" + s->name.toString() + "' is never
completed.");
        //     }
        // }
    }
};
*/
