#define HAS_LEXER

#include "env.h"

#include "parser.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>
#include <vector>
using namespace std;

// -------- type management -------

Type *TypeUtil::Concatenate(Type *front, Type *back)
{
    if (front == nullptr)
        return back;
    else if (back == nullptr)
        return front;

    DerivedType *dt = dynamic_cast<DerivedType *>(front);
    if (dt == nullptr)
        SyntaxError("TypeUtil::Concatenate: can't concatenate type.");

    // TODO: should use a stack during build, not here
    if (dt->getTargetType() == nullptr)
        dt->setTargetType(back);
    else
        dt->setTargetType(Concatenate(dt->getTargetType(), back));
    return front;
}
Type *TypeUtil::TargetType(Type *t)
{
    Type *target = nullptr;
    switch (t->getClass())
    {
        case T_POINTER: target = dynamic_cast<PointerType *>(t)->_t; break;
        case T_ARRAY: target = dynamic_cast<ArrayType *>(t)->_t; break;
        case T_FUNCTION: target = dynamic_cast<FuncType *>(t)->_t; break;
        default:
            SyntaxError("TypeUtil::TargetType: not an aggregate type.");
            break;
    }
    return target;
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
Type *TypeUtil::CloneTop(const Type *t)
{
    Type *result = nullptr;
    switch (t->getClass())
    {
        case T_VOID: result = new VoidType(*(const VoidType *)t); break;
        case T_CHAR: result = new CharType(*(const CharType *)t); break;
        case T_INT: result = new IntegerType(*(const IntegerType *)t); break;
        case T_FLOAT:
            result = new FloatingType(*(const FloatingType *)t);
            break;
        case T_POINTER:
            result = new PointerType(*(const PointerType *)t);
            break;
        case T_TAG: result = new TagType(*(const TagType *)t); break;
        default: SyntaxError("can't clone this type."); break;
    }
    return result;
}
bool TypeUtil::Equal(const Type *t1, const Type *t2)
{
    assert(t1 && t2);
    return t1->equal(*t2);
}
bool TypeUtil::Compatible(const Type *t1, const Type *t2)
{
    // TODO: fully support this.
    assert(t1 != nullptr && t2 != nullptr);

    if (t1->getClass() != t2->getClass())
        return false;
    if (t1->_prop != t2->_prop)
        return false;
    if (t1->_size != t2->_size)
        return false;
    if (t1->_align != t2->_align)
        return false;
    return true;
}
bool TypeUtil::MoreStrictQualified(const Type *test, const Type *base)
{
    if (base->isConst() && !test->isConst())
        return false;
    if (base->isVolatile() && !test->isVolatile())
        return false;
    return true;
}
StringRef TypeUtil::GenerateTag()
{
    static int tag_id = 0;
    static vector<string> tags;
    tags.push_back(to_string(tag_id++));
    return StringRef(tags.back().data());
}

Type *TypeConversion::CondExprConversion(Type *if_true, Type *if_false)
{
    SyntaxWarning("CondExprConversion: not fully implemented.");
    return if_true;
}

Type *TypeConversion::ByAssignmentConversion(Type *to, Type *from)
{
    // remove top type's type qualifiers (const/volatile)
    // TODO

    // type conversion
    //   arithmetic: char <-> int <-> enum <-> float
    //   pointer:
    //       > two compatible types || one type one void || one type one NULL
    //       ((void *)0)
    //   aggregate: struct/union <-> struct/union
    //       > must be compatible
    assert(to != nullptr && from != nullptr);
    if (to->isArithmetic() && from->isArithmetic())
    {
        // warning: if narrowing
    }
    else if (to->getClass() == T_POINTER && from->getClass() == T_POINTER)
    {
        Type *to_type = TypeUtil::TargetType(to);
        Type *from_type = TypeUtil::TargetType(from);
        if (to_type->getClass() == T_VOID || from_type->getClass() == T_VOID)
        {
            if (!TypeUtil::MoreStrictQualified(to_type, from_type))
                SyntaxError(
                    "ByAssignmentConversion: need more strict qualified type.");
        }
        else
        {
            if (!TypeUtil::Compatible(to_type, from_type))
                SyntaxError(
                    "ByAssignmentConversion: pointers of incompatible type.");
        }
    }
    else if (to->getClass() == from->getClass() &&
             (to->getClass() == T_STRUCT || to->getClass() == T_UNION))
    {
        if (!TypeUtil::Compatible(to, from))
            SyntaxError("ByAssignmentConversion: incompatible types.");
    }
    else
    {
        SyntaxError("ByAssignmentConversion: Unsupported type conversion.");
    }

    return to;
}
// Type *TypeConversion::DefaultArgumentPromotion(Type *t);
Type *TypeConversion::UsualArithmeticConversion(Type *l, Type *r)
{
    if (!l->isArithmetic() || !r->isArithmetic())
    {
        SyntaxError("expect arithmetic type.");
    }

    // TODO: fully support the rules
    if (l->getClass() == T_FLOAT && r->getClass() == T_FLOAT)
        return (l->getSize() >= r->getSize()) ? l : r;
    else if (l->getClass() == T_FLOAT)
        return l;
    else if (r->getClass() == T_FLOAT)
        return r;
    else
    {
        Type *lp = IntegerPromotion(l);
        Type *rp = IntegerPromotion(r);
        if (lp->getSize() != rp->getSize())
            return (lp->getSize() >= rp->getSize()) ? lp : rp;
        else
            return lp;
    }
}
Type *TypeConversion::ValueTransformConversion(Type *t)
{
    if (!t->isObject() && !t->isFunction())
        SyntaxError("Need object type.");

    // Array to Pointer
    if (t->getClass() == T_ARRAY)
    {
        PointerType *pt = new PointerType();
        pt->setQualifier(TP_CONST);
        TypeUtil::Concatenate(pt, TypeUtil::TargetType(t));
        return pt;
    }
    // Function to Pointer
    else if (t->getClass() == T_FUNCTION)
    {
        PointerType *pt = new PointerType();
        pt->setQualifier(TP_CONST);
        TypeUtil::Concatenate(pt, t);
        return pt;
    }
    // Lvalue conversion
    else
    {
        Type *tc = TypeUtil::CloneTop(t);
        tc->unsetQualifier(TP_CONST | TP_VOLATILE);
        return tc;
    }
}

Type *TypeConversion::IntegerPromotion(Type *t)
{
    // TODO: also consider unsigned int
    return new IntegerType("i");
}
// Type *TypeConversion::BooleanConversion(Type *t);
// Type *TypeConversion::IntegerConversion(Type *t);
// Type *TypeConversion::PointerConversion(Type *t);
// Type *TypeConversion::FloatingConversion(Type *t);
// Type *TypeConversion::FIIFConversion(Type *from, const Type *to);

// -------- object management -------

std::vector<void *> IRValueFactory::_data;

std::string IRAddress::toString() const
{
    std::string s;
    if (mode == OP_ADDR_mem)
    {
        switch (width)
        {
            case 1: s += "BYTE "; break;
            case 2: s += "WORD "; break;
            case 4: s += "DWORD "; break;
            case 8: s += "QWORD "; break;
            default: SyntaxError("IRAddress: invalid width."); break;
        }
    }
    switch (mode)
    {
        case OP_ADDR_invalid: s += '-'; break;
        case OP_ADDR_mem:
            s += '[';
            s += std::to_string(mem);
            s += ']';
            break;
        case OP_ADDR_imm:
            s += '#';
            s += std::to_string(imm);
            break;
        case OP_ADDR_label:
            assert(label->size() < 1000000);
            s.append(label->data(), label->size());
            break;
    }
    return s;
}
std::string IRInstruction::toString() const
{
    std::string s = IRUtil::OpcodeToString(op);
    s += ' ';
    s += arg1.toString();
    s += ',';
    s += ' ';
    s += arg2.toString();
    s += ',';
    s += ' ';
    s += arg3.toString();
    return s;
}
std::string IRObject::toString() const
{
    std::string s;
    s += name.toString();
    s += ':';
    s += IRUtil::LinkageToString(linkage);
    s += ':';
    s += addr.toString();
    s += ':';
    s += "(value)";
    return s;
}
std::string IRStorage::toString() const
{
    std::string s;
    for (auto &o : _objects)
    {
        s += o.toString();
        s += '\n';
    }
    return s;
}
std::string IRCode::toString() const
{
    std::string s;
    for (auto &op : _code)
    {
        s += op.toString();
        s += '\n';
    }
    return s;
}

IRObject IRObjectBuilder::build() const
{
    IRObject o;
    // name
    o.name = _name;

    // linkage
    // _name == "" && !lex_const    => temporary
    //             && lex_const     => constant
    // _name != ""                  => symbol
    if (_name.empty())
    {
        switch (_lex_type)
        {
            case CONST_CHAR:
            case CONST_INT:
                o.linkage = IR_LINKAGE_inline;  // imm
                break;
            case CONST_FLOAT:
            case STRING:
                o.linkage = IR_LINKAGE_static_const;
                o.name = IRUtil::GenerateLabel();
                break;
            default: o.linkage = IR_LINKAGE_local; break;
        }
    }
    else
    {
        switch (_symbol_linkage)
        {
            case SYMBOL_LINKAGE_unique:
                assert(_syntax_storage == NONE || _syntax_storage == AUTO ||
                       _syntax_storage == REGISTER);
                o.linkage = IR_LINKAGE_local;
                break;
            case SYMBOL_LINKAGE_internal:
                assert(_syntax_storage == STATIC);
                o.linkage = IR_LINKAGE_static;
                break;
            case SYMBOL_LINKAGE_external:
                assert(_syntax_storage == NONE || _syntax_storage == EXTERN);
                if (_syntax_storage == NONE)
                    o.linkage = IR_LINKAGE_static_export;
                else
                    o.linkage = IR_LINKAGE_extern;
                break;
            default: assert(false); break;
        }
    }

    // value
    o.value = _value;
    // addr
    // TODO: addr.width = ?
    o.addr = {OP_ADDR_invalid, 0, {0}};

    return o;
}
void IRStorage::add(IRObject o)
{
    _objects.push_back(o);
    __compute_address(_objects.back());
}
IRAddress IRStorage::addAndGetAddress(IRObject o)
{
    _objects.push_back(o);
    __compute_address(_objects.back());
    return _objects.back().addr;
}
IRAddress IRStorage::getAddressByName(StringRef name) const
{
    IRAddress addr;
    for (auto const &o : _objects)
    {
        if (o.name == name)
        {
            addr = o.addr;
            break;
        }
    }
    assert(addr.mode != OP_ADDR_invalid);
    return addr;
}
void IRStorage::collectTemporarySpace()  // call after expression
{
    _tmp_alloc = 0;
}
std::list<IRInstruction> IRStorage::generateCode() const
{
    std::list<IRInstruction> code;
    if (_alloc + _tmp_alloc_max > 0)
    {
        IRInstruction alloc = {
            IR_OPCODE_alloc,
            {OP_ADDR_imm, sizeof(void *), {_alloc + _tmp_alloc_max}},
            {},
            {},
            new StringRef("")};
        code.push_back(alloc);
    }
    return code;
}

std::string IRUtil::OpcodeToString(EIROpcode op)
{
    std::string s;
    switch (op)
    {
        case IR_OPCODE_alloc: s = "alloc"; break;
        case IR_OPCODE_free: s = "free"; break;
        case IR_OPCODE_cmp: s = "cmp"; break;
        case IR_OPCODE_jmp: s = "jmp"; break;
        case IR_OPCODE_je: s = "je"; break;
        case IR_OPCODE_jl: s = "jl"; break;
        case IR_OPCODE_jle: s = "jle"; break;
        case IR_OPCODE_jg: s = "jg"; break;
        case IR_OPCODE_jge: s = "jge"; break;
        case IR_OPCODE_jb: s = "jb"; break;
        case IR_OPCODE_jbe: s = "jbe"; break;
        case IR_OPCODE_ja: s = "ja"; break;
        case IR_OPCODE_jae: s = "jae"; break;
        case IR_OPCODE_mov: s = "mov"; break;
        case IR_OPCODE_or: s = "or"; break;
        case IR_OPCODE_xor: s = "xor"; break;
        case IR_OPCODE_and: s = "and"; break;
        case IR_OPCODE_not: s = "not"; break;
        case IR_OPCODE_shl: s = "shl"; break;
        case IR_OPCODE_shr: s = "shr"; break;
        case IR_OPCODE_add: s = "add"; break;
        case IR_OPCODE_sub: s = "sub"; break;
        case IR_OPCODE_mul: s = "mul"; break;
        case IR_OPCODE_div: s = "div"; break;
        case IR_OPCODE_mod: s = "mod"; break;
        case IR_OPCODE_inc: s = "inc"; break;
        case IR_OPCODE_dec: s = "dec"; break;
        case IR_OPCODE_neg: s = "neg"; break;
        case IR_OPCODE_ref: s = "ref"; break;
        case IR_OPCODE_deref: s = "deref"; break;
        case IR_OPCODE_param: s = "param"; break;
        case IR_OPCODE_call: s = "call"; break;
        case IR_OPCODE_ret: s = "ret"; break;
        case IR_OPCODE_fld: s = "fld"; break;
        case IR_OPCODE_fst: s = "fst"; break;
        case IR_OPCODE_fabs: s = "fabs"; break;
        case IR_OPCODE_fchs: s = "fchs"; break;
        case IR_OPCODE_fadd: s = "fadd"; break;
        case IR_OPCODE_fsub: s = "fsub"; break;
        case IR_OPCODE_fmul: s = "fmul"; break;
        case IR_OPCODE_fdiv: s = "fdiv"; break;
        default: s = "???"; break;
    }
    return s;
}
std::string IRUtil::LinkageToString(EIRLinkage linkage)
{
    std::string s;
    switch (linkage)
    {
        case IR_LINKAGE_invalid: s += "invalid"; break;
        case IR_LINKAGE_inline: s += "inline"; break;
        case IR_LINKAGE_local: s += "local"; break;
        case IR_LINKAGE_static: s += "static"; break;
        case IR_LINKAGE_static_const: s += "static_const"; break;
        case IR_LINKAGE_static_export: s += "static_export"; break;
        case IR_LINKAGE_extern: s += "extern"; break;
        default: s += "???"; break;
    }
    return s;
}
StringRef IRUtil::GenerateLabel()
{
    static int label_id = 0;
    static std::vector<std::string> labels;
    labels.push_back("L._" + std::to_string(label_id++));
    return StringRef(labels.back().data());
}

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
Symbol *Environment::findSymbolRecursive(ESymbolNamespace space, StringRef name) const
{
    const Environment *e = this;
    Symbol *s = nullptr;
    while (s == nullptr && !e->isRoot())
    {
        s = e->findSymbol(space, name);
        e = e->parent();
    }
    return s;
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
    assert(s != nullptr && s->type != nullptr);

    Symbol *existed_s = findSymbol(s->space, s->name);
    bool need_merge = (existed_s != nullptr);
    if (need_merge)
    {
        // merge type
        Type *merged_type = TypeUtil::Merge(existed_s->type, s->type);
        if (merged_type == nullptr)
            SyntaxError("Symbol '" + s->name.toString() + "' already defined.");
        existed_s->type = merged_type;

        s = existed_s;
    }
    else
    {
        symbols.push_back(s);
    }

    // DebugLog("add symbol: " + s->name.toString() + "\t" +
    // s->type->toString());
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

void Environment::debugPrint() const
{
    __debugPrint(DebugString());
}

std::string Environment::DebugString() const
{
    const int buffer_size = 4096;
    static char buffer[buffer_size];
    const char fmt[] =
        "environment %d = {>\n"
        "symbols = {>\n%s<\n}\n"
        "storage = {>\n%s<\n}\n"
        "envs = {>\n%s<\n}<\n}\n";
    std::string str_symbols;
    std::for_each(symbols.begin(), symbols.end(), [&str_symbols](Symbol *s) {
        str_symbols += s->toString();
        str_symbols += '\n';
    });
    std::string str_envs;
    auto children = getChildren();
    std::for_each(
        children.begin(), children.end(),
        [&str_envs](Environment *e) { str_envs += e->DebugString(); });
    snprintf(buffer, buffer_size, fmt, id, str_symbols.data(),
             storage.toString().data(), str_envs.data());
    return std::string(buffer);
}
