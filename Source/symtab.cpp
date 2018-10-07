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

Type *TypeUtil::Type_size_t()
{
    return new IntegerType("Ui");
}
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
const Type *TypeUtil::Unbox(const Type *tag)
{
    if (!tag || tag->getClass() != T_TAG)
        SyntaxError("unbox: need tag type.");
    const Type *impl = dynamic_cast<const TagType *>(tag)->getImpl();
    if (!impl)
        SyntaxError("unbox: null implement type.");
    return impl;
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
const Type *TypeUtil::TargetType(const Type *t)
{
    const Type *target = nullptr;
    switch (t->getClass())
    {
        case T_POINTER:
            target = dynamic_cast<const PointerType *>(t)->_t;
            break;
        case T_ARRAY: target = dynamic_cast<const ArrayType *>(t)->_t; break;
        case T_FUNCTION: target = dynamic_cast<const FuncType *>(t)->_t; break;
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
bool TypeUtil::Compatible(const Type *t1, const Type *t2, StringRef *reason)
{
    // TODO: support typedef.
    // TODO: support enum-const.
    assert(t1 != nullptr && t2 != nullptr);

    // type qualifier
    int qualifiers = TP_CONST | TP_VOLATILE;
    if ((t1->_prop & qualifiers) != (t2->_prop & qualifiers))
    {
        if (reason)
            *reason = "different qualified.";
        return false;
    }

    // type specifier
    if (t1->getClass() != t2->getClass())
    {
        // XXX: allowed combination:
        //  tag(enum) + enum-const
        //  tag(enum) + integral
        //  enum-const + integral
        t1 = (t1->getClass() == T_TAG) ? Unbox(t1) : t1;
        t2 = (t2->getClass() == T_TAG) ? Unbox(t2) : t2;
        bool t1_enum =
            (t1->getClass() == T_ENUM_CONST || t1->getClass() == T_ENUM);
        bool t1_int = (t1->isIntegral() && !t1_enum);
        bool t2_enum =
            (t2->getClass() == T_ENUM_CONST || t2->getClass() == T_ENUM);
        bool t2_int = (t2->isIntegral() && !t2_enum);
        if (t1_enum && t2_enum)
        {
            // NEED: enum-const has link to origin enum-impl
            const EnumTypeImpl *e =
                (t1->getClass() == T_ENUM)
                    ? dynamic_cast<const EnumTypeImpl *>(t1)
                    : dynamic_cast<const EnumTypeImpl *>(t2);
            const EnumConstType *ec =
                (t1->getClass() == T_ENUM_CONST)
                    ? dynamic_cast<const EnumConstType *>(t1)
                    : dynamic_cast<const EnumConstType *>(t2);
            const EnumConstType *efind = nullptr;
            for (auto em : e->_members)
            {
                if (em->_name == ec->_name)
                {
                    efind = em;
                    break;
                }
            }
            if (!efind)
            {
                if (reason)
                    *reason = "different enum type.";
                return false;
            }
            assert(efind->_value == ec->_value);
            return true;
        }
        else if ((t1_enum && t2_int) || (t1_int && t2_enum))
        {
            // goto test_size_and_align
        }
        else
        {
            if (reason)
                *reason = "different type class.";
            return false;
        }
    }
    else
    {
        bool unboxed = false;
        if (t1->getClass() == T_TAG)
        {
            const TagType *tg1 = dynamic_cast<const TagType *>(t1);
            const TagType *tg2 = dynamic_cast<const TagType *>(t2);
            if (tg1->_name != tg2->_name || tg1->_impl_type != tg2->_impl_type)
            {
                if (reason)
                    *reason = "tag: different tag name or tag type.";
                return false;
            }
            t1 = tg1->getImpl();
            t2 = tg2->getImpl();
            unboxed = true;
        }

        if (t1->getClass() == T_STRUCT)  // struct
        {
            assert(unboxed);
            // number, names, types
            const StructTypeImpl *s1 = dynamic_cast<const StructTypeImpl *>(t1);
            const StructTypeImpl *s2 = dynamic_cast<const StructTypeImpl *>(t2);
            if (s1->_member_name.size() != s2->_member_name.size())
            {
                if (reason)
                    *reason = "struct: different member number.";
                return false;
            }
            for (size_t i = 0; i < s1->_member_name.size(); ++i)
            {
                if (s1->_member_name[i] != s2->_member_name[i] ||
                    !Compatible(s1->_member_type[i], s2->_member_type[i]))
                {
                    if (reason)
                        *reason = "struct: different member name or type.";
                    return false;
                }
            }
            // goto test_size_and_align
        }
        else if (t1->getClass() == T_UNION)  // union
        {
            assert(unboxed);
            // number, names, types
            const StructTypeImpl *s1 = dynamic_cast<const StructTypeImpl *>(t1);
            const StructTypeImpl *s2 = dynamic_cast<const StructTypeImpl *>(t2);
            if (s1->_member_name.size() != s2->_member_name.size())
            {
                if (reason)
                    *reason = "union: different member number.";
                return false;
            }
            for (size_t i = 0; i < s1->_member_name.size(); ++i)
            {
                bool matched = false;
                for (size_t j = 0; j < s2->_member_name.size(); ++j)
                {
                    if (s1->_member_name[i] == s2->_member_name[j])
                    {
                        matched = true;
                        if (!Compatible(s1->_member_type[i],
                                        s2->_member_type[j]))
                        {
                            if (reason)
                                *reason = "union: different member type.";
                            return false;
                        }
                        else
                            break;
                    }
                }
                if (!matched)
                    return false;
            }
            // goto test_size_and_align
        }
        else if (t1->getClass() == T_ENUM)  // enum
        {
            assert(unboxed);
            // number, names, values
            const EnumTypeImpl *e1 = dynamic_cast<const EnumTypeImpl *>(t1);
            const EnumTypeImpl *e2 = dynamic_cast<const EnumTypeImpl *>(t2);
            if (e1->_members.size() != e2->_members.size())
            {
                if (reason)
                    *reason = "enum: different member number.";
                return false;
            }
            for (auto const &em1 : e1->_members)
            {
                bool matched = false;
                for (auto const &em2 : e2->_members)
                {
                    if (em1->_name == em2->_name && em1->_value == em2->_value)
                    {
                        matched = true;
                        break;
                    }
                }
                if (!matched)
                {
                    if (reason)
                        *reason = "enum: unmatched enum constant.";
                    return false;
                }
            }
            // goto test_size_and_align
        }
        else if (t1->getClass() == T_ENUM_CONST)
        {
            const EnumConstType *ec1 = dynamic_cast<const EnumConstType *>(t1);
            const EnumConstType *ec2 = dynamic_cast<const EnumConstType *>(t2);
            if (ec1->_name != ec2->_name || ec1->_value != ec2->_value)
            {
                if (reason)
                    *reason = "enum-const: unmatched.";
                return false;
            }
            // goto test_size_and_align
        }

        // declarator
        if (t1->getClass() == T_POINTER)  // pointer
        {
            if (!Compatible(TargetType(t1), TargetType(t2)))
            {
                if (reason)
                    *reason = "pointer: incompatible target type.";
                return false;
            }
            else
                return true;
        }
        else if (t1->getClass() == T_ARRAY)  // array
        {
            if (!t1->isIncomplete() && !t2->isIncomplete() &&
                dynamic_cast<const ArrayType *>(t1)->_n !=
                    dynamic_cast<const ArrayType *>(t2)->_n)
            {
                if (reason)
                    *reason = "array: unmatched size.";
                return false;
            }
            else
            {
                if (!Compatible(TargetType(t1), TargetType(t2)))
                {
                    if (reason)
                        *reason = "array: incompatible target type.";
                    return false;
                }
                else
                    return true;
            }
        }
        else if (t1->getClass() == T_FUNCTION)  // function
        {
            SyntaxError("not implemented.");
        }

        // char/integer/floating/enum-const
        if (t1->isIntegral() && t2->isIntegral())
        {
            const IntegralType *i1 = dynamic_cast<const IntegralType *>(t1);
            const IntegralType *i2 = dynamic_cast<const IntegralType *>(t2);
            if (i1->isSigned() != i2->isSigned())
            {
                if (reason)
                    *reason = "integral: unmatched signedness.";
                return false;
            }
            // goto test_size_and_align
        }
        // void/label
    }

    if (t1->getSize() != t2->getSize() ||
        t1->getAlignment() != t2->getAlignment())
    {
        if (reason)
            *reason = "unmatched size or alignment.";
        return false;
    }
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
        SyntaxError(
            "ByAssignmentConversion: Unsupported type conversion."
            " from \"" +
            from->toString() + "\" to \"" + to->toString() + "\"");
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

// char, short int, bit-field, enum
//    => int if large enough
//    => unsigned int else
Type *TypeConversion::IntegerPromotion(Type *t)
{
    if (t == nullptr || !t->isIntegral())
        SyntaxError("IntegerPromotion: expect integral type.");
    return new IntegerType("i");
}
// Type *TypeConversion::BooleanConversion(Type *t);
Type *TypeConversion::IntegerConversion(Type *from, Type *to)
{
    assert(from != nullptr && to != nullptr);
    assert(from->getClass() == T_INT && to->getClass() == T_INT);

    size_t size1 = from->getSize();
    size_t size2 = to->getSize();
    // bool signed1 = dynamic_cast<IntegerType *>(from)->isSigned();
    // bool signed2 = dynamic_cast<IntegerType *>(to)->isSigned();

    if (size1 == size2)
    {
    }
    else if (size1 < size2)
    {
    }
    else
    {
    }

    return to;
}
// Type *TypeConversion::PointerConversion(Type *t);
// Type *TypeConversion::FloatingConversion(Type *t);
// Type *TypeConversion::FIIFConversion(Type *from, const Type *to);

// -------- symbol management -------

Symbol *SymbolTable::findSymbol(ESymbolNamespace space, StringRef name) const
{
    for (Symbol *s : symbols)
    {
        if (s->space == space && s->name == name)
            return s;
    }
    return nullptr;
}
Symbol *SymbolTable::findSymbolRecursive(ESymbolNamespace space,
                                         StringRef name) const
{
    const SymbolTable *e = this;
    Symbol *s = nullptr;
    while (s == nullptr)
    {
        s = e->findSymbol(space, name);
        if (!e->isRoot())
            e = e->parent();
        else
            break;
    }
    return s;
}
/*
Symbol *SymbolTable::recursiveFind(ESymbolNamespace space, StringRef name) const
{
    Symbol *s = find(space, name);
    if (s != nullptr)
        return s;
    else
        return parent() ? parent()->recursiveFind(space, name) : nullptr;
}
Symbol *SymbolTable::findDefinition(StringRef name) const
{
    return find(SYMBOL_NAMESPACE_tag, name);
}
Symbol *SymbolTable::recursiveFindDefinition(StringRef name) const
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
Symbol *SymbolTable::recursiveFindTypename(StringRef name) const
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
void SymbolTable::addSymbol(Symbol *s)
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
const Symbol *SymbolTable::SameNameSymbolInFileScope(const SymbolTable *env,
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
void SymbolTable::traverse(IRTranslator &t) const
{
    // all objects and function objects
    for (auto &o : storage->get())
    {
        t.onObject(o);
    }
    for (auto *child : getChildren())
    {
        child->traverse(t);
    }
}

// -------- code generation --------
int SymbolTable::idgen = 0;

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
        //         ((const SymbolTable *)env)->debugPrint(tabs.size());
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

void SymbolTable::debugPrint() const
{
    __debugPrint(DebugString());
}

std::string SymbolTable::DebugString() const
{
    const int buffer_size = 65536;
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
    {
        auto children = getChildren();
        std::for_each(
            children.begin(), children.end(),
            [&str_envs](SymbolTable *e) { str_envs += e->DebugString(); });
    }
    std::string str_storage;
    {
        if (parent() && storage == parent()->getStorage())
            str_storage = "... see parent ...";
        else
            str_storage = storage->toString().data();
    }
    snprintf(buffer, buffer_size, fmt, id, str_symbols.data(),
             str_storage.data(), str_envs.data());
    return std::string(buffer);
}

// void SymbolTableGenerator::traverse(Ast *ast)
// {
//     ast->accept(this, STEP_begin);

//     int i = 0;
//     assert(ast->getChildrenCount() <= 9);
//     for (auto child : ast->getChildren())
//     {
//         traverse(child);
//         ast->accept(this, STEP_1 + (i++));
//     }

//     ast->accept(this, STEP_end);
// }

// void SymbolTableGenerator::visit(sn_translation_unit *node, EVisitStep step) {}
// void SymbolTableGenerator::visit(sn_external_declaration *node, EVisitStep step) { }
// void SymbolTableGenerator::visit(sn_function_definition *node, EVisitStep step) { }
// void SymbolTableGenerator::visit(sn_declaration *node, EVisitStep step) {}
// void SymbolTableGenerator::visit(sn_declaration_list *node, EVisitStep step) {}
// void SymbolTableGenerator::visit(sn_init_declarator *node, EVisitStep step) {}
// void SymbolTableGenerator::visit(sn_init_declarator_list *node, EVisitStep step) { }
// void SymbolTableGenerator::visit(sn_declarator *node, EVisitStep step) {}
// void SymbolTableGenerator::visit(sn_direct_declarator *node, EVisitStep step) {}
// void SymbolTableGenerator::visit(sn_abstract_declarator *node, EVisitStep step) { }
// void SymbolTableGenerator::visit(sn_direct_abstract_declarator *node,
//                                  EVisitStep step) { }
// void SymbolTableGenerator::visit(sn_initializer *node, EVisitStep step) {}
// void SymbolTableGenerator::visit(sn_initializer_list *node, EVisitStep step) {}
// void SymbolTableGenerator::visit(sn_parameter_type_list *node, EVisitStep step) { }
// void SymbolTableGenerator::visit(sn_parameter_list *node, EVisitStep step) {}
// void SymbolTableGenerator::visit(sn_parameter_declaration *node,
//                                  EVisitStep step) { }
// void SymbolTableGenerator::visit(sn_declaration_specifiers *node,
//                                  EVisitStep step)
// {
//     if (step == STEP_begin)
//     {
//         // start type_specifier
//         // start type_qualifier
//         // start type_storage
//     }
//     else if (step == STEP_end)
//     {
//         // type_builder.feed_decl_spec(...)
//         // stop type_specifier
//         // stop type_qualifier
//         // stop type_storage
//     }
// }
// // void SymbolTableGenerator::visit(sn_specifier_qualifier_list *node,
// //                                  EVisitStep step) { }
// void SymbolTableGenerator::visit(sn_storage_specifier *node, EVisitStep step) {}
// void SymbolTableGenerator::visit(sn_type_qualifier *node, EVisitStep step) { }
// void SymbolTableGenerator::visit(sn_type_qualifier_list *node, EVisitStep step) { }
// void SymbolTableGenerator::visit(sn_type_specifier *node, EVisitStep step)
// {
//     if (step == STEP_begin)
//     {
//         start(node->nodeType()); // push type info
//         switch(node->t)
//         {
//             case TYPE_STRUCT:
//             case TYPE_UNION:
//             case TYPE_ENUM:
//             case SYMBOL: break;
//             default: feed(node->t); break;
//         }
//     }
//     else if (step == STEP_end)
//     {
//         switch (current())
//         {
//             case BD_struct_union_specifier:
//             case BD_enum_specifier:
//                 // add name info to SymbolTable
//                 stop(current()); // keep type info, pop name info
//                 pop_assign(); // pop keeped type info, assign it to new top
//                 break;
//             case BD_typedef:
//                 // ???
//                 break;
//             default: break;
//         }
//     }
// }
// void SymbolTableGenerator::visit(sn_struct_union_specifier *node,
//                                  EVisitStep step)
// {
//     if (step == STEP_begin)
//     {
//         start(node->nodeType()); // push type info, name info
//         feed(node->t);
//     }
//     else if (step == STEP_end)
//     {
//         if (current() == SN_STRUCT_DECLARATION_LIST)

//     }
// }
// void SymbolTableGenerator::visit(sn_enum_specifier *node, EVisitStep step) {}
// void SymbolTableGenerator::visit(sn_struct_declaration *node, EVisitStep step)
// {
// }
// void SymbolTableGenerator::visit(sn_struct_declaration_list *node,
//                                  EVisitStep step)
// {
// }
// void SymbolTableGenerator::visit(sn_struct_declarator *node, EVisitStep step) {}
// void SymbolTableGenerator::visit(sn_struct_declarator_list *node,
//                                  EVisitStep step)
// {
// }
// void SymbolTableGenerator::visit(sn_enumerator_list *node, EVisitStep step) {}
// void SymbolTableGenerator::visit(sn_enumerator *node, EVisitStep step) {}
// void SymbolTableGenerator::visit(sn_enumeration_constant *node, EVisitStep step)
// {
// }
// void SymbolTableGenerator::visit(sn_type_name *node, EVisitStep step) {}
// void SymbolTableGenerator::visit(sn_pointer *node, EVisitStep step) {}
// void SymbolTableGenerator::visit(sn_identifier *node, EVisitStep step) {}
// void SymbolTableGenerator::visit(sn_identifier_list *node, EVisitStep step) {}
// void SymbolTableGenerator::visit(sn_typedef_name *node, EVisitStep step) {}
// void SymbolTableGenerator::visit(sn_statement *node, EVisitStep step) {}
// void SymbolTableGenerator::visit(sn_statement_list *node, EVisitStep step) {}
// void SymbolTableGenerator::visit(sn_label_statement *node, EVisitStep step) {}
// void SymbolTableGenerator::visit(sn_compound_statement *node, EVisitStep step)
// {
// }
// void SymbolTableGenerator::visit(sn_expression_statement *node, EVisitStep step)
// {
// }
// void SymbolTableGenerator::visit(sn_selection_statement *node, EVisitStep step)
// {
// }
// void SymbolTableGenerator::visit(sn_iteration_statement *node, EVisitStep step)
// {
// }
// void SymbolTableGenerator::visit(sn_jump_statement *node, EVisitStep step) {}
// void SymbolTableGenerator::visit(sn_expression *node, EVisitStep step) {}
// void SymbolTableGenerator::visit(sn_comma_expression *node, EVisitStep step) {}
// void SymbolTableGenerator::visit(sn_assign_expression *node, EVisitStep step) {}
// void SymbolTableGenerator::visit(sn_cond_expression *node, EVisitStep step) {}
// void SymbolTableGenerator::visit(sn_or_expression *node, EVisitStep step) {}
// void SymbolTableGenerator::visit(sn_and_expression *node, EVisitStep step) {}
// void SymbolTableGenerator::visit(sn_bitor_expression *node, EVisitStep step) {}
// void SymbolTableGenerator::visit(sn_bitxor_expression *node, EVisitStep step) {}
// void SymbolTableGenerator::visit(sn_bitand_expression *node, EVisitStep step) {}
// void SymbolTableGenerator::visit(sn_eq_expression *node, EVisitStep step) {}
// void SymbolTableGenerator::visit(sn_rel_expression *node, EVisitStep step) {}
// void SymbolTableGenerator::visit(sn_shift_expression *node, EVisitStep step) {}
// void SymbolTableGenerator::visit(sn_add_expression *node, EVisitStep step) {}
// void SymbolTableGenerator::visit(sn_mul_expression *node, EVisitStep step) {}
// void SymbolTableGenerator::visit(sn_cast_expression *node, EVisitStep step) {}
// void SymbolTableGenerator::visit(sn_unary_expression *node, EVisitStep step) {}
// void SymbolTableGenerator::visit(sn_postfix_expression *node, EVisitStep step)
// {
// }
// void SymbolTableGenerator::visit(sn_const_expression *node, EVisitStep step) {}
// void SymbolTableGenerator::visit(sn_primary_expression *node, EVisitStep step)
// {
// }
// void SymbolTableGenerator::visit(sn_argument_expression_list *node,
//                                  EVisitStep step)
// {
// }
// void SymbolTableGenerator::visit(sn_const_expression *node, EVisitStep step) {}
