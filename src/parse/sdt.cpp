#include <iostream>

#include "../type/type.h"
#include "../../logging/logging.h"
#include "parser_api.h"

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
    std::cout << "delete: " << g_Types.back()->toString() << std::endl;
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

void new_id(StringRef id) {
    g_Ids.push_back(id);
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

// void new_sym() {
//    g_SymbolBuilders.push_back({});
//}

void verify() {
    CHECK(g_SpecifierBuilders.empty());
    CHECK(g_QualifierBuilders.empty());
    CHECK(g_Ids.empty());
    // CHECK(g_Types.size() == 1);

    int i = 0;
    for (Type * type : g_Types)
    {
        CHECK(type != nullptr);
        std::cout << "type[" << i++ << "]: " << type->toString() << std::endl;
    }
}

// clang-format off
int main(int argc, char * argv[])
{
    GM_BEGIN(G);
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

    declaration =
          declaration_specifiers &
          ~(
             declarator         & GM_CODE({ typ_dup2_merge(); del_id(); }) &
             *(
                TOKEN(",")      &
                declarator      & GM_CODE({ typ_dup2_merge(); del_id(); })
              )
           )                    & GM_CODE({ del_typ(); }) &
          TOKEN(";")
        ;
    declaration_specifiers =
          GM_CODE({ new_typ_specifier(); new_typ_qualifier(); }) &
          (storage_class_specifier | type_specifier | type_qualifier) &
          *(storage_class_specifier | type_specifier | type_qualifier) &
          GM_CODE({ new_typ(typ_pop_specifier(), typ_pop_qualifier()); })
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
              TOKEN("id")       & GM_CODE({ std::cout << "new struct " << GM_MATCHED_TOKEN(0).text << std::endl; })
           ) &
          TOKEN("{") &
          struct_declaration &
          *(struct_declaration) &
          TOKEN("}")
        ;
    struct_declaration =
          GM_CODE({ new_typ_specifier(); new_typ_qualifier(); }) &
          (type_specifier | type_qualifier) &
          *(type_specifier | type_qualifier) &
          GM_CODE({ new_typ(typ_pop_specifier(), typ_pop_qualifier()); }) &
          struct_declarator     & GM_CODE({ typ_dup2_merge(); typ_specifier_struct_add_member(id_pop(), typ_pop2()); }) &
          *(
             TOKEN(",") &
             struct_declarator  & GM_CODE({ typ_dup2_merge(); typ_specifier_struct_add_member(id_pop(), typ_pop2()); })
           )                    & GM_CODE({ del_typ(); }) &
          TOKEN(";")
        ;
    struct_declarator =
          declarator
        ;
    declarator =
          declarator_recursive
        ;
    declarator_recursive =
                                  GM_CODE({ new_typ(); }) &
          ~(
             pointer            & GM_CODE({ typ_merge(); })
           ) &
          direct_declarator     & GM_CODE({ typ_merge(); })
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
        ;
    direct_declarator_tail =
          (
            TOKEN("[")          &
            TOKEN("1")          & GM_CODE({ new_typ(new ArrayType(GM_MATCHED_TOKEN(0).ival), 0); }) &
            TOKEN("]")
          | TOKEN("(")          & GM_CODE({ new_typ(new FuncType(), 0); }) &
            ~(parameter_list)   &
            TOKEN(")")
          ) &
          *(
            TOKEN("[")          &
            TOKEN("1")          & GM_CODE({ new_typ(new ArrayType(GM_MATCHED_TOKEN(0).ival), 0); }) &
            TOKEN("]")          & GM_CODE({ typ_swap_merge(); })
          | TOKEN("(")          & GM_CODE({ new_typ(new FuncType(), 0); }) &
            ~(parameter_list)   & GM_CODE({ typ_swap_merge(); }) &
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
          declaration_specifiers &
          declarator            & GM_CODE({ typ_merge(); })
        ;
    pointer =
          TOKEN("*")            & GM_CODE({ new_typ_qualifier(); }) &
          *(type_qualifier)     & GM_CODE({ new_typ(new PointerType(), typ_pop_qualifier()); }) &
          *(
             TOKEN("*")         & GM_CODE({ new_typ_qualifier(); }) &
             *(type_qualifier)  & GM_CODE({ new_typ(new PointerType(), typ_pop_qualifier()); typ_merge(); })
           )
        ;

    GM_END(G);

    //SourceSanner scanner("extern const signed int * const * volatile i1, * const i2[46];");
    //SourceSanner scanner("struct Fish { int i; struct Body { float size; } body; } fish[12];");
    //SourceSanner scanner("int (*p)[23], *q[23];");
    //SourceSanner scanner("int func(char a, int (*b)(struct S {int i; } s, ...));");
    SourceSanner scanner("struct Functor { void (*fp)(); } functor;");
    Tokenizer tokenizer;
    tokenizer.compile(scanner);
    TokenIterator tokens = tokenizer.getIterator();
    GM_RUN(G, tokens);

    verify();

    return 0;
}
// clang-format on
