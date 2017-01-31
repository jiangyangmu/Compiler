#define HAS_LEXER

#include "env.h"
#include "parser.h"
#include "type.h"

#include <iostream>
using namespace std;

// -------- for TypeTable -------

const IntType &TypeTable::Char()
{
    static IntType i8(1, true);
    return i8;
}
const IntType &TypeTable::UChar()
{
    static IntType u8(1, false);
    return u8;
}
const IntType &TypeTable::Short()
{
    static IntType i16(2, true);
    return i16;
}
const IntType &TypeTable::UShort()
{
    static IntType u16(2, false);
    return u16;
}
const IntType &TypeTable::Int()
{
    static IntType i32(4, true);
    return i32;
}
const IntType &TypeTable::UInt()
{
    static IntType u32(4, false);
    return u32;
}
const IntType &TypeTable::Long()
{
    static IntType i64(8, true);
    return i64;
}
const IntType &TypeTable::ULong()
{
    static IntType u64(8, false);
    return u64;
}
TypeBase *TypeTable::newIntegral(TokenType type, bool is_signed)
{
    const IntType *i = nullptr;
    switch (type)
    {
        case TYPE_CHAR: i = is_signed ? &Char() : &UChar(); break;
        case TYPE_SHORT: i = is_signed ? &Short() : &UShort(); break;
        case TYPE_INT: i = is_signed ? &Int() : &UInt(); break;
        case TYPE_LONG: i = is_signed ? &Long() : &ULong(); break;
        default: SyntaxError("Invalid Integral Type"); break;
    }
    return new IntType(*i);
}

// -------- for SymbolTable -------

static string DebugTypeClass(ETypeClass tc)
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
        if (t->type() == TC_INT)
            o << ' ' << dynamic_cast<IntType *>(t)->toString();
        else
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
        else
        {
            if (empty && line.size() != tabs.size())
                line = tabs;
        }
    }
}
void SymbolTable::add(Symbol *t)
{
    symbols.push_back(t);
    if (t->type)
        size_ += t->type->size();
}
Symbol *SymbolTable::find(ESymbolCategory category, StringRef name) const
{
    for (auto s = symbols.begin(); s != symbols.end(); ++s)
    {
        if ((*s)->category == category && (*s)->name == name)
            return *s;
    }
    return nullptr;
}
void SymbolTable::debugPrint(Lexer &lex) const
{
    cout << "[Symbol Table] " << symbols.size() << " symbols" << endl;
    for (const Symbol *s : symbols)
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
                        // cout << "\t{ ... }" << endl;
                        __debugPrint(dynamic_cast<FuncType *>(s->type)->body->debugString());
                    break;
                default:
                    break;
            }
        }
    }
}

// -------- for Environment --------

int Environment::idgen = 0;
void Environment::debugPrint(Lexer &lex) const
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
void Environment::add(Symbol *s)
{
    assert(s != nullptr);
    Symbol *dup = symbols.find(s->category, s->name);
    if (dup != nullptr)
    {
        assert(dup->type != nullptr);
        if (dup->type->type() == TC_FUNC)
        {
            FuncType *func = dynamic_cast<FuncType *>(dup->type);
            assert(func != nullptr);
            if (func->body != nullptr)
                SyntaxError("Function '" + s->name.toString() + "' redefined.");
        }
        else
            SyntaxError("Symbol '" + s->name.toString() + "' redefined.");
    }
    else if (find(s->category, s->name) != nullptr)
        SyntaxWarning("Symbol '" + s->name.toString() + "' override outer definition.");
    symbols.add(s);
}
size_t Environment::allSymbolSize() const
{
    return symbols.size();
}
void Environment::emit()
{
    if (isRoot()) // global scope
    {
        // generate code for function bodies
        for (const Symbol *s : symbols.symbols)
        {
            if (s->type->type() == TC_FUNC)
            {
                FuncType *func = dynamic_cast<FuncType *>(s->type);
                EmitDecl(".global _%s", s->name.toString().data());
                if (func->body)
                {
                    Emit("_%s:", s->name.toString().data());
                    Emit("pushq %%rbp");
                    Emit("movq %%rsp, %%rbp");
                    Emit("subq $64, %%rsp");
                    if (func->env->paramcnt >= 4)
                        Emit("movq %%rcx, -8(%%rbp)");
                    func->body->emit(this, FOR_NOTHING);
                }
                else
                    SyntaxWarning("function '" + s->name.toString() + "' has no body.");
            }
            else if (s->type->type() == TC_INT)
            {
                EmitData("_%s: .quad 0", s->name.toString().data());
            }
        }
    }
    else // block scope
    {
        if (paramcnt > 6)
            SyntaxError("Not implemented.");
    }
}
void Environment::pushLabel(StringRef start, StringRef end)
{
    slabels.push_back(start);
    elabels.push_back(end);
}
void Environment::popLabel()
{
    slabels.pop_back();
    elabels.pop_back();
}
StringRef Environment::startLabel() const
{
    StringRef l;
    if (!slabels.empty())
        l = slabels.back();
    else if (parent())
        l = parent()->startLabel();
    else
        SyntaxError("can't use 'continue' here");
    return l;
}
StringRef Environment::endLabel() const
{
    StringRef l;
    if (!elabels.empty())
        l = elabels.back();
    else if (parent())
        l = parent()->endLabel();
    else
        SyntaxError("can't use 'break' here");
    return l;
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
TypeBase *__parseIntegralType(Lexer &lex, Environment *env)
{
    // cout << "__parseIntegralType()" << endl;
    bool is_signed = true;
    switch (lex.peakNext().type)
    {
        case SIGNED: lex.getNext(); break;
        case UNSIGNED: is_signed = false; lex.getNext(); break;
        default: break;
    }

    TokenType type = TYPE_INT;
    switch (lex.peakNext().type)
    {
        case TYPE_CHAR:
            type = TYPE_CHAR;
            lex.getNext();
            break;
        case TYPE_SHORT:
            type = TYPE_SHORT;
            lex.getNext();
            if (lex.peakNext().type == TYPE_INT)
                lex.getNext();
            break;
        case TYPE_LONG:
            type = TYPE_LONG;
            lex.getNext();
            if (lex.peakNext().type == TYPE_INT)
                lex.getNext();
            break;
        case TYPE_INT:
            lex.getNext();
            break;
        default:
            SyntaxError("Unexpected token");
            break;
    }
    return env->factory.newIntegral(type, is_signed);
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
        StringRef tag = lex.getNext().symbol;
        // TODO: not defined yet, incomplete type?
        return env->find(SC_TAG, tag)->type;
    }
    else
    {
        // tag
        StringRef tag("<anonymous-struct>");
        if (lex.peakNext().type == SYMBOL)
            tag = lex.getNext().symbol;

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

        env->add(struct_tag);

        return st;
    }
}
TypeBase *__parseEnumType(Lexer &lex, Environment *env)
{
    EXPECT(TYPE_ENUM);
    if (lex.peakNext().type == SYMBOL && lex.peakNext(1).type != BLK_BEGIN)
    {
        // use defined enum
        StringRef tag = lex.getNext().symbol;
        Symbol *s = env->find(SC_TAG, tag);
        if (s == nullptr)
            SyntaxError("Unknown enum type '" + tag.toString() + "'");
        return s->type;
    }
    else
    {
        // tag
        StringRef tag("<anonymous-enum>");
        if (lex.peakNext().type == SYMBOL)
            tag = lex.getNext().symbol;

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
            StringRef id = EXPECT_GET(SYMBOL).symbol;
            // new enum constant
            Symbol *e = new Symbol();
            e->category = SC_ENUM_CONST;
            e->name = id;
            e->type = et;
            e->value = value++;
            env->add(e);
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

        env->add(enum_tag);

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
            s->position = 0;
            params.push_back(s);
            if (lex.peakNext().type == RP)
                break;
            else if (lex.peakNext().type == OP_COMMA)
                lex.getNext();
            else
                SyntaxErrorEx("Unexpected token");
        }
    }
    EXPECT(RP);

    FuncType *f = new FuncType();
    f->env = new Environment(params.size());
    // f->env->setParent(env);

    // calculate parameter position
    // XXX: starting from 1
    long pos = 0;
    for (auto p = params.begin(); p != params.end(); ++p)
    {
        (*p)->position = ++pos;
        f->env->add(*p);
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
        case TYPE_CHAR: t = __parseIntegralType(lex, env); break;
        case TYPE_LONG:
            if (lex.peakNext(1).type == TYPE_DOUBLE)
                t = __parseFloatingType(lex);
            else
                t = __parseIntegralType(lex, env);
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
        symbol = lex.getNext().symbol;
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
    StringRef symbol("<anonymous-symbol>");
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

        Symbol *f = new Symbol();
        f->category = SC_ID;
        f->name = symbol;
        f->type = func;
        f->addr = 0;

        // add func declaration before parsing its body
        env->add(f);
        func->body = CompoundStatement::parse(lex, func->env, true);
    }
    else
    {
        // Declaration
        while (true)
        {
            if (symbol == "<anonymous-symbol>")
            {
                if (spec->type() == TC_STRUCT || spec->type() == TC_ENUM ||
                    spec->type() == TC_UNION)
                    break;
                else
                    SyntaxWarningEx("Missing identifier");
            }
            Symbol *v = new Symbol();
            v->category = SC_ID;
            v->name = symbol;
            v->type = decl;
            v->addr = 0;
            env->add(v);
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
