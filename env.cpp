#define HAS_LEXER

#include "env.h"

#include "parser.h"
#include "symbol.h"

#include <cctype>
#include <iostream>
using namespace std;

// ####### TypeBase ######
size_t TypeFactory::ArraySize(const TypeBase &t)
{
    if (t.type() != T_ARRAY)
        SyntaxError("ArraySize: expect array type.");

    size_t size = 1;
    size_t tmp = 0;
    const char *p = t.desc.data() + 1;
    while (sscanf(p, "_%lu", &tmp) > 0)
    {
        size *= tmp;
        tmp = 0;
        do
        {
            ++p;
        } while (isdigit(*p));
        if (*p == '\0')
            SyntaxError("ArraySize: unexpected EOF");
        if (*p == '$')
            break;
    }
    ++p;
    TypeBase *target = TypeFactory::newInstanceFromCStr(p, strlen(p));
    size *= TypeBase::Sizeof(*target);

    return size;
}
size_t TypeFactory::StructSize(const TypeBase &t)
{
    if (t.type() != T_STRUCT)
        SyntaxError("StructSize: expect array type.");

    size_t size = 1;
    const char *p = t.desc.data() + 1;
    const char *q;
    while (*p == '_')
    {
        ++p;
        for (q = p; *q != '_'; ++q)
        {
            if (*q == '\0')
                SyntaxError("StructSize: unexpected EOF");
            if (*q == '$')
                break;
        }
        assert(q > p);
        TypeBase *target = TypeFactory::newInstanceFromCStr(p, (size_t)(q - p));
        size *= TypeBase::Sizeof(*target);
        p = q;
    }
    assert(*p == '$');

    return size;
}
size_t TypeBase::Sizeof(const TypeBase &t)
{
    size_t size = 0;
    switch (t.type())
    {
        case T_INT: sscanf(t.desc.data() + 1, "%lu", &size); break;
        case T_POINTER: size = sizeof(void *); break;
        case T_FUNCTION:
            // case T_LABEL:
            break;
        case T_ARRAY: size = TypeFactory::ArraySize(t); break;
        case T_STRUCT: size = TypeFactory::StructSize(t); break;
        case T_ENUM: size = sizeof(int); break;
        default:
            SyntaxError("TypeBase: not implemented. " + t.toString());
            break;
    }
    return size;
}
void TypeBase::ConstructObject(Object *&o, const TypeBase &t)
{
    if (t.isIncomplete())
        SyntaxError(
            "ConstructObject: can't construct object of incomplete type.");
    size_t size = Sizeof(t);
    switch (t.type())
    {
        case T_INT: o = new IntegerObject(size); break;
        case T_FLOAT: o = new FloatObject(size); break;
        case T_POINTER: o = new PointerObject(size); break;
        case T_ARRAY: o = new ArrayObject(size); break;
        case T_FUNCTION:
            // function object is handled differently, do nothing here
            break;
        case T_STRUCT: o = new StructObject(size); break;
        case T_ENUM: o = new EnumObject(size); break;
        case T_NONE:
            SyntaxError("ConstructObject: null type has no object..");
            break;
        default: SyntaxError("ConstructObject: not implemented."); break;
    }
}
// base type interface
bool TypeBase::hasOperation(ETypeOperations op) const
{
    const int f_int = TOp_ADD | TOp_SUB | TOp_INC | TOp_DEC | TOp_MUL |
                      TOp_DIV | TOp_MOD | TOp_ASSIGN;
    // const int f_float = TOp_ADD | TOp_SUB | TOp_INC | TOp_DEC | TOp_MUL |
    // TOp_DIV | TOp_ASSIGN;
    const int f_pointer = TOp_ADD | TOp_SUB | TOp_INC | TOp_DEC | TOp_EVAL |
                          TOp_ADDR | TOp_ASSIGN | TOp_INDEX;

    bool has = false;
    switch (type())
    {
        case T_INT: has = f_int & op; break;
        case T_POINTER: has = f_pointer & op; break;
        default: break;
    }
    return has;
}
// concrete type interface
const PointerType *TypeBase::asPointerType() const
{
    return static_cast<const PointerType *>(this);
}
const TypeBase *PointerType::target() const
{
    return TypeFactory::newInstanceFromCStr(desc.data(), desc.size());
}

vector<TypeBase *> TypeFactory::references;
vector<Symbol *> SymbolFactory::references;

// -------- symbol management -------

/*
Symbol *SymbolTable::find(ESymbolCategory category, StringRef name) const
{
    for (auto s = symbols.begin(); s != symbols.end(); ++s)
    {
        if ((*s)->category == category && (*s)->name == name)
            return *s;
    }
    return nullptr;
}
void SymbolTable::debugPrint() const
{
    cout << "[Symbol Table] " << symbols.size() << " symbols" << endl;
    for (const Symbol *s : symbols)
    {
        cout << "  " << (*s) << endl;
        if (s->type)
        {
            switch (s->type->type())
            {
                case T_STRUCT:
                    dynamic_cast<StructType *>(s->type)->debugPrint();
                    break;
                case T_FUNC:
                    if (dynamic_cast<FuncType *>(s->type)->body)
                        // cout << "\t{ ... }" << endl;
                        __debugPrint(dynamic_cast<FuncType *>(s->type)
                                         ->body->debugString());
                    break;
                default: break;
            }
        }
    }
}
*/

Symbol *Environment::find(ESymbolNamespace space, StringRef name) const
{
    for (Symbol *s : symbols)
    {
        if (s->space == space && s->name == name)
            return s;
    }
    return nullptr;
}
Symbol *Environment::recursiveFind(ESymbolNamespace space, StringRef name) const
{
    Symbol *s = find(space, name);
    if (s != nullptr)
        return s;
    else
        return parent() ? parent()->recursiveFind(space, name) : nullptr;
}
void Environment::add(Symbol *s)
{
    assert(s != nullptr);
    Symbol *origin = find(s->space, s->name);
    if (origin != nullptr)
    {
        // merge type
        TypeBase *t1 = origin->type;
        TypeBase *t2 = s->type;
        assert(t1 && t2);
        if (!TypeFactory::MergeRight(*t1, *t2))
            SyntaxError("Symbol '" + s->name.toString() + "' already defined.");

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

// -------- code generation --------
int Environment::idgen = 0;
void Environment::emit()
{
    /*
    if (isRoot())  // global scope
    {
        // generate code for function bodies
        for (const Symbol *s : symbols)
        {
            if (s->type->type() == T_FUNCTION)
            {
                FuncObject *func = dynamic_cast<FuncObject *>(s->obj);
                if (func->body)
                {
                    EmitDecl(".global _%s", s->name.toString().data());
                    Emit("_%s:", s->name.toString().data());
                    Emit("pushq %%rbp");
                    Emit("movq %%rsp, %%rbp");
                    Emit("subq $64, %%rsp");
                    if (func->env->paramcnt >= 4)
                        Emit("movq %%rcx, -32(%%rbp)");
                    func->body->emit(this, FOR_NOTHING);
                }
                else
                    EmitDecl(".extern _%s", s->name.toString().data());
                ;  // SyntaxWarning("function '" + s->name.toString() + "' has
                   // no body.");
            }
            else
                switch (s->type->type())
                {
                    case T_INT:
                    case T_POINTER:
                        EmitData("_%s: .quad 0", s->name.toString().data());
                        break;
                    default:
                        SyntaxError("Environment: type code not implemented");
                        break;
                }
        }
    }
    else  // block scope
    {
        if (paramcnt > 6)
            SyntaxError("Not implemented.");
    }
    */
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

// -------- debug  --------
void __debugPrint(string &&s)
{
    string tabs = "  ";
    string line = "  ";
    bool escape = false, empty = true;
    for (char c : s)
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
                case '<':
                    tabs.pop_back();
                    tabs.pop_back();
                    break;
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
void Environment::debugPrint(int indent) const
{
    for (Symbol *s : symbols)
    {
        printf("%*s %-12sType: %-*sObject: %s\n", indent + 5,
               s->space == SC_ID ? "Name:"
                                 : (s->space == SC_TAG ? "Tag: " : "Labl:"),
               s->name.toString().data(), 33 - indent,
               s->type ? s->type->toString().data() : "<null>",
               s->obj
                   ? (s->obj->toString() + ", " + to_string(s->obj->size()) +
                      " bytes.")
                         .data()
                   : "<null>");
        if (s->obj)
        {
            FuncObject *fo = dynamic_cast<FuncObject *>(s->obj);
            if (fo)
            {
                if (fo->getFuncEnv())
                    fo->getFuncEnv()->debugPrint(indent + 2);
                if (fo->getFuncBody())
                    __debugPrint(fo->getFuncBody()->debugString());
            }
        }
    }

    /*
    int i = 1;
    for (auto *child : getChildren())
    {
        printf(">> child [%d]\n", i);
        child->debugPrint(indent + 4);
        printf("<< child [%d]\n", i);
        ++i;
    }
    */
}

// -------- declaration parsing --------
// TODO: support declarator without name

void __parseIntegralType(TypeBase &t, Lexer &lex);
void __parseFloatingType(TypeBase &t, Lexer &lex);
bool __parseStructType(TypeBase &t, Lexer &lex, Environment *env);
bool __parseEnumType(TypeBase &t, Lexer &lex, Environment *env);
bool __parseDeclarator(Symbol &s, Lexer &lex, Environment *env);

bool __parseSpecifier(TypeBase &t, Lexer &lex, Environment *env)
{
    bool matched = true;
    switch (lex.peakNext().type)
    {
        case TYPE_VOID:
            TypeFactory::Void(t);
            lex.getNext();
            break;
        case SIGNED:
        case UNSIGNED:
        case TYPE_INT:
        case TYPE_SHORT:
        case TYPE_CHAR: __parseIntegralType(t, lex); break;
        case TYPE_LONG:
            if (lex.peakNext(1).type == TYPE_DOUBLE)
                __parseFloatingType(t, lex);
            else
                __parseIntegralType(t, lex);
            break;
        case TYPE_DOUBLE:
        case TYPE_FLOAT: __parseFloatingType(t, lex); break;
        case TYPE_ENUM: __parseEnumType(t, lex, env); break;
        case TYPE_UNION:
            SyntaxError("Specifier: not implemented");
            break;
        // case TYPE_UNION: __parseUnionType(t, lex, env); break;
        case TYPE_STRUCT: __parseStructType(t, lex, env); break;
        default: matched = false; break;
    }
    return matched;
}
void __parseIntegralType(TypeBase &t, Lexer &lex)
{
    if (lex.peakNext().type == TYPE_CHAR)
    {
        lex.getNext();
        TypeFactory::Char(t);
        return;
    }

    bool is_signed = true;
    switch (lex.peakNext().type)
    {
        case SIGNED: lex.getNext(); break;
        case UNSIGNED:
            lex.getNext();
            is_signed = false;
            break;
        default: break;
    }

    size_t length = 4;
    switch (lex.peakNext().type)
    {
        case TYPE_CHAR:
            length = 1;
            lex.getNext();
            break;
        case TYPE_SHORT:
            length = 2;
            lex.getNext();
            if (lex.peakNext().type == TYPE_INT)
                lex.getNext();
            break;
        case TYPE_INT:
            length = 4;
            lex.getNext();
            break;
        case TYPE_LONG:
            length = 8;
            lex.getNext();
            if (lex.peakNext().type == TYPE_INT)
                lex.getNext();
            break;
        default: SyntaxWarning("Specifier: default 'int' type"); break;
    }

    TypeFactory::Integer(t, is_signed, length);
}
void __parseFloatingType(TypeBase &t, Lexer &lex)
{
    size_t length = 0;
    switch (lex.peakNext().type)
    {
        case TYPE_FLOAT:
            length = 4;
            lex.getNext();
            break;
        case TYPE_DOUBLE:
            length = 8;
            lex.getNext();
            break;
        default:
            SyntaxError("Specifier: type 'long double' not supported");
            break;
    }

    TypeFactory::Float(t, length);
}
bool __parseStructType(TypeBase &t, Lexer &lex, Environment *env)
{
    EXPECT(TYPE_STRUCT);
    /*
    if (lex.peakNext().type == SYMBOL && lex.peakNext(1).type != BLK_BEGIN)
    {
        StringRef tag = lex.getNext().symbol;

        Symbol *s = env->recursiveFind(SC_TAG, tag);
        if (s == nullptr)
            SyntaxError("struct '" + tag.toString() + "' not defined.");
        else if (s->type->type() != T_STRUCT)
            SyntaxError("can't find struct '" + tag.toString() + "'.");
        else
        {
            assert( s->type != nullptr );
            t += (*s->type);
        }

        return true;
    }
    */

    TypeBase *st = TypeFactory::newInstance();
    TypeFactory::StructStart(*st);

    // tag
    StringRef tag("<anonymous-struct>");
    {
        if (lex.peakNext().type == SYMBOL)
            tag = lex.getNext().symbol;
        TypeFactory::StructName(*st, tag);
    }

    // add tag to environment temporarily
    {
        TypeBase *tmp = TypeFactory::newInstance();
        *tmp = *st;
        assert(tmp->isIncomplete() == true);
        TypeFactory::StructEnd(*tmp);

        Symbol *sym = SymbolFactory::newTag(tag, tmp);
        env->add(sym);
    }

    // struct members
    if (lex.peakNext().type == BLK_BEGIN)
    {
        EXPECT(BLK_BEGIN);
        TypeFactory::StructBodyBegin(*st);
        while (true)
        {
            TypeBase spec;
            __parseSpecifier(spec, lex, env);
            while (true)
            {
                StringRef id("<anonymous-struct-member>");
                Symbol s;
                __parseDeclarator(s, lex, env);
                assert(s.type != nullptr);
                (*s.type) += spec;
                TypeFactory::StructMember(*st, *s.type);

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
        TypeFactory::StructBodyEnd(*st);
        TypeFactory::StructEnd(*st);

        // add completed tag to environment
        {
            assert(st->isIncomplete() == false);
            Symbol *sym = SymbolFactory::newTag(tag, st);
            env->add(sym);
        }
    }
    else
    {
        TypeFactory::StructEnd(*st);
    }

    t += *st;

    return true;
}
bool __parseEnumType(TypeBase &t, Lexer &lex, Environment *env)
{
    EXPECT(TYPE_ENUM);
    /*
    if (lex.peakNext().type == SYMBOL && lex.peakNext(1).type != BLK_BEGIN)
    {
        StringRef tag = lex.getNext().symbol;

        Symbol *s = env->recursiveFind(SC_TAG, tag);
        if (s == nullptr)
            SyntaxError("enum '" + tag.toString() + "' not defined.");
        else if (s->type->type() != T_STRUCT)
            SyntaxError("can't find enum '" + tag.toString() + "'.");
        else
        {
            assert( s->type != nullptr );
            t += (*s->type);
        }

        return true;
    }
    */

    TypeBase *st = TypeFactory::newInstance();
    TypeFactory::EnumStart(*st);

    // tag
    StringRef tag("<anonymous-enum>");
    {
        if (lex.peakNext().type == SYMBOL)
            tag = lex.getNext().symbol;
        TypeFactory::EnumName(*st, tag);
    }

    // enum constants
    if (lex.peakNext().type == BLK_BEGIN)
    {
        // int value = 0;
        EXPECT(BLK_BEGIN);
        TypeFactory::EnumBodyBegin(*st);
        while (true)
        {
            if (lex.peakNext().type != SYMBOL)
                SyntaxErrorEx("Enum: unexpected token");

            StringRef name = EXPECT_GET(SYMBOL).symbol;
            // new enum constant
            Symbol *e = SymbolFactory::newInstance();
            e->space = SC_ID;
            e->name = name;
            e->type = st;
            // TODO: fill e->type, e->obj(has value info)
            env->add(e);

            if (lex.peakNext().type == OP_COMMA)
                lex.getNext();
            else if (lex.peakNext().type == BLK_END)
                break;
        }
        EXPECT(BLK_END);
        TypeFactory::EnumBodyEnd(*st);
        TypeFactory::EnumEnd(*st);

        // add completed tag to environment
        {
            assert(st->isIncomplete() == false);
            Symbol *sym = SymbolFactory::newTag(tag, st);
            env->add(sym);
        }
    }
    else
    {
        TypeFactory::EnumEnd(*st);
    }

    t += *st;

    return true;
}
// bool __parseUnionType(TypeBase &t, Lexer &lex, Environment *env)

void __parseParameterList(TypeBase &t, Lexer &lex, Environment *env,
                          FuncObject *&func_obj);
// bool __parseDimensionList(TypeBase &t, Lexer &lex, ArrayObject * &array_obj);
bool __parseDimensionList(TypeBase &t, Lexer &lex);
bool __parsePointerList(TypeBase &t, Lexer &lex);

bool __parseDeclarator(Symbol &s, Lexer &lex, Environment *env)
{
    TokenType tt = lex.peakNext().type;
    if (tt != SYMBOL && tt != OP_MUL && tt != LP)
    {
        SyntaxWarningEx("Declarator: unexpected token");
        return false;
    }

    if (s.type == nullptr)
        s.type = TypeFactory::newInstance();

    TypeBase pointer, tail;

    if (lex.peakNext().type == OP_MUL)
        __parsePointerList(pointer, lex);

    // get name? go deeper?
    if (lex.peakNext().type == SYMBOL)
        s.name = lex.getNext().symbol;
    else
    {
        EXPECT(LP);
        __parseDeclarator(s, lex, env);
        // only the host function parameter-list needs env
        EXPECT(RP);
    }

    // more object info?
    if (lex.peakNext().type == LP)
    {
        if (s.type->type() == T_NONE)
        {
            assert(s.obj == nullptr);
            FuncObject *obj = new FuncObject();
            __parseParameterList(tail, lex, env, obj);
            s.obj = obj;
        }
        else
        {
            FuncObject *obj = nullptr;
            __parseParameterList(tail, lex, env, obj);
        }
    }
    else if (lex.peakNext().type == LSB)
    {
        __parseDimensionList(tail, lex);
    }

    assert(s.type != nullptr);
    (*s.type) += tail;
    (*s.type) += pointer;

    return true;
}
// 1. function signature (TypeBase)
// 2. function parameters (vector<Symbol *>)
void __parseParameterList(TypeBase &t, Lexer &lex, Environment *env,
                          FuncObject *&func_obj)
{
    if (func_obj)
    {
        func_obj->setFuncEnv(new Environment());
        // func_obj->body = nullptr;
    }

    TypeFactory::FunctionStart(t);
    EXPECT(LP);
    if (lex.peakNext().type != RP)
    {
        // parsing function parameters
        for (size_t loc = 0;; ++loc)
        {
            assert(loc < 100);
            Symbol s;
            TypeBase spec;
            __parseSpecifier(spec, lex, env);
            __parseDeclarator(s, lex, env);
            assert(s.type != nullptr);
            (*s.type) += spec;

            TypeFactory::FunctionParameter(t, *s.type);

            s.space = SC_ID;
            if (func_obj)
            {
                func_obj->addParameter(s.name, loc);
                func_obj->getFuncEnv()->add(SymbolFactory::newInstance(s));
            }

            if (lex.peakNext().type == RP)
                break;
            else if (lex.peakNext().type == OP_COMMA)
                lex.getNext();
            else
                SyntaxErrorEx("Unexpected token");
        }
    }
    EXPECT(RP);
    TypeFactory::FunctionEnd(t);
}
// bool __parseDimensionList(TypeBase &t, Lexer &lex, ArrayObject * &array_obj)
bool __parseDimensionList(TypeBase &t, Lexer &lex)
{
    if (lex.peakNext().type != LSB)
        return false;

    int dim = 0;
    TypeFactory::ArrayStart(t);
    while (lex.peakNext().type == LSB)
    {
        lex.getNext();
        dim = EXPECT_GET(CONST_INT).ival;
        TypeFactory::ArrayParameter(t, dim);
        // if (array_obj)
        //     ObjectFactory::ArrayParameter(array_obj, dim);
        // ConstExpression::eval(CondExpression::parse(lex));
        EXPECT(RSB);
    }
    TypeFactory::ArrayEnd(t);

    return true;
}
bool __parsePointerList(TypeBase &t, Lexer &lex)
{
    if (lex.peakNext().type != OP_MUL)
        return false;

    bool is_const = false;
    while (lex.peakNext().type == OP_MUL)
    {
        lex.getNext();
        if (lex.peakNext().type == CONST)
        {
            lex.getNext();
            is_const = true;
        }
        TypeFactory::Pointer(t, is_const);
    }
    return true;
}

void __parseDeclaration(Lexer &lex, Environment *env, bool is_global)
{
    TypeBase spec;
    Symbol *s = SymbolFactory::newInstance();
    s->name = StringRef("<anonymous-symbol>");
    s->space = SC_ID;

    bool has_specifier = __parseSpecifier(spec, lex, env);
    __parseDeclarator(*s, lex, env);
    (*s->type) += spec;

    // parse declaration and add to env
    if (lex.peakNext().type == BLK_BEGIN)  // Function definition
    {
        if (!is_global)
            SyntaxError("Function definition not allowed here.");

        assert(s->name != "<anonymous-symbol>");
        assert(s->obj != nullptr);
        FuncObject *obj = dynamic_cast<FuncObject *>(s->obj);
        assert(obj != nullptr);
        obj->getFuncEnv()->setParent(env);
        // add func declaration before parsing its body
        env->add(s);

        // TODO: finish this
        obj->setFuncBody(
            CompoundStatement::parse(lex, obj->getFuncEnv(), true));
    }
    else  // Normal declaration
    {
        if (!has_specifier)
            SyntaxErrorDebug("Declaration: missing specifiers");

        while (true)
        {
            if (s->name == "<anonymous-symbol>")
            {
                SyntaxWarningEx("Missing identifier");
                break;
            }

            // Function object is handled differently, see above
            TypeBase::ConstructObject(s->obj, *(s->type));
            env->add(s);

            if (lex.peakNext().type == OP_COMMA)
            {
                lex.getNext();
                s = SymbolFactory::newInstance();
                s->name = StringRef("<anonymous-symbol>");
                s->space = SC_ID;
                __parseDeclarator(*s, lex, env);
                (*s->type) += spec;
            }
            else
                break;
        }
        EXPECT(STMT_END);
    }
}

// ----------------

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
