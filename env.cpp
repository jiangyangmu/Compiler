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
            case T_POINTER: tref = &dynamic_cast<PointerType *>(p)->_t; break;
            case T_ARRAY: tref = &dynamic_cast<ArrayType *>(p)->_t; break;
            case T_FUNCTION: tref = &dynamic_cast<FuncType *>(p)->_t; break;
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

IRObjectHandle IRStorage::put(IRObject object)
{
    // set address mode & value
    assert(object.addr.mode != OP_ADDR_invalid);
    if (object.addr.mode == OP_ADDR_mem)
    {
        _unnamed_alloc += _alloc;
        object.addr.value =
            __aligned_alloc(_unnamed_alloc, object.size, object.align);
        _unnamed_alloc -= _alloc;

        _unnamed_alloc_max = (_unnamed_alloc_max < _unnamed_alloc)
                                 ? _unnamed_alloc
                                 : _unnamed_alloc_max;
    }

    _unnamed_obj.push_back(object);
    return (_unnamed_obj.size() - 1) + _obj.size();
}
IRObjectHandle IRStorage::putWithName(IRObject object, StringRef label,
                                      bool merge)
{
    assert(_unnamed_alloc == 0 && _unnamed_alloc_max == 0);

    IRObjectHandle h = 0;
    IRObject *pobj = nullptr;
    if (merge)
    {
        SyntaxError("IRStorage: not implemented.");
        // auto it = std::find(_name.begin(), _name.end(), label);
        // if (it == _name.end())
        //     SyntaxError("IRStorage: can't find symbol: " + label.toString());
        // *it = object;

        // pobj = &(*it);
    }
    else
    {
        _name.push_back(label);
        _obj.push_back(object);

        pobj = &(_obj.back());
        h = _obj.size() - 1;
    }
    assert(pobj != nullptr);

    assert(pobj->addr.mode != OP_ADDR_invalid);
    switch (pobj->addr.mode)
    {
        case OP_ADDR_mem:
            pobj->addr.value = __aligned_alloc(_alloc, pobj->size, pobj->align);
            break;
        default:  // label, imm
            assert(pobj->addr.value == 0);
            break;
    }

    return h;
}
IRAddress IRStorage::find(IRObjectHandle h)
{
    assert(h < (_obj.size() + _unnamed_obj.size()));
    if (h < _obj.size())
        return _obj[h].addr;
    else
        return _unnamed_obj[h - _obj.size()].addr;
}
IRAddress IRStorage::findByName(StringRef label)
{
    auto it = std::find(_name.begin(), _name.end(), label);
    if (it == _name.end())
        SyntaxError("IRStorage: can't find symbol: " + label.toString());
    return _obj[it - _name.begin()].addr;
}
std::vector<IROperation> IRStorage::generateCode() const
{
    std::vector<IROperation> code;
    if (_alloc + _unnamed_alloc_max > 0)
    {
        IROperation alloc = {
            OP_TYPE_alloc, {OP_ADDR_imm, _alloc + _unnamed_alloc_max}, {}, {}};
        code.push_back(alloc);
    }
    return code;
}

std::string IRStorage::toString() const
{
    std::string s;
    for (size_t i = 0; i < _name.size(); ++i)
    {
        s += _name[i].toString();
        s += ':';
        s += _obj[i].toString();
        s += '\n';
    }
    return s;
}

std::string IRObject::toString() const
{
    std::string s;
    switch (addr.mode)
    {
        case OP_ADDR_invalid: s += "*:"; break;
        case OP_ADDR_mem:
            s += std::to_string(addr.value);
            s += ':';
            break;
        case OP_ADDR_imm: s += "imm:"; break;
        case OP_ADDR_label: s += "label:"; break;
        default: break;
    }
    switch (type)
    {
        case IR_TYPE_invalid: s += "*:"; break;
        case IR_TYPE_int: s += "int:"; break;
        case IR_TYPE_uint: s += "uint:"; break;
        case IR_TYPE_float: s += "float:"; break;
        case IR_TYPE_array: s += "array:"; break;
        case IR_TYPE_string: s += "string:"; break;
        default: break;
    }
    s += std::to_string(size);
    s += ':';
    s += std::to_string(align);
    s += ':';
    if (type == IR_TYPE_array)
    {
        s += std::to_string(element_size);
        s += ':';
    }
    s += "(value)";
    return s;
}

std::string IROperation::toString() const
{
    std::string s = IRUtil::OperationTypeToString(op);
    s += ' ';
    if (arg1.mode != OP_ADDR_invalid)
    {
        s += (arg1.mode == OP_ADDR_imm) ? "#" : "";
        s += std::to_string(arg1.value);
    }
    else
        s += '-';
    s += ' ';
    if (arg2.mode != OP_ADDR_invalid)
    {
        s += (arg2.mode == OP_ADDR_imm) ? "#" : "";
        s += std::to_string(arg2.value);
    }
    else
        s += '-';
    s += ' ';
    if (arg3.mode != OP_ADDR_invalid)
    {
        s += (arg3.mode == OP_ADDR_imm) ? "#" : "";
        s += std::to_string(arg3.value);
    }
    else
        s += '-';
    return s;
}

IRObject IRObjectBuilder::FromType(const Type *ctype)
{
    IRObject o;

    Type *type = const_cast<Type *>(ctype);
    assert(type != nullptr);
    type->getSize();  // let array complete itself. TODO: remove this later
    if (type->isIncomplete() || !(type->isObject() || type->isFunction()) ||
        (type->isObject() &&
         (type->getSize() == 0 || type->getAlignment() == 0)))
    {
        SyntaxError("IRUtil: invalid type.");
    }
    // type is object or function

    // addr
    o.addr = {type->isObject() ? OP_ADDR_mem : OP_ADDR_label, 0};
    // type
    o.element_size = 0;
    switch (type->getClass())
    {
        case T_CHAR:
        case T_ENUM_CONST: o.type = IR_TYPE_int; break;
        case T_POINTER: o.type = IR_TYPE_uint; break;
        case T_INT:
            o.type = dynamic_cast<const IntegerType *>(type)->isSigned()
                         ? IR_TYPE_int
                         : IR_TYPE_uint;
            break;
        case T_FLOAT: o.type = IR_TYPE_float; break;
        case T_ARRAY:
            o.type = IR_TYPE_array;
            o.element_size = TypeUtil::TargetType(type)->getSize();
            break;
        case T_FUNCTION:
            o.type = IR_TYPE_routine;
            break;
        case T_TAG:
        default: SyntaxError("not supported or not implemented."); break;
    }
    // size
    o.size = type->getSize();
    // align
    o.align = type->getAlignment();
    // value: TODO: value management
    o.value = nullptr;

    return o;
}
IRObject IRObjectBuilder::FromTokenWithType(const Token *token,
                                            const Type *type)
{
    IRObject o = FromType(type);

    // override address for constant
    switch (token->type)
    {
        case CONST_CHAR: o.addr = {OP_ADDR_imm, (uint64_t)token->cval}; break;
        case CONST_INT:
            // TODO: get correct width value
            o.addr = {OP_ADDR_imm, (uint64_t)token->ival};
            break;
        case CONST_FLOAT:
        case STRING: o.addr = {OP_ADDR_label, 0}; break;
        default: SyntaxError("IRObjectBuilder: unexpected token type."); break;
    }

    return o;
}
IRObject IRObjectBuilder::FromSymbol(const Symbol *symbol)
{
    IRObject o = FromType(symbol->type);

    if (symbol->linkage == SYMBOL_LINKAGE_internal)
        o.addr = {OP_ADDR_label, 0};

    return o;
}

std::string IRUtil::OperationTypeToString(EOperationType type)
{
    std::string s;
    switch (type)
    {
        case OP_TYPE_alloc: s = "alloc"; break;
        case OP_TYPE_free: s = "free"; break;
        case OP_TYPE_cmp: s = "cmp"; break;
        case OP_TYPE_jmp: s = "jmp"; break;
        case OP_TYPE_je: s = "je"; break;
        case OP_TYPE_jl: s = "jl"; break;
        case OP_TYPE_jle: s = "jle"; break;
        case OP_TYPE_jg: s = "jg"; break;
        case OP_TYPE_jge: s = "jge"; break;
        case OP_TYPE_jb: s = "jb"; break;
        case OP_TYPE_jbe: s = "jbe"; break;
        case OP_TYPE_ja: s = "ja"; break;
        case OP_TYPE_jae: s = "jae"; break;
        case OP_TYPE_mov: s = "mov"; break;
        case OP_TYPE_or: s = "or"; break;
        case OP_TYPE_xor: s = "xor"; break;
        case OP_TYPE_and: s = "and"; break;
        case OP_TYPE_not: s = "not"; break;
        case OP_TYPE_shl: s = "shl"; break;
        case OP_TYPE_shr: s = "shr"; break;
        case OP_TYPE_add: s = "add"; break;
        case OP_TYPE_sub: s = "sub"; break;
        case OP_TYPE_mul: s = "mul"; break;
        case OP_TYPE_div: s = "div"; break;
        case OP_TYPE_mod: s = "mod"; break;
        case OP_TYPE_inc: s = "inc"; break;
        case OP_TYPE_dec: s = "dec"; break;
        case OP_TYPE_neg: s = "neg"; break;
        case OP_TYPE_ref: s = "ref"; break;
        case OP_TYPE_deref: s = "deref"; break;
        case OP_TYPE_param: s = "param"; break;
        case OP_TYPE_call: s = "call"; break;
        case OP_TYPE_ret: s = "ret"; break;
        case OP_TYPE_fld: s = "fld"; break;
        case OP_TYPE_fst: s = "fst"; break;
        case OP_TYPE_fabs: s = "fabs"; break;
        case OP_TYPE_fchs: s = "fchs"; break;
        case OP_TYPE_fadd: s = "fadd"; break;
        case OP_TYPE_fsub: s = "fsub"; break;
        case OP_TYPE_fmul: s = "fmul"; break;
        case OP_TYPE_fdiv: s = "fdiv"; break;
        default: break;
    }
    return s;
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
    storage.putWithName(IRObjectBuilder::FromType(s->type), s->name,
                        need_merge);

    DebugLog("add symbol: " + s->name.toString());
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
    __debugPrint(storage.toString());
    for (Symbol *s : symbols)
    {
        __debugPrint(s->toString());
    }
    for (auto child : getChildren())
    {
        child->debugPrint(0);
    }
}

// -------- declaration parsing --------
