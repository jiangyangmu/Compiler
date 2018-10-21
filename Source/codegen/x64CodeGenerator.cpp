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

    void feed(Token::Type t, Type * type = nullptr) {
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
                type_ = type;
                break;
                // case Token::KW_UNION:
                //    __check_set(0x400);
                //    type_specifier_allow_ &= 0x400;
                //    // TODO: type_ = new UnionType();
                //    break;
            case Token::KW_ENUM:
                __check_set(0x800);
                type_specifier_allow_ &= 0x800;
                type_ = type;
                break;
            default:
                SyntaxError("DeclarationSpecifiersBuilder: unexpected token");
                break;
        }

#undef __check_set

        assert((~type_specifier_allow_ & type_specifier_has_) == 0);
    }

    Type * build() const {
        if (type_specifier_has_ & 0xe00) // struct/union/enum/typedef
            return (CHECK(type_), type_);   // TODO: copy?
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

class StorageBuilder {
    Token::Type storage_;

public:
    StorageBuilder()
        : storage_(Token::UNKNOWN) {
    }
    void set(Token::Type t) {
        CHECK(storage_ == Token::UNKNOWN);
        storage_ = t;
    }
    Token::Type get() const {
        return storage_;
    }
};

enum ASTScope {
    IN_FILE,
    IN_PARAM_LIST,
    IN_COMPOUND_STATEMENT,
    //IN_STRUCT_BODY,
};

enum DType {
    D_OBJECT,
    D_FUNCTION_DECLARATION,
    D_FUNCTION_DEFINITION,
};

std::deque<TypeSpecifiersBuilder> g_SpecifierBuilders;
std::deque<TypeQualifiersBuilder> g_QualifierBuilders;
std::deque<StorageBuilder> g_StorageBuilders;
std::deque<ASTScope> g_Scopes;
std::deque<Type *> g_Types;
std::deque<StringRef> g_Ids;
std::deque<Symbol *> g_Symbols;
std::deque<Environment *> g_Envs;
std::deque<ExprNode *> g_ExprTree;
std::deque<ExprNodeList> g_argumentLists;
std::deque<Stmt *> g_Stmts;

void new_type()
{
    g_Types.push_back(nullptr);
}
void type_merge()
{
    // (pointer,  *)    -> pointer to *
    // (array,    *)    -> array of *
    // (function, *)    -> function of *
    // (empty,    *)    -> *

    CHECK(g_Types.size() >= 2);

    Type * right    = g_Types.back(); g_Types.pop_back();
    Type * left     = g_Types.back(); g_Types.pop_back();

    if (left == nullptr)
    {
        g_Types.push_back(right);
    }
    else if (right == nullptr)
    {
        g_Types.push_back(left);
    }
    else
    {
        DerivedType *   dt  = dynamic_cast<DerivedType *>(left);
        CHECK(dt != nullptr);

        // Derived type chain
        DerivedType *   dt_last = dt;
        while (dt_last->getTargetType() != nullptr)
        {
            dt_last = dynamic_cast<DerivedType *>(dt_last->getTargetType());
            CHECK(dt_last != nullptr);
        }
        dt_last->setTargetType(right);

        g_Types.push_back(left);
    }
}
void type_set_specifier_qualifier(Type * t)
{
    g_Types.push_back(t);
    type_merge();
}
void type_push(Type * t)
{
    g_Types.push_back(t);
}
Type * type_pop()
{
    CHECK(!g_Types.empty());
    Type * top1 = g_Types.back();
    g_Types.pop_back();
    return top1;
}
Type * type_top()
{
    CHECK(!g_Types.empty());
    return g_Types.back();
}

void new_pointer()
{
    g_Types.push_back(new PointerType());
}
void pointer_set_qualifier(int q)
{
    CHECK(!g_Types.empty());
    PointerType * pt = dynamic_cast<PointerType *>(g_Types.back());
    CHECK(pt);
    pt->setQualifier(q);
}

void new_array()
{
    g_Types.push_back(new ArrayType());
}
void array_set_size(size_t size)
{
    CHECK(!g_Types.empty());
    ArrayType * at = dynamic_cast<ArrayType *>(g_Types.back());
    CHECK(at);
    at->setSize(size);
}

void new_function()
{
    g_Types.push_back(new FuncType());
}
void function_add_parameter(StringRef name, Type * type)
{
    CHECK(!g_Types.empty());
    FuncType * ft = dynamic_cast<FuncType *>(g_Types.back());
    CHECK(ft);
    ft->addParam(name, type);
}
void function_set_varlist()
{
    CHECK(!g_Types.empty());
    FuncType * ft = dynamic_cast<FuncType *>(g_Types.back());
    CHECK(ft);
    ft->enableVarList();
}
void function_set_body(Stmt * stmt)
{
    CHECK(!g_Types.empty());
    FuncType * ft = dynamic_cast<FuncType *>(g_Types.back());
    CHECK(ft);
    ft->setBody(stmt);
}
void function_set_env(Environment * env)
{
    CHECK(!g_Types.empty());
    FuncType * ft = dynamic_cast<FuncType *>(g_Types.back());
    CHECK(ft);
    ft->linkEnv(env);
}
Environment * function_get_env()
{
    CHECK(!g_Types.empty());
    FuncType * ft = dynamic_cast<FuncType *>(g_Types.back());
    CHECK(ft);
    return ft->getEnv();
}

void new_id(StringRef id)
{
    g_Ids.push_back(id);
}
StringRef id_pop()
{
    CHECK(!g_Ids.empty());
    StringRef id = g_Ids.back();
    g_Ids.pop_back();
    return id;
}

void new_struct()
{
    g_Types.push_back(new StructType());
}
void struct_set_name(StringRef name)
{
    CHECK(!g_Types.empty());
    StructType * st = dynamic_cast<StructType *>(g_Types.back());
    CHECK(st);
    st->setTag(name);
}
StringRef struct_get_name()
{
    CHECK(!g_Types.empty());
    StructType * st = dynamic_cast<StructType *>(g_Types.back());
    CHECK(st);
    return st->getTag();
}
void struct_add_member(StringRef name, Type * type)
{
    CHECK(!g_Types.empty());
    StructType * st = dynamic_cast<StructType *>(g_Types.back());
    CHECK(st);
    st->addMember(name, type);
}
void struct_complete()
{
    CHECK(!g_Types.empty());
    StructType * st = dynamic_cast<StructType *>(g_Types.back());
    CHECK(st);
    st->markComplete();
}

void new_enum()
{
    g_Types.push_back(new EnumType());
}
void enum_set_name(StringRef name)
{
    CHECK(!g_Types.empty());
    EnumType * et = dynamic_cast<EnumType *>(g_Types.back());
    CHECK(et);
    et->setTag(name);
}
StringRef enum_get_name()
{
    CHECK(!g_Types.empty());
    EnumType * et = dynamic_cast<EnumType *>(g_Types.back());
    CHECK(et);
    return et->getTag();
}
void enum_add_const(StringRef name)
{
    CHECK(!g_Types.empty());
    EnumType * et = dynamic_cast<EnumType *>(g_Types.back());
    CHECK(et);
    et->addEnumConst(name);
}
void enum_set_const_value(int value)
{
    CHECK(!g_Types.empty());
    EnumType * et = dynamic_cast<EnumType *>(g_Types.back());
    CHECK(et);
    et->setLastEnumConstValue(value);
}
void enum_complete()
{
    CHECK(!g_Types.empty());
    EnumType * et = dynamic_cast<EnumType *>(g_Types.back());
    CHECK(et);
    et->markComplete();
}

void specifier_qualifier_begin()
{
    g_SpecifierBuilders.push_back({});
    g_QualifierBuilders.push_back({});
}
void qualifier_begin()
{
    g_QualifierBuilders.push_back({});
}
void specifier_add(Token::Type token_type, Type * type = nullptr)
{
    CHECK(!g_SpecifierBuilders.empty());
    g_SpecifierBuilders.back().feed(token_type, type);
}
void qualifier_add(Token::Type token_type)
{
    CHECK(!g_QualifierBuilders.empty());
    g_QualifierBuilders.back().feed_type_qualifiers(token_type);
}
int qualifier_get()
{
    CHECK(!g_QualifierBuilders.empty());
    return g_QualifierBuilders.back().build();
}
Type * specifier_qualifier_get()
{
    CHECK(!g_QualifierBuilders.empty());
    CHECK(!g_SpecifierBuilders.empty());
    Type * t = g_SpecifierBuilders.back().build();
    t->setQualifier(g_QualifierBuilders.back().build());
    return t;
}
void qualifier_end()
{
    CHECK(!g_QualifierBuilders.empty());
    g_QualifierBuilders.pop_back();
}
void specifier_qualifier_end()
{
    CHECK(!g_QualifierBuilders.empty());
    CHECK(!g_SpecifierBuilders.empty());
    g_QualifierBuilders.pop_back();
    g_SpecifierBuilders.pop_back();
}

void storage_begin()
{
    g_StorageBuilders.push_back({});
}
void storage_add(Token::Type token_type)
{
    CHECK(!g_StorageBuilders.empty());
    g_StorageBuilders.back().set(token_type);
}
Token::Type storage_get()
{
    CHECK(!g_StorageBuilders.empty());
    return g_StorageBuilders.back().get();
}
void storage_end()
{
    CHECK(!g_StorageBuilders.empty());
    g_StorageBuilders.pop_back();
}

void scope_begin(ASTScope scope)
{
    g_Scopes.push_back(scope);
}
ASTScope scope_get()
{
    CHECK(!g_Scopes.empty());
    return g_Scopes.back();
}
void scope_end()
{
    CHECK(!g_Scopes.empty());
    g_Scopes.pop_back();
}

Symbol::StorageType compute_storage_type(ASTScope scope,
                                         Token::Type storageToken,
                                         DType dtype,
                                         Symbol * file_scope_symbol)
{
    if (storageToken == Token::KW_TYPEDEF)
        return Symbol::NONE;

    if (scope == IN_FILE)
    {
        if (storageToken == Token::KW_STATIC)
        {
            return Symbol::THIS_TRANSLATION_UNIT;
        }

        CHECK(storageToken == Token::UNKNOWN ||
              storageToken == Token::KW_EXTERN);

        if (dtype == D_OBJECT)
        {
            if (storageToken == Token::KW_EXTERN)
            {
                return file_scope_symbol
                       ? Symbol::CHECK_FILE_SCOPE_SYMBOL
                       : Symbol::OTHER_TRANSLATION_UNIT;
            }
            else
            {
                return Symbol::THIS_TRANSLATION_UNIT;
            }
        }
        else if (dtype == D_FUNCTION_DECLARATION)
        {
            return Symbol::OTHER_TRANSLATION_UNIT;
        }
        else
        {
            return Symbol::THIS_TRANSLATION_UNIT;
        }
    }
    else if (scope == IN_COMPOUND_STATEMENT)
    {
        if (dtype == D_OBJECT)
        {
            if (storageToken == Token::KW_STATIC)
            {
                return Symbol::THIS_TRANSLATION_UNIT;
            }
            else if (storageToken == Token::KW_EXTERN)
            {
                return file_scope_symbol
                       ? Symbol::CHECK_FILE_SCOPE_SYMBOL
                       : Symbol::OTHER_TRANSLATION_UNIT;
            }
            else
            {
                CHECK(storageToken == Token::UNKNOWN);
                return Symbol::LOCAL;
            }
        }
        else
        {
            CHECK(dtype == D_FUNCTION_DECLARATION);
            return file_scope_symbol
                   ? Symbol::CHECK_FILE_SCOPE_SYMBOL
                   : Symbol::OTHER_TRANSLATION_UNIT;
        }
    }
    else if (scope == IN_PARAM_LIST)
    {
        return Symbol::LOCAL;
    }
    else
    {
        return Symbol::NONE;
    }
}

void new_symbol()
{
    g_Symbols.push_back(new Symbol());
}
void symbol_set_id(StringRef name)
{
    CHECK(!g_Symbols.empty());
    g_Symbols.back()->name = name;
}
void symbol_set_symbol_type(Symbol::SymbolType symbolType)
{
    CHECK(!g_Symbols.empty());
    g_Symbols.back()->symbolType = symbolType;
}
void symbol_set_object_type(Type * objectType)
{
    CHECK(!g_Symbols.empty());
    g_Symbols.back()->objectType = objectType;
}
void symbol_set_storage_type(Symbol::StorageType storageType)
{
    CHECK(!g_Symbols.empty());
    g_Symbols.back()->storageType = storageType;
}
void symbol_set_export()
{
    CHECK(!g_Symbols.empty());
    g_Symbols.back()->needExport = true;
}
Symbol * symbol_top()
{
    CHECK(!g_Symbols.empty());
    return g_Symbols.back();
}
Symbol * symbol_pop()
{
    CHECK(!g_Symbols.empty());
    Symbol * symbol = g_Symbols.back();
    g_Symbols.pop_back();
    return symbol;
}

void new_env()
{
    g_Envs.push_back(new Environment());
    if (g_Envs.size() >= 2)
    {
        g_Envs[g_Envs.size() - 1]->setParent(g_Envs[g_Envs.size() - 2]);
    }
}
void env_push(Environment * env)
{
    g_Envs.push_back(env);
}
void env_add_symbol(Symbol * symbol)
{
    CHECK(!g_Envs.empty());
    g_Envs.back()->addSymbol(symbol);
}
Symbol * env_find(StringRef name, Symbol::SymbolType symbolType)
{
    CHECK(!g_Envs.empty());
    return g_Envs.back()->findSymbol(name, symbolType);
}
Symbol * env_find_in_file_scope(StringRef name, Symbol::SymbolType symbolType)
{
    CHECK(!g_Envs.empty());
    Environment * file_scope_env = g_Envs.front();
    Symbol * symbol = file_scope_env->findSymbol(name, symbolType);
    return symbol;
}
Environment * env_pop()
{
    CHECK(!g_Envs.empty());
    Environment * env = g_Envs.back();
    g_Envs.pop_back();
    return env;
}

Stmt * stmt_top()
{
    CHECK(!g_Stmts.empty());
    return g_Stmts.back();
}
Stmt * stmt_pop()
{
    CHECK(!g_Stmts.empty());
    Stmt * stmt = g_Stmts.back();
    g_Stmts.pop_back();
    return stmt;
}

void new_label_stmt()
{
    g_Stmts.push_back(new LabelStmt());
}
void label_stmt_set_label(StringRef label)
{
    CHECK(!g_Stmts.empty());
    LabelStmt * ls = dynamic_cast<LabelStmt *>(g_Stmts.back());
    CHECK(ls);
    ls->label = label;
}
void label_stmt_set_stmt(Stmt * stmt)
{
    CHECK(!g_Stmts.empty());
    LabelStmt * ls = dynamic_cast<LabelStmt *>(g_Stmts.back());
    CHECK(ls);
    ls->stmt = stmt;
}

void new_case_stmt()
{
    g_Stmts.push_back(new CaseStmt());
}
void case_stmt_set_expr(ExprNode * expr)
{
    CHECK(!g_Stmts.empty());
    CaseStmt * cs = dynamic_cast<CaseStmt *>(g_Stmts.back());
    CHECK(cs);
    cs->expr = expr;
}
void case_stmt_set_stmt(Stmt * stmt)
{
    CHECK(!g_Stmts.empty());
    CaseStmt * cs = dynamic_cast<CaseStmt *>(g_Stmts.back());
    CHECK(cs);
    cs->stmt = stmt;
}

void new_default_stmt()
{
    g_Stmts.push_back(new DefaultStmt());
}
void default_stmt_set_stmt(Stmt * stmt)
{
    CHECK(!g_Stmts.empty());
    DefaultStmt * ds = dynamic_cast<DefaultStmt *>(g_Stmts.back());
    CHECK(ds);
    ds->stmt = stmt;
}

void new_compound_stmt()
{
    g_Stmts.push_back(new CompoundStmt());
}
void compound_stmt_add(Stmt * stmt)
{
    CHECK(!g_Stmts.empty());
    CompoundStmt * cs = dynamic_cast<CompoundStmt *>(g_Stmts.back());
    CHECK(cs);
    cs->stmts.push_back(stmt);
}
void compound_stmt_set_env(Environment * env)
{
    CHECK(!g_Stmts.empty());
    CompoundStmt * cs = dynamic_cast<CompoundStmt *>(g_Stmts.back());
    CHECK(cs);
    cs->env = env;
}

void new_expr_stmt()
{
    g_Stmts.push_back(new ExpressionStmt());
}
void expr_stmt_set_expr(ExprNode * expr)
{
    CHECK(!g_Stmts.empty());
    ExpressionStmt * es = dynamic_cast<ExpressionStmt *>(g_Stmts.back());
    CHECK(es);
    es->expr = expr;
}

void new_if_stmt()
{
    g_Stmts.push_back(new IfStmt());
}
void if_stmt_set_expr(ExprNode * expr)
{
    CHECK(!g_Stmts.empty());
    IfStmt * is = dynamic_cast<IfStmt *>(g_Stmts.back());
    CHECK(is);
    is->cond_expr = expr;
}
void if_stmt_set_if_block(Stmt * stmt)
{
    CHECK(!g_Stmts.empty());
    IfStmt * is = dynamic_cast<IfStmt *>(g_Stmts.back());
    CHECK(is);
    is->if_stmt = stmt;
}
void if_stmt_set_else_block(Stmt * stmt)
{
    CHECK(!g_Stmts.empty());
    IfStmt * is = dynamic_cast<IfStmt *>(g_Stmts.back());
    CHECK(is);
    is->else_stmt = stmt;
}

void new_switch_stmt()
{
    g_Stmts.push_back(new SwitchStmt());
}
void switch_stmt_set_expr(ExprNode * expr)
{
    CHECK(!g_Stmts.empty());
    SwitchStmt * ss = dynamic_cast<SwitchStmt *>(g_Stmts.back());
    CHECK(ss);
    ss->cond_expr = expr;
}
void switch_stmt_set_stmt(Stmt * stmt)
{
    CHECK(!g_Stmts.empty());
    SwitchStmt * ss = dynamic_cast<SwitchStmt *>(g_Stmts.back());
    CHECK(ss);
    ss->stmt = stmt;
}

void new_while_stmt()
{
    g_Stmts.push_back(new WhileStmt());
}
void while_stmt_set_expr(ExprNode * expr)
{
    CHECK(!g_Stmts.empty());
    WhileStmt * ws = dynamic_cast<WhileStmt *>(g_Stmts.back());
    CHECK(ws);
    ws->cond_expr = expr;
}
void while_stmt_set_stmt(Stmt * stmt)
{
    CHECK(!g_Stmts.empty());
    WhileStmt * ws = dynamic_cast<WhileStmt *>(g_Stmts.back());
    CHECK(ws);
    ws->stmt = stmt;
}

void new_do_while_stmt()
{
    g_Stmts.push_back(new DoWhileStmt());
}
void do_while_stmt_set_expr(ExprNode * expr)
{
    CHECK(!g_Stmts.empty());
    DoWhileStmt * ws = dynamic_cast<DoWhileStmt *>(g_Stmts.back());
    CHECK(ws);
    ws->cond_expr = expr;
}
void do_while_stmt_set_stmt(Stmt * stmt)
{
    CHECK(!g_Stmts.empty());
    DoWhileStmt * ws = dynamic_cast<DoWhileStmt *>(g_Stmts.back());
    CHECK(ws);
    ws->stmt = stmt;
}

void new_for_stmt()
{
    g_Stmts.push_back(new ForStmt());
}
void for_stmt_set_init_expr(ExprNode * expr)
{
    CHECK(!g_Stmts.empty());
    ForStmt * fs = dynamic_cast<ForStmt *>(g_Stmts.back());
    CHECK(fs);
    fs->init_expr = expr;
}
void for_stmt_set_cond_expr(ExprNode * expr)
{
    CHECK(!g_Stmts.empty());
    ForStmt * fs = dynamic_cast<ForStmt *>(g_Stmts.back());
    CHECK(fs);
    fs->loop_expr = expr;
}
void for_stmt_set_tail_expr(ExprNode * expr)
{
    CHECK(!g_Stmts.empty());
    ForStmt * fs = dynamic_cast<ForStmt *>(g_Stmts.back());
    CHECK(fs);
    fs->tail_expr = expr;
}
void for_stmt_set_stmt(Stmt * stmt)
{
    CHECK(!g_Stmts.empty());
    ForStmt * fs = dynamic_cast<ForStmt *>(g_Stmts.back());
    CHECK(fs);
    fs->stmt = stmt;
}

void new_goto_stmt()
{
    g_Stmts.push_back(new GotoStmt());
}
void goto_stmt_set_label(StringRef label)
{
    CHECK(!g_Stmts.empty());
    GotoStmt * gs = dynamic_cast<GotoStmt *>(g_Stmts.back());
    CHECK(gs);
    gs->label = label;
}

void new_continue_stmt()
{
    g_Stmts.push_back(new ContinueStmt());
}

void new_break_stmt()
{
    g_Stmts.push_back(new BreakStmt());
}

void new_return_stmt()
{
    g_Stmts.push_back(new ReturnStmt());
}
void return_stmt_set_expr(ExprNode * expr)
{
    CHECK(!g_Stmts.empty());
    ReturnStmt * rs = dynamic_cast<ReturnStmt *>(g_Stmts.back());
    CHECK(rs);
    rs->expr = expr;
}

ExprNode * expr_pop()
{
    CHECK(!g_ExprTree.empty());
    ExprNode * e = g_ExprTree.back();
    g_ExprTree.pop_back();
    return e;
}

void new_argument_list()
{
    g_argumentLists.push_back({nullptr, nullptr});
}
void argument_add(ExprNode * expr)
{
    CHECK(!g_argumentLists.empty());
    ExprNodeList * last = &g_argumentLists.back();
    while (last->next)
    {
        last = last->next;
    }
    last->next = new ExprNodeList;
    last       = last->next;
    last->node = expr;
    last->next = nullptr;
}
ExprNodeList argument_list_pop()
{
    ExprNodeList head = g_argumentLists.back();
    g_argumentLists.pop_back();
    return head;
}

void expr_comma(ExprNode * commaExpr, ExprNode * assignExpr)
{
    g_ExprTree.push_back(ExprTreeBuilder::Comma(commaExpr, assignExpr));
}
void expr_assign(ExprNode * condExpr, Token::Type op, ExprNode * assignExpr)
{
    g_ExprTree.push_back(ExprTreeBuilder::Assign(condExpr, assignExpr));
}
void expr_cond(ExprNode * orExpr, ExprNode * commaExpr, ExprNode * condExpr)
{
    g_ExprTree.push_back(ExprTreeBuilder::Cond(orExpr, commaExpr, condExpr));
}
void expr_or(ExprNode * orExpr, ExprNode * andExpr)
{
    g_ExprTree.push_back(ExprTreeBuilder::Or(orExpr, andExpr));
}
void expr_and(ExprNode * andExpr, ExprNode * bitOrExpr)
{
    g_ExprTree.push_back(ExprTreeBuilder::And(andExpr, bitOrExpr));
}
void expr_bit_or(ExprNode * bitOrExpr, ExprNode * bitXorExpr)
{
    g_ExprTree.push_back(ExprTreeBuilder::BitOr(bitOrExpr, bitXorExpr));
}
void expr_bit_xor(ExprNode * bitXorExpr, ExprNode * bitAndExpr)
{
    g_ExprTree.push_back(ExprTreeBuilder::BitXor(bitXorExpr, bitAndExpr));
}
void expr_bit_and(ExprNode * bitAndExpr, ExprNode * eqExpr)
{
    g_ExprTree.push_back(ExprTreeBuilder::BitAnd(bitAndExpr, eqExpr));
}
void expr_eq(ExprNode * eqExpr, Token::Type op, ExprNode * relExpr)
{
    g_ExprTree.push_back(ExprTreeBuilder::Eq(eqExpr, relExpr));
}
void expr_rel(ExprNode * relExpr, Token::Type op, ExprNode * shiftExpr)
{
    g_ExprTree.push_back(ExprTreeBuilder::Rel(relExpr, shiftExpr));
}
void expr_shift(ExprNode * shiftExpr, Token::Type op, ExprNode * addExpr)
{
    g_ExprTree.push_back(ExprTreeBuilder::Shift(shiftExpr, addExpr));
}
void expr_add(ExprNode * addExpr, Token::Type op, ExprNode * mulExpr)
{
    g_ExprTree.push_back(ExprTreeBuilder::Add(addExpr, mulExpr));
}
void expr_mul(ExprNode * mulExpr, Token::Type op, ExprNode * castExpr)
{
    g_ExprTree.push_back(ExprTreeBuilder::Mul(mulExpr, castExpr));
}
void expr_cast(Type * type, ExprNode * unaryExpr)
{
    g_ExprTree.push_back(ExprTreeBuilder::Cast(type, unaryExpr));
}
void expr_unary(Token::Type op, ExprNode * unaryOrCastExpr)
{
    switch (op)
    {
        case Token::OP_INC:   g_ExprTree.push_back(ExprTreeBuilder::PrefixInc(unaryOrCastExpr)); break;
        case Token::OP_DEC:   g_ExprTree.push_back(ExprTreeBuilder::PrefixDec(unaryOrCastExpr)); break;
        case Token::BIT_AND:  g_ExprTree.push_back(ExprTreeBuilder::GetAddress(unaryOrCastExpr)); break;
        case Token::OP_MUL:   g_ExprTree.push_back(ExprTreeBuilder::Indirect(unaryOrCastExpr)); break;
        case Token::OP_ADD:   g_ExprTree.push_back(ExprTreeBuilder::Positive(unaryOrCastExpr)); break;
        case Token::OP_SUB:   g_ExprTree.push_back(ExprTreeBuilder::Negative(unaryOrCastExpr)); break;
        case Token::BIT_NOT:  g_ExprTree.push_back(ExprTreeBuilder::BitNot(unaryOrCastExpr)); break;
        case Token::BOOL_NOT: g_ExprTree.push_back(ExprTreeBuilder::Not(unaryOrCastExpr)); break;
        default:              ASSERT(false); break;
    }
}
//void expr_unary(size_t sizeOfValue)
//{
//    g_ExprTree.push_back(ExprTreeBuilder::Unary(token));
//}
void expr_postfix(ExprNode * primary, ExprNode * arraySizeExpr)
{
    g_ExprTree.push_back(ExprTreeBuilder::Subscript(primary, arraySizeExpr));
}
void expr_postfix(ExprNode * primary, ExprNodeList argumentList)
{
    g_ExprTree.push_back(ExprTreeBuilder::Call(primary, argumentList));
}
void expr_postfix(ExprNode * primary, Token::Type op, StringRef memberId)
{
    switch (op)
    {
        case Token::OP_DOT:     g_ExprTree.push_back(ExprTreeBuilder::GetMember(primary, memberId)); break;
        case Token::OP_POINTTO: g_ExprTree.push_back(ExprTreeBuilder::IndirectGetMember(primary, memberId)); break;
        default:                ASSERT(false); break;
    }
}
void expr_postfix(ExprNode * primary, Token::Type op)
{
    switch (op)
    {
        case Token::OP_INC:     g_ExprTree.push_back(ExprTreeBuilder::PostfixInc(primary)); break;
        case Token::OP_DEC:     g_ExprTree.push_back(ExprTreeBuilder::PostfixDec(primary)); break;
        default:                ASSERT(false); break;
    }
}
void expr_primary(Token token)
{
    if (token.type == Token::ID)
    {
        Symbol * symbol = env_find(token.text, Symbol::ID);
        ASSERT(symbol);
        g_ExprTree.push_back(ExprTreeBuilder::Primary(symbol));
    }
    else
    {
        g_ExprTree.push_back(ExprTreeBuilder::Primary(token));
    }
}

void reset() {
    g_Envs.clear();
    g_Stmts.clear();
}
void verify() {
    CHECK(g_SpecifierBuilders.empty());
    CHECK(g_QualifierBuilders.empty());
    CHECK(g_StorageBuilders.empty());
    CHECK(g_Scopes.empty());
    CHECK(g_Types.empty());
    CHECK(g_Ids.empty());
    CHECK(g_Symbols.empty());
    CHECK(g_argumentLists.empty());
    CHECK(g_ExprTree.empty());

    int i = 0;
    for (auto env : g_Envs)
    {
        std::cout << "env[" << i++ << "]:" << std::endl
            << env->toString() << std::endl;
    }

    i = 0;
    for (auto stmt : g_Stmts)
    {
        std::cout << "stmt[" << i++ << "]" << std::endl
            << StatementToString(stmt) << std::endl;
    }
}

void Visit(Ast * ast)
{
    ASSERT(ast);
    Ast * child = ast->leftChild;
    ExprNode * e1;
    switch(ast->type)
    {
        case TRANSLATION_UNIT:
            new_env();
            scope_begin(IN_FILE);
            while(child)
            {
                Visit(child);
                child = child->rightSibling;
            }
            scope_end();
            break;
        case FUNCTION_DEFINITION:
            specifier_qualifier_begin();
            storage_begin();
            
            Visit(child);
            child = child->rightSibling;
            Visit(child);
            child = child->rightSibling;

            CHECK(type_top()->isFunction());
            type_set_specifier_qualifier(specifier_qualifier_get());
            specifier_qualifier_end();

            new_env();
            function_set_env(env_pop());

            new_symbol();
            symbol_set_id(id_pop());
            symbol_set_symbol_type(
                storage_get() == Token::KW_TYPEDEF
                ? Symbol::ALIAS
                : Symbol::ID
            );
            symbol_set_storage_type(
                compute_storage_type(
                    IN_FILE,
                    storage_get(),
                    D_FUNCTION_DEFINITION,
                    env_find_in_file_scope(
                        symbol_top()->name,
                        symbol_top()->symbolType
                    )
                )
            );
            symbol_set_object_type(type_top());            
            if (symbol_top()->storageType == Symbol::THIS_TRANSLATION_UNIT &&
                storage_get() != Token::KW_STATIC)
            {
                symbol_set_export();
            }
            storage_end();
            env_add_symbol(symbol_pop());

            env_push(function_get_env());
            scope_begin(IN_COMPOUND_STATEMENT);
            Visit(child);
            child = child->rightSibling;
            scope_end();
            env_pop();

            function_set_body(stmt_top());
            type_pop();
            break;
        case DECLARATION:
            specifier_qualifier_begin();
            storage_begin();
            while (child)
            {
                Visit(child);
                child = child->rightSibling;
            }
            storage_end();
            specifier_qualifier_end();
            break;
        case DECLARATOR_INITIALIZER:
            Visit(child);

            type_set_specifier_qualifier(specifier_qualifier_get());

            new_symbol();
            symbol_set_id(id_pop());
            symbol_set_symbol_type(
                storage_get() == Token::KW_TYPEDEF
                ? Symbol::ALIAS
                : Symbol::ID
            );
            symbol_set_storage_type(
                compute_storage_type(
                    scope_get(),
                    storage_get(),
                    type_top()->isFunction()
                        ? D_FUNCTION_DECLARATION
                        : D_OBJECT,
                    env_find_in_file_scope(
                        symbol_top()->name,
                        symbol_top()->symbolType
                    )
                )
            );
            symbol_set_object_type(type_pop());
            if (symbol_top()->storageType == Symbol::THIS_TRANSLATION_UNIT &&
                storage_get() != Token::KW_STATIC)
            {
                symbol_set_export();
            }

            env_add_symbol(symbol_pop());

            // TODO: initializer
            break;
        case DECLARATOR:
            new_type();

            if (child->type == POINTER_LIST)
            {
                Visit(child);
                child = child->rightSibling;

                type_merge();
            }

            if (child->type == IDENTIFIER)
            {
                new_id(child->token.text);
                new_type();
            }
            else
            {
                ASSERT(child->type == DECLARATOR);
                Visit(child);
            }
            child = child->rightSibling;

            while (child)
            {
                if (child->type == PRIMARY_EXPR)
                {
                    new_array();
                    ASSERT(child->token.type == Token::CONST_INT);
                    array_set_size(child->token.ival);

                    child = child->rightSibling;

                    type_merge();
                }
                else
                {
                    ASSERT(child->type == PARAMETER_LIST);

                    new_function();

                    Visit(child);
                    child = child->rightSibling;

                    type_merge();
                }
            }

            type_merge();
            break;
        case DIRECT_DECLARATOR:
            break;
        case ABSTRACT_DECLARATOR:
            break;
        case DIRECT_ABSTRACT_DECLARATOR:
            break;
        case PARAMETER_VAR_LIST:
            break;
        case PARAMETER_LIST:
            scope_begin(IN_PARAM_LIST);

            while (child)
            {
                if (child->type != PARAMETER_VAR_LIST)
                {
                    Visit(child);
                }
                else
                {
                    function_set_varlist();
                }
                child = child->rightSibling;
            }

            scope_end();
            break;
        case PARAMETER_DECLARATION:
            specifier_qualifier_begin();
            storage_begin();

            Visit(child);
            child = child->rightSibling;
            Visit(child);
            child = child->rightSibling;

            type_set_specifier_qualifier(specifier_qualifier_get());
            specifier_qualifier_end();
            storage_end();

            function_add_parameter(id_pop(), type_pop());
            break;
        case IDENTIFIER:
            break;
        case IDENTIFIER_LIST:
            break;
        case TYPE_NAME:
            break;
        case POINTER:
            qualifier_begin();

            while (child)
            {
                Visit(child);
                child = child->rightSibling;
            }

            new_pointer();
            pointer_set_qualifier(qualifier_get());
            qualifier_end();
            break;
        case POINTER_LIST:
            Visit(child);
            child = child->rightSibling;
            while (child)
            {
                Visit(child);
                child = child->rightSibling;

                type_merge();
            }
            break;
        case DECLARATION_SPECIFIERS:
            while (child)
            {
                Visit(child);
                child = child->rightSibling;
            }
            break;
        case STORAGE_CLASS_SPECIFIER:
            if (scope_get() == IN_PARAM_LIST)
            {
                CHECK(ast->token.type == Token::KW_AUTO ||
                      ast->token.type == Token::KW_REGISTER);
            }
            storage_add(ast->token.type);
            break;
        case TYPE_QUALIFIER:
            qualifier_add(ast->token.type);
            break;
        case TYPE_SPECIFIER:
            specifier_add(ast->token.type);
            break;
        case STRUCT_SPECIFIER:
            new_struct();

            if (child->type == IDENTIFIER)
            {
                struct_set_name(child->token.text);

                child = child->rightSibling;
            }

            if (child)
            {
                do
                {
                    ASSERT(child->type == STRUCT_DECLARATION);
                    Visit(child);
                    child = child->rightSibling;
                }
                while (child);

                struct_complete();
            }

            new_symbol();
            symbol_set_id(struct_get_name());
            symbol_set_symbol_type(Symbol::TAG);
            symbol_set_storage_type(Symbol::NONE);
            symbol_set_object_type(type_top());
            env_add_symbol(symbol_pop());

            specifier_add(Token::KW_STRUCT, type_pop());
            break;
        case UNION_SPECIFIER:
            // TODO: support union specifier
            ASSERT(false);
            specifier_add(Token::KW_UNION, type_pop());
            break;
        case STRUCT_DECLARATION:
            specifier_qualifier_begin();

            while (child && child->type != STRUCT_DECLARATOR)
            {
                ASSERT(child->type == TYPE_SPECIFIER ||
                       child->type == TYPE_QUALIFIER);
                Visit(child);
                child = child->rightSibling;
            }

            ASSERT(child);
            
            while (child)
            {
                ASSERT(child->type == STRUCT_DECLARATOR);
                Visit(child);
                child = child->rightSibling;

                type_set_specifier_qualifier(specifier_qualifier_get());
                struct_add_member(id_pop(), type_pop());
            }

            specifier_qualifier_end();
            break;
        case STRUCT_DECLARATOR:
            ASSERT(child && child->type == DECLARATOR);
            Visit(child);
            child = child->rightSibling;
            // TODO: bit-field
            break;
        case ENUM_SPECIFIER:
            new_enum();

            ASSERT(child);
            if (child->type == IDENTIFIER)
            {
                enum_set_name(child->token.text);

                child = child->rightSibling;
            }

            if (child)
            {
                do
                {
                    ASSERT(child->type == ENUM_CONSTANT);

                    enum_add_const(child->token.text);

                    new_symbol();
                    symbol_set_id(child->token.text);
                    symbol_set_symbol_type(Symbol::ID);
                    symbol_set_storage_type(Symbol::NONE);
                    symbol_set_object_type(type_top());
                    env_add_symbol(symbol_pop());

                    if (child->leftChild)
                    {
                        ASSERT(child->leftChild->type == CONSTANT_EXPR);
                        enum_set_const_value(child->leftChild->token.ival);
                    }

                    child = child->rightSibling;
                }
                while (child);

                enum_complete();
            }

            new_symbol();
            symbol_set_id(enum_get_name());
            symbol_set_storage_type(Symbol::NONE);
            symbol_set_object_type(type_top());
            env_add_symbol(symbol_pop());

            specifier_add(Token::KW_ENUM, type_pop());
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
        case COMMA_EXPR:
        case ASSIGN_EXPR:
        case COND_EXPR:
        case OR_EXPR:
        case AND_EXPR:
        case BIT_OR_EXPR:
        case BIT_XOR_EXPR:
        case BIT_AND_EXPR:
        case EQ_EXPR:
        case REL_EXPR:
        case SHIFT_EXPR:
        case ADD_EXPR:
        case MUL_EXPR:
            while (child)
            {
                Visit(child);
                child = child->rightSibling;
            }
            break;
        case CAST_EXPR:
            break;
        case UNARY_EXPR:
            Visit(child);
            expr_unary(ast->token.type, expr_pop());
            break;
        case POSTFIX_EXPR:
            Visit(child);
            if (!child->rightSibling)
            {
                expr_postfix(expr_pop(), ast->token.type);
            }
            else
            {
                child = child->rightSibling;

                if (child->type > EXPR && child->type < ARGUMENT_EXPR_LIST)
                {
                    e1 = expr_pop();
                    Visit(child);
                    expr_postfix(e1, expr_pop());
                }
                else if (child->type == ARGUMENT_EXPR_LIST)
                {
                    Visit(child);
                    expr_postfix(expr_pop(), argument_list_pop());
                }
                else
                {
                    ASSERT(child->type == IDENTIFIER);
                    expr_postfix(expr_pop(), ast->token.type, child->token.text);
                }
            }
            break;
        case PRIMARY_EXPR:
            expr_primary(ast->token);
            break;
        case ARGUMENT_EXPR_LIST:
            new_argument_list();
            while (child)
            {
                Visit(child);
                child = child->rightSibling;
                argument_add(expr_pop());
            }
            break;
        case CONSTANT_EXPR:
            break;
        case STMT:
            break;
        case STMT_LIST:
            break;
        case LABELED_STMT:
            break;
        case LABEL_STMT:
            new_label_stmt();
            
            ASSERT(child && child->type == IDENTIFIER);
            label_stmt_set_label(child->token.text);
            child = child->rightSibling;

            Visit(child);
            label_stmt_set_stmt(stmt_pop());
            break;
        case CASE_STMT:
            new_case_stmt();
            
            Visit(child);
            child = child->rightSibling;
            
            case_stmt_set_expr(expr_pop());
            
            Visit(child);

            case_stmt_set_stmt(stmt_pop());
            break;
        case DEFAULT_STMT:
            new_default_stmt();

            Visit(child);

            default_stmt_set_stmt(stmt_pop());
            break;
        case COMPOUND_STMT:
            new_compound_stmt();
            new_env();
            
            while (child && child->type == DECLARATION)
            {
                Visit(child);
                child = child->rightSibling;
            }
            while (child)
            {
                Visit(child);
                child = child->rightSibling;
                compound_stmt_add(stmt_pop());
            }
            
            compound_stmt_set_env(env_pop());
            break;
        case EXPRESSION_STMT:
            new_expr_stmt();

            if (child)
            {
                Visit(child);

                expr_stmt_set_expr(expr_pop());
            }
            break;
        case SELECTION_STMT:
            break;
        case IF_STMT:
        case IF_ELSE_STMT:
            new_if_stmt();

            Visit(child);
            child = child->rightSibling;

            if_stmt_set_expr(expr_pop());

            Visit(child);
            child = child->rightSibling;

            if_stmt_set_if_block(stmt_pop());

            if (child)
            {
                Visit(child);

                if_stmt_set_else_block(stmt_pop());
            }
            break;
        case SWITCH_STMT:
            new_switch_stmt();

            Visit(child);
            child = child->rightSibling;

            switch_stmt_set_expr(expr_pop());

            Visit(child);

            switch_stmt_set_stmt(stmt_pop());
            break;
        case ITERATION_STMT:
            break;
        case WHILE_STMT:
            new_while_stmt();

            Visit(child);
            child = child->rightSibling;

            while_stmt_set_expr(expr_pop());

            Visit(child);

            while_stmt_set_stmt(stmt_pop());
            break;
        case DO_WHILE_STMT:
            new_do_while_stmt();

            Visit(child);
            child = child->rightSibling;

            do_while_stmt_set_stmt(stmt_pop());

            Visit(child);

            do_while_stmt_set_expr(expr_pop());
            break;
        case FOR_STMT:
            new_for_stmt();

            if (child->type != EXPR)
            {
                Visit(child);
                for_stmt_set_init_expr(expr_pop());
            }
            child = child->rightSibling;

            if (child->type != EXPR)
            {
                Visit(child);
                for_stmt_set_cond_expr(expr_pop());
            }
            child = child->rightSibling;

            if (child->type != EXPR)
            {
                Visit(child);
                for_stmt_set_tail_expr(expr_pop());
            }
            child = child->rightSibling;

            Visit(child);
            for_stmt_set_stmt(stmt_pop());
            break;
        case JUMP_STMT:
            break;
        case GOTO_STMT:
            new_goto_stmt();
            goto_stmt_set_label(child->token.text);
            break;
        case CONTINUE_STMT:
            new_continue_stmt();
            break;
        case BREAK_STMT:
            new_break_stmt();
            break;
        case RETURN_STMT:
            new_return_stmt();

            if (child)
            {
                Visit(child);
                return_stmt_set_expr(expr_pop());
            }
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
    x64_DWORD   = 4,
    x64_QWORD   = 8,
};

x64DataType SizeToX64DataType(size_t size)
{
    ASSERT(size == 1 || size == 2 || size == 4 || size == 8);
    return (x64DataType)size;
}

std::pair<x64DataType, size_t> SizeToX64(size_t size, size_t count)
{
    ASSERT(size > 0 && count > 0);
    if (size == 1 || size == 2 || size == 4 || size == 8)
        return {SizeToX64DataType(size), 1};
    else
        return {x64_BYTE, size * count};
}

const char * X64DataTypeToString(x64DataType dt)
{
    const char * s = nullptr;
    switch (dt)
    {
        case x64_BYTE:   s = "BYTE";    break;
        case x64_WORD:   s = "WORD";    break;
        case x64_DWORD:  s = "DWORD";   break;
        case x64_QWORD:  s = "QWORD";   break;
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
    StringRef name;
    size_t count;
    bool needExport;
};

std::string & operator += (std::string & s, const StringRef & sr)
{
    s.append(sr.begin(), sr.end());
    return s;
}

// void     -> *
// char     -> BYTE
// int      -> BYTR/WORD/DWORD/QWORD
// pointer  -> QWORD
// struct   -> QWORD
// array    -> ??
// function -> PROC
class ASMFile
{
public:
    void AddCSymbol(Symbol * sym)
    {
        if (sym->storageType == Symbol::NONE)
            return;

        if (sym->objectType->getClass() == T_CHAR  ||
            sym->objectType->getClass() == T_INT   ||
            sym->objectType->getClass() == T_FLOAT ||
            sym->objectType->getClass() == T_POINTER ||
            sym->objectType->getClass() == T_ENUM)
        {
            x64Variable var =
            {
                SizeToX64DataType(sym->objectType->getSize()),
                x64_NO_INIT,
                sym->name,
                1,
                sym->needExport
            };
            bssSegment.push_back(var);
        }
        else if (sym->objectType->getClass() == T_STRUCT ||
                 sym->objectType->getClass() == T_UNION)
        {
            x64Variable var =
            {
                x64_BYTE,
                x64_NO_INIT,
                sym->name,
                0,
                sym->needExport
            };
            std::tie(var.dataType, var.count) = SizeToX64(sym->objectType->getSize(), 1);
            bssSegment.push_back(var);
        }
        else if (sym->objectType->getClass() == T_ARRAY)
        {
            x64Variable var =
            {
                x64_BYTE,
                x64_NO_INIT,
                sym->name,
                0,
                sym->needExport
            };
            std::tie(var.dataType, var.count) = SizeToX64(sym->objectType->getSize(), 1);
            bssSegment.push_back(var);
        }
        else
        {
            //T_NONE,
            //T_VOID,
            //T_TAG,
            //T_ENUM,
            //T_ENUM_CONST,
            //T_FUNCTION,
            //T_TYPEDEF,
            //T_LABEL
        }
    }

    void BeginProc();
    void EndProc();

    std::string Generate() const
    {
        std::string out;

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
            out += var.name;
            out += ' ';
            out += X64DataTypeToString(var.dataType);
            out += ' ';
            out += IntegerToASM(var.count);
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

void GenerateX64ASM(Ast * ast)
{
    reset();
    Visit(ast);
    verify();

    ASMFile asmFile;

    Environment * globalEnv = g_Envs[0];
    for (auto symbol : globalEnv->allSymbols())
    {
        asmFile.AddCSymbol(symbol);
    }
    std::cout << asmFile.Generate() << std::endl;
}
