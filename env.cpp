#include "env.h"
#include "parser.h"
#include "type.h"

// static struct
// {
//     IntType i8, ui8, i16, ui16, i32, ui32, i64, ui64;
//     FloatType f32, f64;
// } primitives;

#include <iostream>
using namespace std;
string DebugTypeClass(ETypeClass tc)
{
    switch (tc)
    {
        case TC_INT: return "int";
        case TC_FLOAT: return "float";
        case TC_POINTER: return "pointer to";
        case TC_ARRAY: return "array of";
        case TC_FUNC: return "func return";
        case TC_STRUCT: return "struct";
        case TC_UNION: return "union";
        case TC_ENUM: return "enum";
        default: break;
    }
    return "<null>";
}
ostream &operator<<(ostream &o, const Symbol &s)
{
    switch (s.category)
    {
        case SC_ID: o << "<id>"; break;
        case SC_ENUM_CONST: o << "<enum-const>"; break;
        case SC_LABEL: o << "<label>"; break;
        case SC_TAG: o << "<tag>"; break;
    }
    o << s.name << " is";
    TypeBase *t = s.type;
    while (t)
    {
        o << ' ' << DebugTypeClass(t->type());
        t = t->next();
    }
    return o;
}
void __debugPrint(string &&s)
{
    string tabs = "\t";
    string line = "\t";
    bool escape = false, empty = true;
    for(char c : s)
    {
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
                case '<': tabs.pop_back(); tabs.pop_back(); break;
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
                cout << line;
            line = tabs;
            empty = true;
        }
    }
}
void SymbolTable::debugPrint(Lexer &lex)
{
    cout << "[Symbol Table] " << symbols.size() << " symbols" << endl;
    for (Symbol *s : symbols)
    {
        cout << "  " << (*s) << endl;
        if (s->type)
        {
            switch (s->type->type())
            {
                case TC_STRUCT:
                    dynamic_cast<StructType *>(s->type)->debugPrint();
                    break;
                case TC_FUNC:
                    if (dynamic_cast<FuncType *>(s->type)->body)
                        __debugPrint(dynamic_cast<FuncType *>(s->type)->body->debugString());
                    break;
                default:
                    break;
            }
        }
    }
}

int Environment::idgen = 0;
void Environment::debugPrint(Lexer &lex)
{
    cout << "Environment [" << id << "] with " << getChildren().size()
         << " children:";
    for (Environment *c : getChildren())
        cout << ' ' << c->id;
    cout << endl;
    symbols.debugPrint(lex);
    cout << endl;
    for (Environment *c : getChildren())
        c->debugPrint(lex);
}
Symbol *Environment::find(ESymbolCategory category, StringRef name) const
{
    Symbol *s = nullptr;
    const Environment *env = this;
    while (s == nullptr && env != nullptr)
    {
        s = env->symbols.find(category, name);
        env = env->parent();
    }
    return s;
}

TypeBase *__parseVoidType(Lexer &lex)
{
    // cout << "__parseVoidType()" << endl;
    if (lex.peakNext().type != TYPE_VOID)
    {
        SyntaxError("Expect 'void'");
    }
    lex.getNext();
    return new VoidType();
}
TypeBase *__parseIntegralType(Lexer &lex)
{
    // cout << "__parseIntegralType()" << endl;
    TypeBase *i = nullptr;
    switch (lex.getNext().type)
    {
        case TYPE_INT:    // i = &primitives.i32; break;
        case TYPE_CHAR:   // i = &primitives.i8; break;
        case TYPE_SHORT:  // i = &primitives.i16; break;
        case TYPE_LONG:   // i = &primitives.i64; break;
            i = new IntType();
            break;
        default: SyntaxError("Unexpected token");
    }
    return i;
}
TypeBase *__parseFloatingType(Lexer &lex)
{
    // cout << "__parseFloatingType()" << endl;
    TypeBase *f = nullptr;
    switch (lex.getNext().type)
    {
        case TYPE_FLOAT:   // f = &primitives.f32; break;
        case TYPE_DOUBLE:  // f = &primitives.f64; break;
            f = new FloatType();
            break;
        default: SyntaxError("Unexpected token");
    }
    return f;
}

TypeBase *__parseSpecifier(Lexer &lex, Environment *env);
TypeBase *__parseDeclarator(Lexer &lex, Environment *env, TypeBase *spec,
                            StringRef &symbol);

TypeBase *__parseStructType(Lexer &lex, Environment *env)
{
    EXPECT(TYPE_STRUCT);
    if (lex.peakNext().type == SYMBOL && lex.peakNext(1).type != BLK_BEGIN)
    {
        // use defined struct
        StringRef tag = lex.symbolName(lex.getNext().symid);
        // TODO: not defined yet, incomplete type?
        return env->symbols.find(SC_TAG, tag)->type;
    }
    else
    {
        // tag
        StringRef tag("<anonymous-struct>");
        if (lex.peakNext().type == SYMBOL)
            tag = lex.symbolName(lex.getNext().symid);

        // type definition
        StructType *st = new StructType();
        EXPECT(BLK_BEGIN);
        while (true)
        {
            TypeBase *spec = __parseSpecifier(lex, env);
            while (true)
            {
                StringRef id("<anonymous-struct-member>");
                TypeBase *decl = __parseDeclarator(lex, env, spec, id);
                Symbol *member = new Symbol();
                member->category = SC_ID;
                member->name = id;
                member->type = decl;
                member->addr = 0;
                st->addMember(member);

                if (lex.peakNext().type == OP_COMMA)
                    lex.getNext();
                else if (lex.peakNext().type == STMT_END)
                {
                    lex.getNext();
                    break;
                }
                else
                    SyntaxError("Unexpected token");
            }
            if (lex.peakNext().type == BLK_END)
                break;
        }
        EXPECT(BLK_END);

        Symbol *struct_tag = new Symbol();
        struct_tag->category = SC_TAG;
        struct_tag->name = tag;
        struct_tag->type = st;
        struct_tag->addr = 0;

        env->symbols.add(struct_tag);

        return st;
    }
}
TypeBase *__parseEnumType(Lexer &lex, Environment *env)
{
    EXPECT(TYPE_ENUM);
    if (lex.peakNext().type == SYMBOL && lex.peakNext(1).type != BLK_BEGIN)
    {
        // use defined enum
        StringRef tag = lex.symbolName(lex.getNext().symid);
        return env->symbols.find(SC_TAG, tag)->type;
    }
    else
    {
        // tag
        StringRef tag("<anonymous-enum>");
        if (lex.peakNext().type == SYMBOL)
            tag = lex.symbolName(lex.getNext().symid);

        // type definition
        EnumType *et = new EnumType();
        int value = 0;
        EXPECT(BLK_BEGIN);
        while (true)
        {
            if (lex.peakNext().type != SYMBOL)
            {
                SyntaxErrorEx("diagnosis");
            }
            StringRef id = lex.symbolName(EXPECT_GET(SYMBOL).symid);
            // new enum constant
            Symbol *e = new Symbol();
            e->category = SC_ENUM_CONST;
            e->name = id;
            e->type = et;
            e->value = value++;
            env->symbols.add(e);
            if (lex.peakNext().type == OP_COMMA)
                lex.getNext();
            else if (lex.peakNext().type == BLK_END)
                break;
        }
        EXPECT(BLK_END);

        Symbol *enum_tag = new Symbol();
        enum_tag->category = SC_TAG;
        enum_tag->name = tag;
        enum_tag->type = et;
        enum_tag->addr = 0;

        env->symbols.add(enum_tag);

        return et;
    }
}
TypeBase *__parseUnionType(Lexer &lex, Environment *env)
{
    SyntaxError("Not supported syntax");
    return nullptr;
}

// only handle parameters
TypeBase *__parseFuncType(Lexer &lex, Environment *env)
{
    // cout << "__parseFuncType()" << endl;
    if (lex.peakNext().type != LP)
        return nullptr;

    FuncType *f = new FuncType();
    f->env = new Environment();
    // f->env->setParent(env);

    // parse parameters
    vector<Symbol *> params;
    EXPECT(LP);
    if (lex.peakNext().type != RP)
    {
        while (true)
        {
            StringRef symbol("<anonymous-param>");
            TypeBase *decl =
                __parseDeclarator(lex, env, __parseSpecifier(lex, env), symbol);
            Symbol *s = new Symbol();
            s->category = SC_ID;
            s->name = symbol;
            s->type = decl;
            s->addr = 0;
            params.push_back(s);
            if (lex.peakNext().type == RP)
                break;
            else if (lex.peakNext().type == OP_COMMA)
                lex.getNext();
            else
                SyntaxError("Unexpected token");
        }
    }
    EXPECT(RP);

    // calculate parameter location
    // long location = 0;
    for (auto p = params.rbegin(); p != params.rend(); ++p)
    {
        // p.location = location;
        // location += p.type->size();
        f->env->symbols.add(*p);
    }

    return f;
}
TypeBase *__parseArrayType(Lexer &lex)
{
    // cout << "__parseArrayType()" << endl;
    if (lex.peakNext().type != LSB)
        return nullptr;

    ArrayType *a = new ArrayType();
    a->length = 1;
    while (lex.peakNext().type == LSB)
    {
        lex.getNext();
        long l =
            lex.getNext()
                .ival;  // ConstExpression::eval(CondExpression::parse(lex));
        // assert( l > 0 );
        a->axis.push_back(l);
        a->length *= l;
        if (lex.getNext().type != RSB)
        {
            SyntaxError("Expect ']'");
        }
    }
    a->dimen = a->axis.size();
    return a;
}
// return tail of chain
TypeBase *__parsePointerType(Lexer &lex)
{
    // cout << "__parsePointerType()" << endl;
    TypeBase *tail = nullptr;
    TypeBase *p, *q;
    while (lex.peakNext().type == OP_MUL)
    {
        lex.getNext();
        p = new PointerType();
        if (tail)
            q = q->mergeAtHead(p);
        else
            tail = q = p;
    }
    return tail;
}
// extract type-list from specifier
TypeBase *__parseSpecifier(Lexer &lex, Environment *env)
{
    // cout << "__parseSpecifier()" << endl;
    TypeBase *t = nullptr;
    switch (lex.peakNext().type)
    {
        case TYPE_VOID: t = __parseVoidType(lex); break;
        case SIGNED:
        case UNSIGNED:
        case TYPE_INT:
        case TYPE_SHORT:
        case TYPE_CHAR: t = __parseIntegralType(lex); break;
        case TYPE_LONG:
            if (lex.peakNext(1).type == TYPE_DOUBLE)
                t = __parseFloatingType(lex);
            else
                t = __parseIntegralType(lex);
            break;
        case TYPE_DOUBLE:
        case TYPE_FLOAT: t = __parseFloatingType(lex); break;
        case TYPE_ENUM: t = __parseEnumType(lex, env); break;
        case TYPE_STRUCT: t = __parseStructType(lex, env); break;
        case TYPE_UNION: t = __parseUnionType(lex, env); break;
        default: break;
    }
    return t;
}
bool __isDeclarator(Lexer &lex)
{
    return lex.peakNext().type == OP_MUL || lex.peakNext().type == LP ||
           lex.peakNext().type == SYMBOL;
}
// extract type-list from declarator
TypeBase *__parseDeclarator(Lexer &lex, Environment *env, TypeBase *spec,
                            StringRef &symbol)
{
    // cout << "__parseDeclarator()" << endl;
    if (!__isDeclarator(lex))
        return nullptr;

    TypeBase *pointer = __parsePointerType(lex);
    TypeBase *inner = nullptr;
    TypeBase *array_or_func = nullptr;

    if (lex.peakNext().type == LP)
    {
        lex.getNext();
        inner = __parseDeclarator(lex, env, nullptr, symbol);
        EXPECT(RP);
    }
    else if (lex.peakNext().type == SYMBOL)
    {
        symbol = lex.symbolName(lex.getNext().symid);
    }
    else
    {
        SyntaxError("Expect symbol or '('");
    }

    if (lex.peakNext().type == LSB)
    {
        array_or_func = __parseArrayType(lex);
    }
    else if (lex.peakNext().type == LP)
    {
        array_or_func = __parseFuncType(lex, env);
    }

    if (spec == nullptr)
    {
        if (pointer == nullptr)
            return array_or_func->mergeAtHead(inner);
        else
            return pointer->mergeAtHead(array_or_func)->mergeAtHead(inner);
    }
    else
        return spec->mergeAtHead(pointer)
            ->mergeAtHead(array_or_func)
            ->mergeAtHead(inner);
}

// void Environment::ParseGlobalDeclaration(Lexer &lex, Environment *env)
void __parseDeclaration(Lexer &lex, Environment *env, bool allow_func_def)
{
    // cout << "__parseDeclaration()" << endl;
    StringRef symbol;
    TypeBase *spec = __parseSpecifier(lex, env);
    TypeBase *decl = __parseDeclarator(lex, env, spec, symbol);
    if (lex.peakNext().type == BLK_BEGIN)
    {
        if (!allow_func_def)
            SyntaxError("Function definition not allowed here.");

        // Function definition
        // assert( decl->type() == TC_FUNC );
        FuncType *func = dynamic_cast<FuncType *>(decl);
        func->env->setParent(env);
        func->body = CompoundStatement::parse(lex, func->env, true);
        Symbol *f = new Symbol();
        f->category = SC_ID;
        f->name = symbol;
        f->type = func;
        f->addr = 0;
        env->symbols.add(f);
    }
    else
    {
        // Declaration
        while (true)
        {
            if (symbol.size() == 0)
            {
                if (spec->type() == TC_STRUCT || spec->type() == TC_ENUM ||
                    spec->type() == TC_UNION)
                    break;
                else
                    SyntaxError("Missing identifier");
            }
            Symbol *v = new Symbol();
            v->category = SC_ID;
            v->name = symbol;
            v->type = decl;
            v->addr = 0;
            env->symbols.add(v);
            if (lex.peakNext().type == OP_COMMA)
            {
                lex.getNext();
                symbol.clear();
                decl = __parseDeclarator(lex, env, spec, symbol);
            }
            else
                break;
        }
        EXPECT(STMT_END);
    }

    // TODO: export assignment statements
}

// parse all declarations
void Environment::ParseLocalDeclaration(Lexer &lex, Environment *env)
{
    __parseDeclaration(lex, env, false);
}
// parse function definition, all declarations
void Environment::ParseGlobalDeclaration(Lexer &lex, Environment *env)
{
    while (lex.hasNext())
    {
        __parseDeclaration(lex, env, true);
    }
}

TypeBase *Environment::ParseTypename(Lexer &lex, Environment *env)
{
    return nullptr;
}
