#define HAS_LEXER

#include "env.h"

#include "parser.h"

#include <cctype>
#include <iostream>
#include <string>
#include <vector>
using namespace std;

// -------- type management -------

// XXX: maybe move this logic to AggregateType
Type *TypeUtil::Concatenate(Type *front, Type *back)
{
    if (front == nullptr)
        return back;

    Type *p = front;
    Type **tref = &p;
    while (true)
    {
        p = *tref;
        switch (p->getClass())
        {
            case T_POINTER:
                tref = &dynamic_cast<PointerType *>(p)->_t;
                break;
            case T_ARRAY:
                tref = &dynamic_cast<ArrayType *>(p)->_t;
                break;
            case T_FUNCTION:
                tref = &dynamic_cast<FuncType *>(p)->_t;
                break;
            default:
                SyntaxError("TypeUtil::Concatenate: can't concatenate type.");
                break;
        }
        if (*tref == nullptr)
        {
            *tref = back;
            return front;
        }
    }
}
Type *TypeUtil::Merge(Type *t1, Type *t2)
{
    if (t1 == nullptr)
        return t2;
    else if (t2 == nullptr)
        return t1;

    SyntaxWarning("TypeUtil::Merge not fully implemented.");
    return nullptr;
}
bool TypeUtil::Equal(const Type *t1, const Type *t2)
{
    SyntaxError("TypeUtil::Equal: not implemented.");
    return false;
}
StringRef TypeUtil::GenerateTag()
{
    static int tag_id = 0;
    static vector<string> tags;
    tags.push_back(to_string(tag_id++));
    return StringRef(tags.back().data());
}
// -------- object management -------

// -------- symbol management -------

Symbol *Environment::findSymbol(ESymbolNamespace space, StringRef name) const
{
    for (Symbol *s : symbols)
    {
        if (s->space == space && s->name == name)
            return s;
    }
    return nullptr;
}
/*
Symbol *Environment::recursiveFind(ESymbolNamespace space, StringRef name) const
{
    Symbol *s = find(space, name);
    if (s != nullptr)
        return s;
    else
        return parent() ? parent()->recursiveFind(space, name) : nullptr;
}
Symbol *Environment::findDefinition(StringRef name) const
{
    return find(SYMBOL_NAMESPACE_tag, name);
}
Symbol *Environment::recursiveFindDefinition(StringRef name) const
{
    Symbol *s = find(SYMBOL_NAMESPACE_tag, name);

    if (s == nullptr)
        return parent() ? parent()->recursiveFindDefinition(name) : nullptr;

    assert(s->type != nullptr);
    if (!s->type->isIncompleteSimple())
        return s;

    Symbol *ps = parent() ? parent()->recursiveFindDefinition(name) : nullptr;
    return (ps && !ps->type->isIncompleteSimple()) ? ps : s;
}
Symbol *Environment::recursiveFindTypename(StringRef name) const
{
    for (Symbol *s : symbols)
    {
        if (s->space == SYMBOL_NAMESPACE_id && s->name == name && s->type &&
            s->type->type() == T_TYPEDEF)
            return s;
    }
    return parent() ? parent()->recursiveFindTypename(name) : nullptr;
}
*/
void Environment::addSymbol(Symbol *s)
{
    assert(s != nullptr);
    Symbol *origin = findSymbol(s->space, s->name);
    if (origin != nullptr)
    {
        // merge type
        Type *t1 = origin->type;
        Type *t2 = s->type;
        assert(t1 && t2);

        Type *t = TypeUtil::Merge(t1, t2);
        if (t == nullptr)
            SyntaxError("Symbol '" + s->name.toString() + "' already defined.");
        origin->type = t;

        // merge object
        if (origin->obj == nullptr)
            origin->obj = s->obj;
        else
        {
            assert(s->obj == nullptr);
        }
    }
    else
    {
        symbols.push_back(s);
    }
}
const Symbol *Environment::SameNameSymbolInFileScope(const Environment *env,
                                                     const Type *type,
                                                     const StringRef name)
{
    assert(env != nullptr);
    while (!env->isRoot())
    {
        env = env->parent();
    }

    // must be an object
    Symbol *s = env->findSymbol(SYMBOL_NAMESPACE_id, name);
    if (s && TypeUtil::Equal(s->type, type))
        return s;
    else
        return nullptr;
}

// -------- code generation --------
int Environment::idgen = 0;

// -------- debug  --------
void __debugPrint(string &&s)
{
    std::string tabs = "  ";
    std::string line = "  ";
    bool escape = false, empty = true;
    bool printenv = false;
    // uintptr_t env = 0;
    for (char c : s)
    {
        // if (printenv)
        // {
        //     if (c != '\n')
        //     {
        //         env *= 10;
        //         env += c - '0';
        //     }
        //     else
        //     {
        //         ((const Environment *)env)->debugPrint(tabs.size());
        //         printenv = false;
        //         env = 0;
        //     }
        //     continue;
        // }

        if (c == '~')
        {
            escape = true;
            continue;
        }

        if (!escape)
        {
            switch (c)
            {
                case '>': tabs += "  "; break;
                case '<':
                    tabs.pop_back();
                    tabs.pop_back();
                    break;
                case '@': printenv = true; break;
                default: line.push_back(c); break;
            }
        }
        else
        {
            line.push_back(c);
            escape = false;
        }
        if (!line.empty())
            empty = (empty && isspace(line.back()));

        if (c == '\n')
        {
            if (!empty)
                std::cout << line;
            line = tabs;
            empty = true;
        }
        else
        {
            if (empty && line.size() != tabs.size())
                line = tabs;
        }
    }
}
void Environment::debugPrint(int indent) const
{
    if (indent == 0)
    {
        // printf("%lu types. %lu symbols\n", TypeFactory::size(),
        //        SymbolFactory::size());
    }
    for (Symbol *s : symbols)
    {
        __debugPrint(s->toString());
        // printf("%*s %-12sType: %-*sObject: %s\n", indent + 5,
        //        s->space == SYMBOL_NAMESPACE_id
        //            ? "Name:"
        //            : (s->space == SYMBOL_NAMESPACE_tag ? "Tag: " : "Labl:"),
        //        s->name.toString().data(), 33 - indent,
        //        s->type ? s->type->toString().data() : "<null>",
        //        s->obj
        //            ? (s->obj->toString() + ", " + to_string(s->obj->size()) +
        //               " bytes.")
        //                  .data()
        //            : "<null>");
        // if (s->obj)
        // {
        //     FuncObject *fo = dynamic_cast<FuncObject *>(s->obj);
        //     if (fo)
        //     {
        //         // if (fo->getFuncEnv())
        //         //     fo->getFuncEnv()->debugPrint(indent + 2);
        //         if (fo->getFuncBody())
        //             __debugPrint(fo->getFuncBody()->debugString());
        //     }
        // }
    }
}

// -------- declaration parsing --------

