#include <iostream>
#include <vector>
#include <deque>

#include "../parse2/parser2.h"
#include "../symbol/symbol.h"
#include "../symbol/env.h"
#include "../ir/expr.h"
#include "../ir/stmt.h"
#include "../logging/logging.h"

// always valid assertion
#define ASSERT(e)                                                          \
    (void)((!!(e)) ||                                                      \
           (_wassert(                                                      \
                _CRT_WIDE(#e), _CRT_WIDE(__FILE__), (unsigned)(__LINE__)), \
            0))

class TypeSpecifiersBuilder {
    // collect type-specifier set
    // bits:
    //      void,char,short,int,
    //      long,float,double,signed
    //      unsigned,struct/union,enum,typedef-name
    int type_specifier_allow_, type_specifier_has_;
    // for struct/union, enum, typedef-name
    Type * type_;
    std::vector<Type *> member_types_;
    std::vector<StringRef> member_names_;

public:
    TypeSpecifiersBuilder()
        : type_specifier_allow_(0xffffFFFF)
        , type_specifier_has_(0)
        , type_(nullptr) {
    }

    void feed_type_specifiers(Token::Type t) {
#define __check_set(value)                                             \
    if ((type_specifier_allow_ & (value)) == 0 ||                      \
        type_specifier_has_ & (value))                                 \
        SyntaxError("DeclarationSpecifiersBuilder: unexpected token"); \
    else                                                               \
        type_specifier_has_ |= (value);

        switch (t)
        {
            case Token::KW_VOID:
                __check_set(0x1);
                type_specifier_allow_ &= 0x1;
                break;
            case Token::KW_CHAR:
                __check_set(0x2);
                type_specifier_allow_ &= 0x182; // char, signed, unsigned
                break;
            case Token::KW_SHORT:
                __check_set(0x4);
                type_specifier_allow_ &= 0x18c; // short, signed, unsigned, int
                break;
            case Token::KW_INT:
                __check_set(0x8);
                type_specifier_allow_ &=
                    0x19c; // int, signed, unsigned, short, long
                break;
            case Token::KW_LONG:
                __check_set(0x10);
                type_specifier_allow_ &=
                    0x1d8; // long, signed, unsigned, int, double
                break;
            case Token::KW_FLOAT:
                __check_set(0x20);
                type_specifier_allow_ &= 0x20;
                break;
            case Token::KW_DOUBLE:
                __check_set(0x40);
                type_specifier_allow_ &= 0x50; // double, long
                break;
            case Token::KW_SIGNED:
                __check_set(0x80);
                type_specifier_allow_ &= 0x9e; // signed, char, short, int, long
                break;
            case Token::KW_UNSIGNED:
                __check_set(0x100);
                type_specifier_allow_ &=
                    0x11e; // unsigned, char, short, int, long
                break;
            case Token::KW_STRUCT:
                __check_set(0x200);
                type_specifier_allow_ &= 0x200;
                type_ = new StructType();
                break;
                // case Token::KW_UNION:
                //    __check_set(0x400);
                //    type_specifier_allow_ &= 0x400;
                //    // TODO: type_ = new UnionType();
                //    break;
                // case Token::KW_ENUM:
                //    __check_set(0x800);
                //    type_specifier_allow_ &= 0x800;
                //    // TODO: type_ = new EnumType();
                //    break;
            default:
                SyntaxError("DeclarationSpecifiersBuilder: unexpected token");
                break;
        }

#undef __check_set

        assert((~type_specifier_allow_ & type_specifier_has_) == 0);
    }

    void add_struct_member(StringRef name, Type * type) {
        StructType * st = dynamic_cast<StructType *>(type_);
        CHECK(st != nullptr);
        st->addMember(name, type);
    }
    void set_struct_tag(StringRef tag) {
        StructType * st = dynamic_cast<StructType *>(type_);
        CHECK(st != nullptr);
        st->setTag(tag);
    }
    StructType * get_struct() {
        StructType * st = dynamic_cast<StructType *>(type_);
        CHECK(st != nullptr);
        return st;
    }

    Type * build() const {
        if (type_specifier_has_ & 0xe00) // struct/union/enum/typedef
            return (CHECK(type_), type_);
        else if (type_specifier_has_ == 0x1) // void
            return new VoidType();
        else if (type_specifier_has_ == 0x2) // char
            return new CharType();
        else if (type_specifier_has_ == 0x20) // float
            return new FloatingType("f");
        else if (type_specifier_has_ & 0x40) // double/long double
            return new FloatingType((type_specifier_has_ & 0x10) ? "ld" : "d");
        else
        {
            char * desc = new char[4];
            char * d = desc;
            if (type_specifier_has_ & 0x80)
                *(d++) = 'S';
            if (type_specifier_has_ & 0x100)
                *(d++) = 'U';
            if (type_specifier_has_ & 0x2)
                *(d++) = 'c';
            if (type_specifier_has_ & 0x4)
                *(d++) = 's';
            if (type_specifier_has_ & 0x10)
                *(d++) = 'l';
            if (type_specifier_has_ & 0x8)
                *(d++) = 'i';
            *d = '\0';
            return new IntegerType(desc);
        }
    }
};

class TypeQualifiersBuilder {
    // collect type-qualifier
    // bits:
    //      const,volatile
    int type_qualifier_has_;

public:
    TypeQualifiersBuilder()
        : type_qualifier_has_(0) {
    }
    void reset() {
        type_qualifier_has_ = 0;
    }
    void feed_type_qualifiers(Token::Type t) {
        switch (t)
        {
            case Token::KW_CONST:
                if (type_qualifier_has_ & TP_CONST)
                    SyntaxWarning("duplicate type qualifier 'const'");
                type_qualifier_has_ |= TP_CONST;
                break;
            case Token::KW_VOLATILE:
                if (type_qualifier_has_ & TP_VOLATILE)
                    SyntaxWarning("duplicate type qualifier 'volatile'");
                type_qualifier_has_ |= TP_VOLATILE;
                break;
            default:
                SyntaxError("DeclarationSpecifiersBuilder: unexpected token");
                break;
        }
    }

    int build() const {
        return type_qualifier_has_;
    }
};

std::deque<TypeSpecifiersBuilder> g_SpecifierBuilders;
std::deque<TypeQualifiersBuilder> g_QualifierBuilders;
std::deque<Type *> g_Types;
std::deque<StringRef> g_Ids;
// std::deque<SymbolBuilder> g_SymbolBuilders;
std::deque<Symbol *> g_Symbols;
std::deque<Environment *> g_Envs;
std::deque<ExprNode *> g_ExprTree;
std::deque<Stmt *> g_StmtList;

void new_typ_specifier() {
    g_SpecifierBuilders.push_back({});
}
void typ_specifier_add_token(Token::Type t) {
    CHECK(!g_SpecifierBuilders.empty());
    g_SpecifierBuilders.back().feed_type_specifiers(t);
}
void typ_specifier_struct_add_member(StringRef name, Type * type) {
    CHECK(!g_SpecifierBuilders.empty());
    g_SpecifierBuilders.back().add_struct_member(name, type);
}
void typ_specifier_struct_set_tag(StringRef tag) {
    CHECK(!g_SpecifierBuilders.empty());
    g_SpecifierBuilders.back().set_struct_tag(tag);
}
Type * typ_specifier_get_struct() {
    CHECK(!g_SpecifierBuilders.empty());
    return g_SpecifierBuilders.back().get_struct();
}
Type * typ_pop_specifier() {
    CHECK(!g_SpecifierBuilders.empty());
    Type * specifier = g_SpecifierBuilders.back().build();
    g_SpecifierBuilders.pop_back();
    return specifier;
}

void new_typ_qualifier() {
    g_QualifierBuilders.push_back({});
}
void typ_qualifier_add_token(Token::Type t) {
    CHECK(!g_QualifierBuilders.empty());
    g_QualifierBuilders.back().feed_type_qualifiers(t);
}
int typ_pop_qualifier() {
    CHECK(!g_QualifierBuilders.empty());
    int qualifier = g_QualifierBuilders.back().build();
    g_QualifierBuilders.pop_back();
    return qualifier;
}

void new_typ() {
    g_Types.push_back(nullptr);
}
void new_typ(Type * specifier, int qualifier) {
    g_Types.push_back(specifier);
    g_Types.back()->setQualifier(qualifier);
}
void typ_merge() {
    CHECK(g_Types.size() >= 2);
    Type * top1 = g_Types[g_Types.size() - 1];
    Type * top2 = g_Types[g_Types.size() - 2];

    if (top1 == nullptr)
    {
        // Includes:
        //   top1 == nullptr && top2 != nullptr
        //   top1 == nullptr && top2 == nullptr
        g_Types.pop_back();
    }
    else if (top2 == nullptr)
    {
        g_Types.pop_back();
        g_Types.pop_back();
        g_Types.push_back(top1);
    }
    else
    {
        DerivedType * dt = dynamic_cast<DerivedType *>(top1);
        Type * referenced_type = top2;
        CHECK(dt != nullptr);

        // Derived type chain
        DerivedType * dt_last = dt;
        while (dt_last->getTargetType() != nullptr)
        {
            dt_last = dynamic_cast<DerivedType *>(dt_last->getTargetType());
            CHECK(dt_last != nullptr);
        }
        dt_last->setTargetType(referenced_type);

        g_Types.pop_back();
        g_Types.pop_back();
        g_Types.push_back(dt);
    }
}
void typ_swap_merge() {
    CHECK(g_Types.size() >= 2);
    Type * top1 = g_Types[g_Types.size() - 1];
    g_Types[g_Types.size() - 1] = g_Types[g_Types.size() - 2];
    g_Types[g_Types.size() - 2] = top1;
    typ_merge();
}
void typ_dup2_merge() {
    // TODO: clone type
    CHECK(g_Types.size() >= 2);
    Type * top2 = g_Types[g_Types.size() - 2];
    typ_merge();
    g_Types.push_back(top2);
}
Type * typ_top2() {
    CHECK(g_Types.size() >= 2);
    return g_Types[g_Types.size() - 2];
}
Type * typ_pop() {
    CHECK(!g_Types.empty());
    Type * top1 = g_Types.back();
    g_Types.pop_back();
    return top1;
}
Type * typ_pop2() {
    CHECK(g_Types.size() >= 2);
    Type * top1 = g_Types[g_Types.size() - 1];
    Type * top2 = g_Types[g_Types.size() - 2];
    g_Types.pop_back();
    g_Types.pop_back();
    g_Types.push_back(top1);
    return top2;
}
void del_typ() {
    CHECK(!g_Types.empty());
    //std::cout << "delete: " << g_Types.back()->toString() << std::endl;
    g_Types.pop_back();
}
void typ_func_add_param(StringRef name, Type * type) {
    CHECK(!g_Types.empty());
    FuncType *ft = dynamic_cast<FuncType *>(g_Types.back());
    CHECK(ft != nullptr);
    ft->addParam(name, type);
}
void typ_func_set_varlist() {
    CHECK(!g_Types.empty());
    FuncType *ft = dynamic_cast<FuncType *>(g_Types.back());
    CHECK(ft != nullptr);
    ft->enableVarList();
}
void typ_func_link_env(Environment *env) {
    CHECK(!g_Types.empty());
    FuncType *ft = dynamic_cast<FuncType *>(g_Types.back());
    CHECK(ft != nullptr);
    ft->linkEnv(env);
}
Environment * typ_func_get_env() {
    CHECK(!g_Types.empty());
    FuncType *ft = dynamic_cast<FuncType *>(g_Types.back());
    CHECK(ft != nullptr);
    return ft->getEnv();
}

void new_id(StringRef id) {
    g_Ids.push_back(id);
}
void id_merge() {
    CHECK(g_Ids.size() >= 2);
    StringRef top1 = g_Ids[g_Ids.size() - 1];
    StringRef top2 = g_Ids[g_Ids.size() - 2];
    g_Ids.pop_back();
    g_Ids.pop_back();

    CHECK(top1.empty() || top2.empty());
    g_Ids.push_back(top1.empty() ? top2 : top1);
}
StringRef id_top() {
    CHECK(!g_Ids.empty());
    return g_Ids.back();
}
StringRef id_pop() {
    CHECK(!g_Ids.empty());
    StringRef id = g_Ids.back();
    g_Ids.pop_back();
    return id;
}
void del_id() {
    CHECK(!g_Ids.empty());
    g_Ids.pop_back();
}

void new_sym(Symbol::Namespace space, StringRef name, Type *type) {
    g_Symbols.push_back(new Symbol());
    g_Symbols.back()->space = space;
    g_Symbols.back()->name = name;
    g_Symbols.back()->type = type;
}
Symbol * sym_top() {
    CHECK(!g_Symbols.empty());
    return g_Symbols.back();
}
Symbol * sym_pop() {
    CHECK(!g_Symbols.empty());
    Symbol *sym = g_Symbols.back();
    g_Symbols.pop_back();
    return sym;
}

void new_env() {
    g_Envs.push_back(new Environment());
    if (g_Envs.size() >= 2)
    {
        g_Envs[g_Envs.size() - 1]->setParent(g_Envs[g_Envs.size() - 2]);
    }
}
Environment * env_top() {
    CHECK(!g_Envs.empty());
    return g_Envs.back();
}
Environment * del_env() {
    CHECK(!g_Envs.empty());
    Environment * env = g_Envs.back();
    g_Envs.pop_back();
    return env;
}
void env_push(Environment * env) {
    g_Envs.push_back(env);
}
Environment * env_pop() {
    return del_env();
}
void env_add_sym(Symbol * sym) {
    CHECK(!g_Envs.empty());
    g_Envs.back()->addSymbol(sym);
}
Symbol * env_find_sym(StringRef id) {
    CHECK(!g_Envs.empty());
    Symbol * sym = g_Envs.back()->findSymbol(id);
    CHECK(sym);
    return sym;
}

void expr_push(ExprNode * node) {
    CHECK(node);
    g_ExprTree.push_back(node);
}
ExprNode * expr_pop() {
    CHECK(!g_ExprTree.empty());
    ExprNode * en = g_ExprTree.back();
    g_ExprTree.pop_back();
    return en;
}

void stmt_push(Stmt * stmt) {
    CHECK(stmt);
    g_StmtList.push_back(stmt);
}
Stmt * stmt_pop() {
    CHECK(!g_StmtList.empty());
    Stmt * stmt = g_StmtList.back();
    g_StmtList.pop_back();
    return stmt;
}

void reset() {
    g_Envs.clear();
    g_StmtList.clear();
}
void verify() {
    CHECK(g_SpecifierBuilders.empty());
    CHECK(g_QualifierBuilders.empty());
    CHECK(g_Ids.empty());
    CHECK(g_Types.empty());
    CHECK(g_ExprTree.empty());

    int i = 0;
    for (auto env : g_Envs)
    {
        std::cout << "env[" << i++ << "]:" << std::endl
            << env->toString() << std::endl;
    }

    i = 0;
    for (auto stmt : g_StmtList)
    {
        std::cout << "stmt[" << i++ << "] " << stmt->toString() << std::endl;
    }
}

void Visit(Ast * ast)
{
    ASSERT(ast);
    Ast * child = ast->leftChild;
    bool exist;
    switch(ast->type)
    {
        case TRANSLATION_UNIT:
            new_env();
            while(child)
            {
                Visit(child);
                child = child->rightSibling;
            }
            break;
        case FUNCTION_DEFINITION:
            break;
        case DECLARATION:
            while (child)
            {
                Visit(child);
                child = child->rightSibling;
            }
            del_typ();
            break;
        case DECLARATOR_INITIALIZER:
            Visit(child);
            typ_dup2_merge();
            ASSERT(!id_top().empty());
            new_sym(Symbol::Namespace::ID, id_pop(), typ_pop2());
            env_add_sym(sym_pop());
            break;
        case DECLARATOR:
            exist = (child->type == POINTER_LIST);
            if (exist)
            {
                Visit(child);
                child = child->rightSibling;
            }

            if (child->type == IDENTIFIER)
            {
                new_id(child->token.text);
                new_typ();
            }
            else
            {
                Visit(child);
            }
            child = child->rightSibling;

            if (exist) typ_merge();

            while (child)
            {
                Visit(child);
                child = child->rightSibling;
            }
            break;
        case DIRECT_DECLARATOR:
            if (child->type == IDENTIFIER)
            {
                new_typ();
                new_id(child->token.text);
            }
            else
            {
                Visit(child);
                child = child->rightSibling;
            }
            // TODO: array, param-list
            break;
        case ABSTRACT_DECLARATOR:
            break;
        case DIRECT_ABSTRACT_DECLARATOR:
            break;
        case PARAMETER_LIST_VARLIST:
            while (child)
            {
                Visit(child);
                child = child->rightSibling;
                typ_func_add_param(id_pop(), typ_pop());
            }
            typ_func_set_varlist();
            break;
        case PARAMETER_LIST:
            while (child)
            {
                Visit(child);
                child = child->rightSibling;
                typ_func_add_param(id_pop(), typ_pop());
            }
            break;
        case PARAMETER_DECLARATION:
            break;
        case IDENTIFIER:
            break;
        case IDENTIFIER_LIST:
            break;
        case TYPE_NAME:
            break;
        case POINTER:
            new_typ_qualifier();
            if (child)
            {
                Visit(child);
            }
            new_typ(new PointerType(), typ_pop_qualifier());
            break;
        case POINTER_LIST:
            Visit(child);
            child = child->rightSibling;
            while (child)
            {
                Visit(child);
                child = child->rightSibling;
                typ_merge();
            }
            break;
        case DECLARATION_SPECIFIERS:
            new_typ_specifier();
            new_typ_qualifier();
            while (child)
            {
                Visit(child);
                child = child->rightSibling;
            }
            new_typ(typ_pop_specifier(), typ_pop_qualifier());
            break;
        case STORAGE_CLASS_SPECIFIER:
            break;
        case TYPE_QUALIFIER:
            typ_qualifier_add_token(ast->token.type);
            break;
        case TYPE_SPECIFIER:
            typ_specifier_add_token(ast->token.type);
            break;
        case STRUCT_SPECIFIER:
        case UNION_SPECIFIER:
            typ_specifier_add_token(ast->type == STRUCT_SPECIFIER ? Token::KW_STRUCT : Token::KW_UNION);
            if (child->type == IDENTIFIER)
            {
                typ_specifier_struct_set_tag(child->token.text);
                child = child->rightSibling;
            }
            while (child)
            {
                Visit(child);
                child = child->rightSibling;
            }
            break;
        case STRUCT_DECLARATION:
            new_typ_specifier();
            new_typ_qualifier();
            while (child && child->type != STRUCT_DECLARATOR)
            {
                Visit(child);
                child = child->rightSibling;
            }
            new_typ(typ_pop_specifier(), typ_pop_qualifier());
            while (child)
            {
                ASSERT(child->type == STRUCT_DECLARATOR);
                Visit(child);
                child = child->rightSibling;
                typ_dup2_merge();
                typ_specifier_struct_add_member(id_pop(), typ_pop2());
            }
            del_typ();
            break;
        case STRUCT_DECLARATOR:
            ASSERT(child->type == DECLARATOR);
            Visit(child);
            ASSERT(!id_top().empty());
            break;
        case ENUM_SPECIFIER:
            break;
        case ENUM_CONSTANT:
            break;
        case TYPEDEF_NAME:
            break;
        case INITIALIZER:
            break;
        case INITIALIZER_LIST:
            break;
        case EXPR:
            break;
        case ASSIGN_EXPR:
            break;
        case COND_EXPR:
            break;
        case OR_EXPR:
            break;
        case AND_EXPR:
            break;
        case BIT_OR_EXPR:
            break;
        case BIT_XOR_EXPR:
            break;
        case BIT_AND_EXPR:
            break;
        case EQ_EXPR:
            break;
        case REL_EXPR:
            break;
        case SHIFT_EXPR:
            break;
        case ADD_EXPR:
            break;
        case MUL_EXPR:
            break;
        case CAST_EXPR:
            break;
        case UNARY_EXPR:
            break;
        case POSTFIX_EXPR:
            break;
        case PRIMARY_EXPR:
            break;
        case ARGUMENT_EXPR_LIST:
            break;
        case CONSTANT_EXPR:
            break;
        case STMT:
            break;
        case STMT_LIST:
            break;
        case LABELED_STMT:
            break;
        case COMPOUND_STMT:
            break;
        case EXPRESSION_STMT:
            break;
        case SELECTION_STMT:
            break;
        case IF_STMT:
            break;
        case IF_ELSE_STMT:
            break;
        case SWITCH_STMT:
            break;
        case ITERATION_STMT:
            break;
        case WHILE_STMT:
            break;
        case DO_WHILE_STMT:
            break;
        case FOR_STMT:
            break;
        case JUMP_STMT:
            break;
        case GOTO_STMT:
            break;
        case CONTINUE_STMT:
            break;
        case BREAK_STMT:
            break;
        case RETURN_STMT:
            break;
        default:
            break;
    }
}


std::string IntegerToASM(uint64_t value)
{
    std::string s = "H";
    while (value > 0)
    {
        uint64_t c = (uint64_t)0xf & value;
        value >>= 4;
        s += (char)((c > 9) ? (c - 10 + 'a') : (c + '0'));
    }
    s += "0";
    for (size_t i = 0, j = s.length() - 1; i != j; ++i, --j)
    {
        char c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
    return s;
}

const char * SizeToASM(size_t size)
{
    const char * asmStr = nullptr;
    switch (size)
    {
        case 8: asmStr = "QWORD";   break;
        case 4: asmStr = "DWORD";   break;
        case 2: asmStr = "WORD";    break;
        case 1: asmStr = "BYTE";    break;
        default:                    break;
    }
    return asmStr;
}

enum x64DataType
{
    x64_BYTE    = 1,
    x64_WORD    = 2,
    x64_QWORD   = 4,
    x64_DWORD   = 8,
};

x64DataType SizeToX64DataType(size_t size)
{
    ASSERT(size == 1 || size == 2 || size == 4 || size == 8);
    return (x64DataType)size;
}

const char * X64DataTypeToString(x64DataType dt)
{
    const char * s = nullptr;
    switch (dt)
    {
        case x64_BYTE:   s = "QWORD";   break;
        case x64_WORD:   s = "DWORD";   break;
        case x64_QWORD:  s = "WORD";    break;
        case x64_DWORD:  s = "BYTE";    break;
        default:                        break;
    }
    return s;
}

enum x64InitType
{
    x64_NO_INIT,
    x64_VALUE_INIT,
    x64_ZERO_INIT,
};

struct x64Variable
{
    x64DataType dataType;
    x64InitType initType;
    const char *name;
    size_t count;
    bool needExport;
};

class ASMFile
{
public:
    void AddGlobalVariable(const char *name, size_t size, bool isInitialized, uint64_t initValue)
    {
        x64Variable var =
        {
            SizeToX64DataType(size),
            isInitialized ? x64_VALUE_INIT : x64_NO_INIT,
            name,
            1,
            true,
        };

        if (var.initType == x64_VALUE_INIT)
            dataSegment.push_back(var);
        else
            bssSegment.push_back(var);
    }

    std::string Generate() const
    {
        std::string out;

        // PUBLIC ???
        for (auto & var : dataSegment)
        {
            if (var.needExport)
                out += "PUBLIC ", out += var.name, out += '\n';
        }
        for (auto & var : bssSegment)
        {
            if (var.needExport)
                out += "PUBLIC ", out += var.name, out += '\n';
        }

        // _DATA
        // COMM name:type[:count]
        // name type value
        if (!dataSegment.empty())
            out += "_DATA\n";
        for (auto & var : dataSegment)
        {
            if (var.initType == x64_NO_INIT)
            {
                out += "COMM ";
                out += var.name;
                out += ':';
                out += X64DataTypeToString(var.dataType);
                if (var.count > 1)
                {
                    out += ':';
                    out += IntegerToASM(var.count);
                }
            }
            else
            {
                out += var.name;
                out += ' ';
                out += X64DataTypeToString(var.dataType);
            }
            out += '\n';
        }
        if (!dataSegment.empty())
            out += "_DATA\n";

        // _BSS
        // name type count DUP
        if (!bssSegment.empty())
            out += "_BSS\n";
        for (auto & var : bssSegment)
        {
            out += var.name;
            out += ' ';
            out += X64DataTypeToString(var.dataType);
            out += ' ';
            out += IntegerToASM(var.count);
            out += " DUP\n";
        }
        if (!bssSegment.empty())
            out += "_BSS\n";

        // _TEXT
        // _TEXT

        return out;
    }

private:
    std::vector<x64Variable> dataSegment;
    std::vector<x64Variable> bssSegment;
};

// int, float, char, pointer
void AddGlobalVariable(const char *name, size_t size, bool isInitialized, uint64_t initValue)
{
    if (isInitialized)
    {
        // _TEXT
        std::cout << name << ' ' << SizeToASM(size) << IntegerToASM(initValue).data() << std::endl;
    }
    else
    {
        // _TEXT
        std::cout << "COMM " << name << ":" << SizeToASM(size) << std::endl;
    }
}

void AddGlobalArray(const char *name, size_t elementSize, size_t arrayLength, bool isInitialized, size_t initCount, uint64_t *initValues)
{
    if (isInitialized)
    {
        if (initCount == 1 && initValues[0] == 0)
        {
            // _BSS
            std::cout << name << ' ' << SizeToASM(elementSize) << ' ' << IntegerToASM(arrayLength).data() << " DUP" << std::endl;
        }
        else
        {
            // _TEXT
            std::cout << name << ' ' << SizeToASM(elementSize);
            for (size_t i = 0; i < initCount; ++i)
                std::cout << ' ' << IntegerToASM(initValues[i]).data();
            if (initCount < arrayLength)
            {
                std::cout << ' ' << "ORG $+" << ((arrayLength - initCount) * elementSize);
            }
            std::cout << std::endl;
        }
    }
    else
    {
        // _TEXT
        std::cout << "COMM " << name << ":" << SizeToASM(elementSize) << ":" << IntegerToASM(arrayLength).data() << std::endl;
    }
}

void GenerateX64ASM(Ast * ast)
{
    reset();
    Visit(ast);
    verify();

    ASMFile asmFile;

    Environment * globalEnv = g_Envs[0];
    std::deque<std::string> nameCopy;
    for (auto symbol : globalEnv->allSymbols())
    {
        nameCopy.emplace_back(std::move(symbol->name.toString()));
        asmFile.AddGlobalVariable(nameCopy.back().data(),
                                  symbol->type->getSize(),
                                  false,
                                  0);
    }
    std::cout << asmFile.Generate() << std::endl;
}
