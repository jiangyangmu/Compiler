#define HAS_LEXER

#include "env.h"

#include "parser.h"
#include "symbol.h"

#include <cctype>
#include <iostream>
using namespace std;

// -------- type management -------
// sizeof
size_t TypeFactory::ArraySize(const TypeBase &t, const Environment *env)
{
    if (t.type() != T_ARRAY)
        SyntaxError("ArraySize: expect array type.");
    if (t.isIncomplete(env))
        SyntaxError("ArraySize: incomplete array type.");

    size_t size = 1;
    size_t tmp = 0;
    const char *p = t._desc.data() + 1;

    int ret = sscanf(p, "%lu", &tmp);
    assert(ret > 0);
    size *= tmp;
    do { ++p; } while (isdigit(*p));
    assert(*p != '\0');

    TypeBase *target = TypeFactory::ReadType(p, env);
    size *= TypeFactory::Sizeof(*target, env);

    return size;
}
size_t TypeFactory::StructSize(const TypeBase &t, const Environment *env)
{
    if (t.type() != T_STRUCT)
        SyntaxError("StructSize: expect struct type.");

    // TODO: recursive search here?
    const TypeBase *def = ((const StructType &)t).getDefinition(env, true);
    assert(def != nullptr);
    if (!def->_complete)
        SyntaxError("StructSize: get size of incomplete struct: " +
                    def->toString());

    // read size info
    size_t size = 0;
    const char *p = def->_desc.data() + 1;
    while (*p != '$')
    {
        assert(*p != '\0');
        TypeBase *target = TypeFactory::ReadType(p, env);
        size += TypeFactory::Sizeof(*target, env);
    }
    assert(*p == '$');

    return size;
}
size_t TypeFactory::Sizeof(const TypeBase &t, const Environment *env)
{
    if (t.isIncomplete(env))
        SyntaxError(
            "Sizeof: can't get size of incomplete type.");
    size_t size = 0;
    switch (t.type())
    {
        // leaf node
        case T_CHAR: size = sizeof(char); break;
        case T_INT: sscanf(t._desc.data() + 1, "%lu", &size); break;
        case T_POINTER: size = sizeof(void *); break;
        case T_ENUM: size = sizeof(int); break;
        // internal node
        case T_ARRAY: size = TypeFactory::ArraySize(t, env); break;
        case T_STRUCT: size = TypeFactory::StructSize(t, env); break;
        case T_VOID:
            SyntaxError("Sizeof: can't get size of void type. ");
            break;
        case T_FUNCTION:
            // case T_LABEL:
            SyntaxError("Sizeof: can't get size of function type. " + t.toString());
            break;
        default:
            SyntaxError("Sizeof: not implemented. " + t.toString());
            break;
    }
    return size;
}
void TypeFactory::ConstructObject(Object *&o, const TypeBase &t, const Environment *env)
{
    switch (t.type())
    {
        case T_INT: o = new IntegerObject(Sizeof(t, env)); break;
        case T_FLOAT: o = new FloatObject(Sizeof(t, env)); break;
        case T_POINTER: o = new PointerObject(Sizeof(t, env)); break;
        case T_ENUM: o = new EnumObject(Sizeof(t, env)); break;
        // lazy evaluation
        case T_ARRAY: o = new ArrayObject(&t, env); break;
        case T_STRUCT: o = new StructObject(&t, env); break;
        case T_FUNCTION:
            // function object is handled differently, do nothing here
            break;
        case T_NONE:
            SyntaxError("ConstructObject: null type has no object..");
            break;
        default: SyntaxError("ConstructObject: not implemented."); break;
    }
}
// incomplete
bool TypeBase::isIncomplete(const Environment *env, bool recursive) const
{
    bool is = true;
    switch (type())
    {
        case T_VOID:
        case T_CHAR:
        case T_INT:
        case T_FLOAT:
        case T_POINTER:
        case T_ARRAY:
        case T_FUNCTION: is = !_complete; break;
        case T_STRUCT: is = ((const StructType *)this)->isIncomplete(env, recursive); break;
        case T_ENUM: is = ((const EnumType *)this)->isIncomplete(env, recursive); break;
        default: SyntaxError("isIncomplete: unknown type: " + _desc); break;
    }
    return is;
}
// must be tag, not definition
bool StructType::isIncomplete(const Environment *env, bool recursive) const
{
    // Completeness of members are ensured during struct construction.
    const TypeBase *def = getDefinition(env, recursive);
    return def && !def->_complete;
}
bool EnumType::isIncomplete(const Environment *env, bool recursive) const
{
    const TypeBase *def = getDefinition(env, recursive);
    return def && !def->_complete;
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
StringRef StructType::getTag() const
{
    assert(_desc.size() >= 2);
    StringRef tag(_desc.data() + 1, _desc.size() - 2);
    assert(!tag.empty());
    return tag;
}
StringRef EnumType::getTag() const
{
    assert(_desc.size() >= 2);
    StringRef tag(_desc.data() + 1, _desc.size() - 2);
    assert(!tag.empty());
    return tag;
}
const TypeBase *StructType::getDefinition(const Environment *env,
                                          bool recursive) const
{
    assert(env != nullptr);
    Symbol *sym = recursive ? env->recursiveFindDefinition(getTag())
                            : env->findDefinition(getTag());
    return sym ? sym->type : nullptr;
}
const TypeBase *EnumType::getDefinition(const Environment *env,
                                        bool recursive) const
{
    assert(env != nullptr);
    Symbol *sym = recursive ? env->recursiveFindDefinition(getTag())
                            : env->findDefinition(getTag());
    return sym ? sym->type : nullptr;
}

vector<TypeBase *> TypeFactory::references;
vector<Symbol *> SymbolFactory::references;

// -------- object management -------
// delay time to read size
size_t Object::size()
{
    if (_need_read)
    {
        assert(_env != nullptr);
        _size = TypeFactory::Sizeof(*_type, _env);
        _need_read = false;
    }
    return _size;
}

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
Symbol *Environment::findDefinition(StringRef name) const
{
    return find(SC_TAG, name);
}
Symbol *Environment::recursiveFindDefinition(StringRef name) const
{
    Symbol *s = find(SC_TAG, name);

    if (s == nullptr)
        return parent() ? parent()->recursiveFindDefinition(name) : nullptr;

    assert(s->type != nullptr);
    if (!s->type->isIncompleteSimple())
        return s;

    Symbol *ps = parent() ? parent()->recursiveFindDefinition(name) : nullptr;
    return (ps && !ps->type->isIncompleteSimple()) ? ps : s;
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
        if (!TypeFactory::MergeDefinition(*t1, *t2))
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
    bool printenv = false;
    uintptr_t env = 0;
    for (char c : s)
    {
        if (printenv)
        {
            if (c != '\n')
            {
                env *= 10;
                env += c - '0';
            }
            else
            {
                ((const Environment *)env)->debugPrint(tabs.size());
                printenv = false;
                env = 0;
            }
            continue;
        }

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
    if (indent == 0)
    {
        printf("%lu types. %lu symbols\n", TypeFactory::size(), SymbolFactory::size());
    }
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
                // if (fo->getFuncEnv())
                //     fo->getFuncEnv()->debugPrint(indent + 2);
                if (fo->getFuncBody())
                    __debugPrint(fo->getFuncBody()->debugString());
            }
        }
    }
}

// -------- declaration parsing --------
// TODO: support declarator without name

TypeBase *__parseIntegralType(Lexer &lex);
TypeBase *__parseFloatingType(Lexer &lex);
TypeBase *__parseStructType(Lexer &lex, Environment *env);
TypeBase *__parseEnumType(Lexer &lex, Environment *env);
bool __parseDeclarator(Symbol &s, Lexer &lex, Environment *env);

TypeBase *__parseSpecifier(Lexer &lex, Environment *env)
{
    TypeBase *s = nullptr;
    switch (lex.peakNext().type)
    {
        case TYPE_VOID:
            s = TypeFactory::Void();
            lex.getNext();
            break;
        case SIGNED:
        case UNSIGNED:
        case TYPE_INT:
        case TYPE_SHORT:
        case TYPE_CHAR: s = __parseIntegralType(lex); break;
        case TYPE_LONG:
            if (lex.peakNext(1).type == TYPE_DOUBLE)
                s = __parseFloatingType(lex);
            else
                s = __parseIntegralType(lex);
            break;
        case TYPE_DOUBLE:
        case TYPE_FLOAT: s = __parseFloatingType(lex); break;
        case TYPE_ENUM: s = __parseEnumType(lex, env); break;
        case TYPE_STRUCT: s = __parseStructType(lex, env); break;
        case TYPE_UNION:
            SyntaxError("Specifier: not implemented");
            break;
        default: break;
    }
    return s;
}
TypeBase *__parseIntegralType(Lexer &lex)
{
    if (lex.peakNext().type == TYPE_CHAR)
    {
        lex.getNext();
        return TypeFactory::Char();
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

    return TypeFactory::Integer(length, is_signed);
}
TypeBase *__parseFloatingType(Lexer &lex)
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

    return TypeFactory::Float(length);
}
TypeBase *__parseStructType(Lexer &lex, Environment *env)
{
    EXPECT(TYPE_STRUCT);

    StringRef tag("<anonymous-struct>");
    {
        if (lex.peakNext().type == SYMBOL)
            tag = lex.getNext().symbol;
    }

    // add empty struct to environment temporarily,
    // so we can define things like:
    //      struct self pointer,
    //      function pointer return struct self.
    {
        TypeBase *st_empty = TypeFactory::Struct();
        TypeFactory::StructBegin(st_empty);
        TypeFactory::StructEnd(st_empty);
        Symbol *sym = SymbolFactory::newTag(tag, st_empty);
        env->add(sym);
    }

    // the real struct
    if (lex.peakNext().type == BLK_BEGIN)
    {
        EXPECT(BLK_BEGIN);
        TypeBase *st = TypeFactory::Struct();
        TypeFactory::StructBegin(st);
        TypeFactory::StructBodyBegin(st);
        while (true)
        {
            TypeBase *spec = __parseSpecifier(lex, env);
            while (true)
            {
                StringRef id("<anonymous-struct-member>");
                Symbol s;
                __parseDeclarator(s, lex, env);
                assert(s.type != nullptr);
                TypeFactory::AppendRight(*s.type, *spec, env);
                TypeFactory::StructMember(st, *s.type, env);

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
        TypeFactory::StructBodyEnd(st);
        TypeFactory::StructEnd(st);

        // add completed struct to environment
        {
            Symbol *sym = SymbolFactory::newTag(tag, st);
            env->add(sym);
        }
    }

    // return struct tag
    return TypeFactory::StructTag(tag);
}
TypeBase *__parseEnumType(Lexer &lex, Environment *env)
{
    EXPECT(TYPE_ENUM);

    // tag
    StringRef tag("<anonymous-enum>");
    {
        if (lex.peakNext().type == SYMBOL)
            tag = lex.getNext().symbol;
    }

    // enum constants
    if (lex.peakNext().type == BLK_BEGIN)
    {
        TypeBase *en = TypeFactory::Enum(tag);
        // int value = 0;
        EXPECT(BLK_BEGIN);
        while (true)
        {
            if (lex.peakNext().type != SYMBOL)
                SyntaxErrorEx("Enum: unexpected token");

            StringRef name = EXPECT_GET(SYMBOL).symbol;
            // new enum constant
            Symbol *e = SymbolFactory::newInstance();
            e->space = SC_ID;
            e->name = name;
            e->type = en;
            // TODO: fill e->type, e->obj(has value info)
            env->add(e);

            if (lex.peakNext().type == OP_COMMA)
                lex.getNext();
            else if (lex.peakNext().type == BLK_END)
                break;
        }
        EXPECT(BLK_END);

        // add completed enum to environment
        {
            Symbol *sym = SymbolFactory::newTag(tag, en);
            env->add(sym);
        }
    }

    return TypeFactory::EnumTag(tag);
}
// bool __parseUnionType(TypeBase &t, Lexer &lex, Environment *env)

TypeBase *__parseParameterList(Lexer &lex, Environment *env,
                          FuncObject *&func_obj);
TypeBase *__parseDimensionList(Lexer &lex);
TypeBase *__parsePointerList(Lexer &lex);

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

    TypeBase *pointer = nullptr;
    TypeBase *tail = nullptr;

    if (lex.peakNext().type == OP_MUL)
        pointer = __parsePointerList(lex);

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
            tail = __parseParameterList(lex, env, obj);
            s.obj = obj;
        }
        else
        {
            FuncObject *obj = nullptr;
            tail = __parseParameterList(lex, env, obj);
        }
    }
    else if (lex.peakNext().type == LSB)
    {
        tail = __parseDimensionList(lex);
    }

    assert(s.type != nullptr);
    if (tail) TypeFactory::AppendRight(*s.type, *tail, env);
    if (pointer) TypeFactory::AppendRight(*s.type, *pointer, env);

    return true;
}
// 1. function signature (TypeBase)
// 2. function parameters (vector<Symbol *>)
TypeBase *__parseParameterList(Lexer &lex, Environment *env,
                          FuncObject *&func_obj)
{
    if (func_obj)
    {
        Environment *func_env = new Environment();
        func_env->setParent(env);
        func_obj->setFuncEnv(func_env);
        // func_obj->body = nullptr;
    }

    TypeBase *f = TypeFactory::Function();
    TypeFactory::FunctionBegin(f);
    EXPECT(LP);
    if (lex.peakNext().type != RP)
    {
        // parsing function parameters
        for (size_t loc = 0;; ++loc)
        {
            assert(loc < 100);
            TypeBase *spec = __parseSpecifier(lex, env);
            Symbol s;
            __parseDeclarator(s, lex, env);
            assert(s.type != nullptr);
            TypeFactory::AppendRight(*s.type, *spec, env);

            TypeFactory::FunctionParameter(f, *s.type);

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
    TypeFactory::FunctionEnd(f);

    return f;
}
TypeBase *__parseDimensionList(Lexer &lex)
{
    TypeBase *a = nullptr;

    if (lex.peakNext().type == LSB)
    {
        lex.getNext();
        if (lex.peakNext().type == CONST_INT)
        {
            // ConstExpression::eval(CondExpression::parse(lex));
            a = TypeFactory::Array(lex.getNext().ival);
        }
        else if (lex.peakNext().type == RSB)
        {
            a = TypeFactory::ArrayIncomplete();
        }
        else
            SyntaxError("array expects an integer.");
        EXPECT(RSB);

        TypeBase *sub = __parseDimensionList(lex);
        if (sub) TypeFactory::AppendRight(*a, *sub, nullptr);
    }

    return a;
}
TypeBase *__parsePointerList(Lexer &lex)
{
    TypeBase *p = nullptr;

    if (lex.peakNext().type == OP_MUL)
    {
        lex.getNext();
        if (lex.peakNext().type == CONST)
        {
            lex.getNext();
            p = TypeFactory::Pointer(true);
        }
        else
            p = TypeFactory::Pointer(false);

        TypeBase *sub = __parsePointerList(lex);
        if (sub) TypeFactory::AppendRight(*p, *sub, nullptr);
    }

    return p;
}

void __parseDeclaration(Lexer &lex, Environment *env, bool is_global)
{
    Symbol *s = SymbolFactory::newInstance();
    s->name = StringRef("<anonymous-symbol>");
    s->space = SC_ID;

    TypeBase *spec = __parseSpecifier(lex, env);
    __parseDeclarator(*s, lex, env);
    TypeFactory::AppendRight(*s->type, *spec, env);

    // parse declaration and add to env
    if (lex.peakNext().type == BLK_BEGIN)  // Function definition
    {
        if (!is_global)
            SyntaxError("Function definition not allowed here.");

        assert(s->name != "<anonymous-symbol>");

        // check function return type & parameter type
        TypeFactory::FunctionCheck(s->type, env, s->name);

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
        if (spec == nullptr)
            SyntaxErrorDebug("Declaration: missing specifiers");

        while (true)
        {
            if (s->name == "<anonymous-symbol>")
            {
                SyntaxWarningEx("Missing identifier");
                break;
            }

            // create objects (function object is handled differently, see above)
            {
                // use outer layer env if inner layer env doesn't has definition
                Environment *obj_env = env;
                while (s->type->isIncomplete(obj_env, false) && obj_env->parent())
                {
                    obj_env = obj_env->parent();
                }
                TypeFactory::ConstructObject(s->obj, *(s->type), obj_env);
                env->add(s);
            }

            if (lex.peakNext().type == OP_COMMA)
            {
                lex.getNext();
                s = SymbolFactory::newInstance();
                s->name = StringRef("<anonymous-symbol>");
                s->space = SC_ID;
                __parseDeclarator(*s, lex, env);
                TypeFactory::AppendRight(*s->type, *spec, env);
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
