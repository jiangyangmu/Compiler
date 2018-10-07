#include <iostream>

#include "parser_api.h"

#include "../symbol/symbol.h"
#include "../symbol/env.h"
#include "../ir/expr.h"
#include "../ir/stmt.h"
#include "../logging/logging.h"


#define TOKEN(s) PRODUCTION(TokenFromString(s))

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

// clang-format off
int tmain(int argc, char * argv[])
{
    GM_BEGIN(G);
    GM_ADD(G, translation_unit);
    GM_ADD(G, declaration_or_function_definition);
    GM_ADD(G, declaration);

    GM_ADD(G, declaration_specifiers);
    GM_ADD(G, storage_class_specifier);
    GM_ADD(G, type_specifier);
    GM_ADD(G, type_qualifier);

    GM_ADD(G, struct_or_union_specifier);
    GM_ADD(G, struct_declaration);
    GM_ADD(G, struct_declarator);

    GM_ADD(G, declarator);
    GM_ADD(G, declarator_recursive);
    GM_ADD(G, direct_declarator);
    GM_ADD(G, direct_declarator_tail);

    GM_ADD(G, parameter_list);
    GM_ADD(G, parameter_declaration);

    GM_ADD(G, pointer);

    GM_ADD(G, statement);
    GM_ADD(G, compound_stmt);
    GM_ADD(G, labeled_stmt);
    GM_ADD(G, expression_stmt);
    GM_ADD(G, selection_stmt);

    GM_ADD(G, expression);
    GM_ADD(G, comma_expr);
    GM_ADD(G, add_expr);
    GM_ADD(G, primary_expr);

    translation_unit =
                                  GM_CODE({ new_env(); }) &
          declaration_or_function_definition & *(declaration_or_function_definition)
                                & GM_CODE({ verify(); })
        ;
    declaration_or_function_definition =
          declaration_specifiers &
          (
            declarator          & GM_CODE({ typ_dup2_merge();
                                            CHECK(!id_top().empty());
                                            new_sym(Symbol::Namespace::ID, id_pop(), typ_top2());
                                            env_add_sym(sym_pop()); }) &
            (                     GM_CODE({ del_typ(); env_push(typ_func_get_env()); }) &
              compound_stmt     & GM_CODE({ env_pop(); del_typ(); })
            |                     GM_CODE({ typ_pop2(); }) &
              *(
                TOKEN(",") &
                declarator      & GM_CODE({ typ_dup2_merge();
                                            CHECK(!id_top().empty());
                                            new_sym(Symbol::Namespace::ID, id_pop(), typ_pop2());
                                            env_add_sym(sym_pop()); })
              ) &
              TOKEN(";")        & GM_CODE({ del_typ(); })
            )
          | TOKEN(";")          & GM_CODE({ del_typ(); })
          )
        ;
    declaration =
          declaration_specifiers &
          ~(
            declarator          & GM_CODE({ typ_dup2_merge();
                                            CHECK(!id_top().empty());
                                            new_sym(Symbol::Namespace::ID, id_pop(), typ_pop2());
                                            env_add_sym(sym_pop()); }) &
            *(
                TOKEN(",") &
                declarator      & GM_CODE({ typ_dup2_merge();
                                            CHECK(!id_top().empty());
                                            new_sym(Symbol::Namespace::ID, id_pop(), typ_pop2());
                                            env_add_sym(sym_pop()); })
            )
          ) &
          TOKEN(";")            & GM_CODE({ del_typ(); })
        ;
    declaration_specifiers =
                                  GM_CODE({ new_typ_specifier(); new_typ_qualifier(); }) &
          (storage_class_specifier | type_specifier | type_qualifier) &
          *(storage_class_specifier | type_specifier | type_qualifier)
                                & GM_CODE({ new_typ(typ_pop_specifier(), typ_pop_qualifier()); })
        ;
    storage_class_specifier =
          TOKEN("typedef")      & GM_CODE({ })
        | TOKEN("extern")       & GM_CODE({ })
        | TOKEN("static")       & GM_CODE({ })
        | TOKEN("auto")         & GM_CODE({ })
        | TOKEN("register")     & GM_CODE({ })
        ;
    type_specifier =
          TOKEN("void")         & GM_CODE({ typ_specifier_add_token(Token::KW_VOID); })
        | TOKEN("char")         & GM_CODE({ typ_specifier_add_token(Token::KW_CHAR); })
        | TOKEN("int")          & GM_CODE({ typ_specifier_add_token(Token::KW_INT); })
        | TOKEN("float")        & GM_CODE({ typ_specifier_add_token(Token::KW_FLOAT); })
        | TOKEN("signed")       & GM_CODE({ typ_specifier_add_token(Token::KW_SIGNED); })
        | struct_or_union_specifier
        ;
    type_qualifier =
          TOKEN("const")        & GM_CODE({ typ_qualifier_add_token(Token::KW_CONST); })
        | TOKEN("volatile")     & GM_CODE({ typ_qualifier_add_token(Token::KW_VOLATILE); })
        ;
    struct_or_union_specifier =
          (TOKEN("struct") | TOKEN("union"))
                                & GM_CODE({ typ_specifier_add_token(GM_MATCHED_TOKEN(0).type); }) &
          ~(
            TOKEN("id")         & GM_CODE({ typ_specifier_struct_set_tag(GM_MATCHED_TOKEN(0).text);
                                            new_sym(Symbol::Namespace::TAG, GM_MATCHED_TOKEN(0).text, typ_specifier_get_struct());
                                            env_add_sym(sym_pop()); })
          ) &
          TOKEN("{") &
          struct_declaration &
          *(struct_declaration) &
          TOKEN("}")
        ;
    struct_declaration =
                                  GM_CODE({ new_typ_specifier(); new_typ_qualifier(); }) &
          (type_specifier | type_qualifier) &
          *(type_specifier | type_qualifier)
                                & GM_CODE({ new_typ(typ_pop_specifier(), typ_pop_qualifier()); }) &
          struct_declarator     & GM_CODE({ typ_dup2_merge(); typ_specifier_struct_add_member(id_pop(), typ_pop2()); }) &
          *(
            TOKEN(",") &
            struct_declarator   & GM_CODE({ typ_dup2_merge(); typ_specifier_struct_add_member(id_pop(), typ_pop2()); })
          )                     & GM_CODE({ del_typ(); }) &
          TOKEN(";")
        ;
    struct_declarator =
          declarator            & GM_CODE({ CHECK(!id_top().empty()); })
        ;
    declarator =
          declarator_recursive
        ;
    declarator_recursive =
          pointer               & GM_CODE({ new_id(""); }) &
          ~(
            direct_declarator   & GM_CODE({ typ_merge(); id_merge(); })
          )
        | direct_declarator
        ;
    direct_declarator =
          TOKEN("id")           & GM_CODE({ new_typ(); new_id(GM_MATCHED_TOKEN(0).text); }) &
          ~(
            direct_declarator_tail
                                & GM_CODE({ typ_merge(); })
          )
        | TOKEN("(")            &
          declarator_recursive  &
          TOKEN(")")            &
          ~(
            direct_declarator_tail
                                & GM_CODE({ typ_swap_merge(); })
          )
        | direct_declarator_tail & GM_CODE({ new_id(""); })
        ;
    direct_declarator_tail =
          (
            TOKEN("[")          &
            TOKEN("1")          & GM_CODE({ new_typ(new ArrayType(GM_MATCHED_TOKEN(0).ival), 0); }) &
            TOKEN("]")
          | TOKEN("(")          & GM_CODE({ new_typ(new FuncType(), 0); new_env(); typ_func_link_env(env_top()); }) &
            ~(parameter_list)   &
            TOKEN(")")          & GM_CODE({ del_env(); })
          ) &
          *(
            TOKEN("[")          &
            TOKEN("1")          & GM_CODE({ new_typ(new ArrayType(GM_MATCHED_TOKEN(0).ival), 0); }) &
            TOKEN("]")          & GM_CODE({ typ_swap_merge(); })
          | TOKEN("(")          & GM_CODE({ new_typ(new FuncType(), 0); new_env(); typ_func_link_env(env_top()); }) &
            ~(parameter_list)   & GM_CODE({ del_env(); typ_swap_merge(); }) &
            TOKEN(")")
          )
        ;
    parameter_list =
          parameter_declaration & GM_CODE({ typ_func_add_param(id_pop(), typ_pop()); }) &
          *(
            TOKEN(",") &
            (
              parameter_declaration & GM_CODE({ typ_func_add_param(id_pop(), typ_pop()); })
            | TOKEN("...")          & GM_CODE({ typ_func_set_varlist(); })
            )
          )
        ;
    parameter_declaration =
          declaration_specifiers & GM_CODE({ new_id(""); }) &
          ~(
            declarator          & GM_CODE({ typ_merge(); id_merge(); })
          )
        ;
    pointer =
          TOKEN("*")            & GM_CODE({ new_typ_qualifier(); }) &
          *(type_qualifier)     & GM_CODE({ new_typ(new PointerType(), typ_pop_qualifier()); }) &
          *(
             TOKEN("*")         & GM_CODE({ new_typ_qualifier(); }) &
             *(type_qualifier)  & GM_CODE({ new_typ(new PointerType(), typ_pop_qualifier()); typ_merge(); })
           )
        ;

    statement = 
          compound_stmt
        | expression_stmt
        | labeled_stmt
        | selection_stmt
        ;
    compound_stmt =
          TOKEN("{")            &
          *(declaration)        &
          *(statement)          &
          TOKEN("}")
        ;
    expression_stmt =
                                  GM_CODE({ stmt_push(StmtBuilder::EXPR(nullptr)); }) &
          ~(
            expression          & GM_CODE({ stmt_pop(); stmt_push(StmtBuilder::EXPR(expr_pop())); })
          ) &
          TOKEN(";")
        ;
    labeled_stmt =
          TOKEN("id")           & GM_CODE({ new_sym(Symbol::LABEL, GM_MATCHED_TOKEN(0).text, nullptr);
                                            env_add_sym(sym_top()); }) &
          TOKEN(":") & statement & GM_CODE({ stmt_push(StmtBuilder::LABEL(sym_pop(), stmt_pop()));  }) & TOKEN(";") /* TODO: remove this after FOLLOW computation is fixed. */
        | TOKEN("case") & TOKEN("1") & TOKEN(":") & statement & TOKEN(";") /* TODO: remove this after FOLLOW computation is fixed. */
        | TOKEN("default") & TOKEN(":") & statement & TOKEN(";") /* TODO: remove this after FOLLOW computation is fixed. */
        ;
    selection_stmt =
          TOKEN("if")           &
          TOKEN("(")            &
          expression            &
          TOKEN(")")            &
          statement             & GM_CODE({ stmt_push(StmtBuilder::IF(expr_pop(), stmt_pop(), nullptr)); })// & TOKEN(";") /* TODO: remove this after FOLLOW computation is fixed. */
        ;

    expression =
          add_expr /* TODO: change to comma_expr */
        ;
    comma_expr =
          add_expr              &
          *(
            TOKEN(",")          &
            add_expr
          )
        ;
    add_expr =
          primary_expr          &
          *(
            TOKEN("+")          &
            primary_expr        & GM_CODE({ expr_push(ExprTreeBuilder::ADD(expr_pop(), expr_pop())); })
          )
        ;
    primary_expr =
          TOKEN("id")           & GM_CODE({ expr_push(ExprTreeBuilder::NODE(env_find_sym(GM_MATCHED_TOKEN(0).text))); })
        | TOKEN("1")
        | TOKEN("(") & expression & TOKEN(")")
        ;

    GM_END(G);

    //SourceScanner scanner("extern const signed int * const * volatile i1, * const i2[46];");
    //SourceScanner scanner("struct Fish { int i; struct Body { float size; } body; } fish[12];");
    //SourceScanner scanner("int (*p)[23], *q[23];");
    //SourceScanner scanner("int func(char , int (*)(struct S {int i; } s, ...));");
    //SourceScanner scanner("struct Functor { void (*fp)(); } functor;");
    //SourceScanner scanner("struct A { struct B { int i; } b; int j,k; } a;");
    //SourceScanner scanner("int func() { 1, 2, (3,4,5); };");
    SourceScanner scanner("int (*(*fp)(int a))();");
    //SourceScanner scanner("int func(int a1, int a2) { int a, b, c; 1, 2, (3,4,5), b + c; };");
    //SourceScanner scanner("int func(int a1, int a2) { int a, b, c; if (a) a + (b + c); }");
    Tokenizer tokenizer;
    tokenizer.compile(scanner);
    TokenIterator tokens = tokenizer.getIterator();
    GM_RUN(G, tokens);

    return 0;
}
// clang-format on
