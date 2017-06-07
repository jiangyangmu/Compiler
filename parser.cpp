// #include <iostream>
#include <map>
#include <string>
#include <cstdint>
// using namespace std;

#include "common.h"
// #include "convert.h"
// #include "env.h"
#include "lexer.h"
#include "parser.h"

// TODO: check if symbol is typedef-name
bool first(uint64_t types, Token t, Environment *env)
{
    static std::map<TokenType, uint64_t> m;
    if (m.empty())
    {
        m[LP] = SN_STATEMENT_LIST | SN_POSTFIX_EXPRESSION |
                SN_UNARY_EXPRESSION | SN_INITIALIZER_LIST |
                SN_DIRECT_ABSTRACT_DECLARATOR | SN_ABSTRACT_DECLARATOR;
        m[LSB] = SN_DIRECT_ABSTRACT_DECLARATOR | SN_ABSTRACT_DECLARATOR;
        m[CONST_CHAR] = m[CONST_INT] = m[CONST_FLOAT] = m[STRING] =
            SN_STATEMENT_LIST | SN_POSTFIX_EXPRESSION | SN_UNARY_EXPRESSION |
            SN_INITIALIZER_LIST;
        m[OP_INC] = m[OP_DEC] = m[SIZEOF] = m[BIT_AND] = m[OP_ADD] = m[OP_SUB] =
            m[BIT_NOT] = m[BOOL_NOT] =
                SN_STATEMENT_LIST | SN_UNARY_EXPRESSION | SN_INITIALIZER_LIST;
        m[OP_MUL] = SN_STATEMENT_LIST | SN_POINTER | SN_UNARY_EXPRESSION |
                    SN_INITIALIZER_LIST | SN_ABSTRACT_DECLARATOR;
        m[BLK_BEGIN] =
            SN_STATEMENT_LIST | SN_COMPOUND_STATEMENT | SN_INITIALIZER_LIST;
        m[TYPEDEF] = m[EXTERN] = m[STATIC] = m[AUTO] = m[REGISTER] =
            SN_STORAGE_SPECIFIER | SN_DECLARATION_LIST | SN_DECLARATION |
            SN_DECLARATION_SPECIFIERS | SN_PARAMETER_TYPE_LIST;
        m[CONST] = m[VOLATILE] =
            SN_TYPE_NAME | SN_TYPE_QUALIFIER | SN_DECLARATION_LIST |
            SN_DECLARATION | SN_DECLARATION_SPECIFIERS | SN_PARAMETER_TYPE_LIST;
        m[TYPE_VOID] = m[TYPE_CHAR] = m[TYPE_SHORT] = m[TYPE_INT] =
            m[TYPE_LONG] = m[TYPE_FLOAT] = m[TYPE_DOUBLE] = m[SIGNED] =
                m[UNSIGNED] = m[TYPE_STRUCT] = m[TYPE_UNION] = m[TYPE_ENUM] =
                    SN_TYPE_NAME | SN_TYPE_SPECIFIER | SN_DECLARATION_LIST |
                    SN_DECLARATION | SN_DECLARATION_SPECIFIERS |
                    SN_PARAMETER_TYPE_LIST;
        m[SYMBOL] = SN_STATEMENT_LIST | SN_POSTFIX_EXPRESSION |
                    SN_UNARY_EXPRESSION | SN_TYPE_NAME | SN_TYPE_SPECIFIER |
                    SN_DECLARATION_LIST | SN_DECLARATION |
                    SN_DECLARATION_SPECIFIERS | SN_INITIALIZER_LIST |
                    SN_PARAMETER_TYPE_LIST;
        m[CASE] = m[DEFAULT] = m[IF] = m[SWITCH] = m[WHILE] = m[DO] = m[FOR] =
            m[GOTO] = m[CONTINUE] = m[BREAK] = m[RETURN] = m[STMT_END] =
                SN_STATEMENT_LIST;
    }
    auto kv = m.find(t.type);
    if (kv == m.end())
        return false;
    else if (t.type == SYMBOL &&
             (types & (SN_TYPE_SPECIFIER | SN_TYPE_NAME | SN_DECLARATION_LIST |
                       SN_DECLARATION | SN_DECLARATION_SPECIFIERS |
                       SN_PARAMETER_TYPE_LIST)) != 0)
        return false; //env->recursiveFindTypename(t.symbol);
    else
        return (kv->second & types) != 0;
}

bool follow(uint64_t types, TokenType t)
{
    static std::map<TokenType, uint64_t> m;
    if (m.empty())
    {
        m[RP] = SN_TYPE_NAME | SN_PARAMETER_TYPE_LIST;
        m[OP_COMMA] = SN_PARAMETER_DECLARATION;
    }
    auto kv = m.find(t);
    // if (kv == m.end())
    //     SyntaxError("FOLLOW: type not implemented.");
    return kv != m.end() && (kv->second & types) != 0;
}

sn_translation_unit *sn_translation_unit::parse(Lexer &lex, Environment *env)
{
    sn_translation_unit *t = new sn_translation_unit();
    t->ed = sn_external_declaration::parse(lex, env);
    t->left = nullptr;
    if (lex.hasNext())
    {
        sn_translation_unit *t2 = parse(lex, env);
        t2->left = t;
        t = t2;
    }
    return t;
}
sn_external_declaration *sn_external_declaration::parse(Lexer &lex,
                                                        Environment *env)
{
    sn_external_declaration *ed = new sn_external_declaration();

    sn_declaration_specifiers *s = sn_declaration_specifiers::parse(lex, env);
    sn_declarator *d = sn_declarator::parse(lex, env);
    if (first(SN_COMPOUND_STATEMENT | SN_DECLARATION_LIST, lex.peakNext(), env))
    {
        ed->branch = FUNCDEF;
        ed->data.fd = sn_function_definition::parse(lex, env, s, d);
    }
    else
    {
        ed->branch = DECL;
        ed->data.d = sn_declaration::parse(lex, env, s, d);
    }
    return ed;
}
sn_function_definition *sn_function_definition::parse(
    Lexer &lex, Environment *env, sn_declaration_specifiers *s,
    sn_declarator *d)
{
    sn_function_definition *fd = new sn_function_definition();
    fd->s = s;
    fd->d = d;
    fd->dl = nullptr;
    if (first(SN_DECLARATION_LIST, lex.peakNext(), env))
        fd->dl = sn_declaration_list::parse(lex, env);
    fd->cs = sn_compound_statement::parse(lex, env);
    return fd;
}
sn_declaration *sn_declaration::parse(Lexer &lex, Environment *env)
{
    sn_declaration *dn = new sn_declaration();
    dn->s = sn_declaration_specifiers::parse(lex, env);
    dn->idl = nullptr;
    if (first(SN_INITIALIZER_LIST, lex.peakNext(), env))
        dn->idl = sn_init_declarator_list::parse(lex, env);
    EXPECT(STMT_END);
    return dn;
}
sn_declaration *sn_declaration::parse(Lexer &lex, Environment *env,
                                      sn_declaration_specifiers *s,
                                      sn_declarator *d)
{
    sn_declaration *dn = new sn_declaration();

    if (s == nullptr)
        SyntaxError("Declaration: missing specifiers.");
    dn->s = s;

    dn->idl = nullptr;
    if (d != nullptr)
    {
        sn_initializer *i = nullptr;
        if (lex.peakNext().type == ASSIGN)
        {
            lex.getNext();
            i = sn_initializer::parse(lex, env);
        }
        sn_init_declarator *id = new sn_init_declarator(d, i);
        dn->idl = sn_init_declarator_list::parse(lex, env, id);
    }
    EXPECT(STMT_END);
    return dn;
}
sn_declaration_list *sn_declaration_list::parse(Lexer &lex, Environment *env)
{
    sn_declaration_list *dl = new sn_declaration_list();
    dl->d = sn_declaration::parse(lex, env);
    dl->left = nullptr;
    if (first(SN_DECLARATION, lex.peakNext(), env))
    {
        sn_declaration_list *dl2 = parse(lex, env);
        dl2->left = dl;
        dl = dl2;
    }
    return dl;
}

// Declaration
sn_init_declarator *sn_init_declarator::parse(Lexer &lex, Environment *env)
{
    sn_init_declarator *id = new sn_init_declarator();
    id->d = sn_declarator::parse(lex, env);
    id->i = nullptr;
    if (lex.peakNext().type == ASSIGN)
    {
        lex.getNext();
        id->i = sn_initializer::parse(lex, env);
    }
    return id;
}
sn_init_declarator_list *sn_init_declarator_list::parse(Lexer &lex,
                                                        Environment *env,
                                                        sn_init_declarator *id)
{
    sn_init_declarator_list *idl = new sn_init_declarator_list();
    if (id != nullptr)
        idl->id = id;
    else
        idl->id = sn_init_declarator::parse(lex, env);
    idl->left = nullptr;
    if (lex.peakNext().type == OP_COMMA)
    {
        lex.getNext();
        sn_init_declarator_list *idl2 = parse(lex, env);
        idl2->left = idl;
        idl = idl2;
    }
    return idl;
}
sn_declarator *sn_declarator::parse(Lexer &lex, Environment *env)
{
    sn_declarator *d = new sn_declarator();
    d->p = nullptr;
    if (first(SN_POINTER, lex.peakNext(), env))
        d->p = sn_pointer::parse(lex, env);
    d->dd = sn_direct_declarator::parse(lex, env);
    return d;
}
sn_direct_declarator *sn_direct_declarator::parse(Lexer &lex, Environment *env,
                                                  sn_direct_declarator *left)
{
    sn_direct_declarator *dd = new sn_direct_declarator();
    dd->left = left;

    if (left == nullptr)
    {
        switch (lex.peakNext().type)
        {
            case SYMBOL:
                dd->branch = ID;
                dd->data.id = sn_identifier::parse(lex, env);
                break;
            case LP:
                EXPECT(LP);
                dd->branch = DECLARATOR;
                dd->data.d = sn_declarator::parse(lex, env);
                EXPECT(RP);
                break;
            default:
                SyntaxErrorDebug("Direct Declarator: unexpected token.");
                break;
        }
    }
    else
    {
        switch (lex.peakNext().type)
        {
            case LSB:
                dd->branch = ARRAY;
                EXPECT(LSB);
                dd->data.arr = sn_const_expression::parse(lex, env);
                EXPECT(RSB);
                break;
            case LP:
                EXPECT(LP);
                if (first(SN_PARAMETER_TYPE_LIST, lex.peakNext(), env))
                {
                    dd->branch = PARAM_LIST;
                    dd->data.ptlist = sn_parameter_type_list::parse(lex, env);
                }
                else if (first(SN_IDENTIFIER_LIST, lex.peakNext(), env))
                {
                    dd->branch = ID_LIST;
                    dd->data.idlist = sn_identifier_list::parse(lex, env);
                }
                else if (lex.peakNext().type == RP)
                {
                    dd->branch = ID_LIST;
                    dd->data.idlist = nullptr;
                }
                else
                    SyntaxError("unexpected token");
                EXPECT(RP);
                break;
            default: SyntaxError("unexpected token"); break;
        }
    }

    if (lex.peakNext().type == LSB || lex.peakNext().type == LP)
        dd = sn_direct_declarator::parse(lex, env, dd);
    return dd;
}
sn_abstract_declarator *sn_abstract_declarator::parse(Lexer &lex,
                                                      Environment *env)
{
    sn_abstract_declarator *d = new sn_abstract_declarator();
    d->p = nullptr;
    d->dad = nullptr;
    if (first(SN_POINTER, lex.peakNext(), env))
    {
        d->p = sn_pointer::parse(lex, env);
        if (first(SN_DIRECT_ABSTRACT_DECLARATOR, lex.peakNext(), env))
            d->dad = sn_direct_abstract_declarator::parse(lex, env);
    }
    else
        d->dad = sn_direct_abstract_declarator::parse(lex, env);
    return d;
}
sn_direct_abstract_declarator *sn_direct_abstract_declarator::parse(
    Lexer &lex, Environment *env, sn_direct_abstract_declarator *left)
{
    sn_direct_abstract_declarator *dad = new sn_direct_abstract_declarator();
    dad->left = left;

    switch (lex.peakNext().type)
    {
        case LP:
            EXPECT(LP);
            if (first(SN_ABSTRACT_DECLARATOR, lex.peakNext(), env))
            {
                dad->branch = ABST_DECL;
                dad->data.ad = sn_abstract_declarator::parse(lex, env);
            }
            else
            {
                dad->branch = PARAM_LIST;
                dad->data.ptlist =
                    lex.peakNext().type == RP
                        ? nullptr
                        : sn_parameter_type_list::parse(lex, env);
            }
            EXPECT(RP);
        case LSB:
            dad->branch = ARRAY;
            EXPECT(LSB);
            dad->data.arr = sn_const_expression::parse(lex, env);
            EXPECT(RSB);
            break;
        default:
            SyntaxError("Direct Abstract Declarator: unexpected token.");
            break;
    }

    if (first(SN_DIRECT_ABSTRACT_DECLARATOR, lex.peakNext(), env))
        dad = sn_direct_abstract_declarator::parse(lex, env, dad);
    return dad;
}
sn_initializer *sn_initializer::parse(Lexer &lex, Environment *env)
{
    sn_initializer *i = new sn_initializer();
    i->e = nullptr;
    i->il = nullptr;
    if (lex.peakNext().type == BLK_BEGIN)
    {
        EXPECT(BLK_BEGIN);
        i->il = sn_initializer_list::parse(lex, env);
        if (lex.peakNext().type == OP_COMMA)
            lex.getNext();
        EXPECT(BLK_END);
    }
    else
    {
        i->e = sn_assign_expression::parse(lex, env);
    }
    return i;
}
sn_initializer_list *sn_initializer_list::parse(Lexer &lex, Environment *env)
{
    sn_initializer_list *il = new sn_initializer_list();
    il->i = sn_initializer::parse(lex, env);
    il->left = nullptr;
    if (lex.peakNext().type == OP_COMMA)
    {
        lex.getNext();
        sn_initializer_list *il2 = new sn_initializer_list();
        il2->left = il;
        il = il2;
    }
    return il;
}
sn_parameter_type_list *sn_parameter_type_list::parse(Lexer &lex,
                                                      Environment *env)
{
    sn_parameter_type_list *ptl = new sn_parameter_type_list();
    ptl->pl = sn_parameter_list::parse(lex, env);
    ptl->varlist = false;
    if (lex.peakNext().type == OP_COMMA)
    {
        EXPECT(OP_COMMA);
        EXPECT(VAR_PARAM);
        ptl->varlist = true;
    }
    return ptl;
}
sn_parameter_list *sn_parameter_list::parse(Lexer &lex, Environment *env)
{
    sn_parameter_list *pl = new sn_parameter_list();
    pl->pd = sn_parameter_declaration::parse(lex, env);
    pl->left = nullptr;
    if (lex.peakNext().type == OP_COMMA && lex.peakNext(1).type != VAR_PARAM)
    {
        lex.getNext();
        sn_parameter_list *pl2 = parse(lex, env);
        pl2->left = pl;
        pl = pl2;
    }
    return pl;
}
int declarator_or_abstract_declarator(Lexer &lex, Environment *env)
{
    int status = 0;  // 1: declarator 2: abstract declarator 3: none
    for (size_t i = 0; status == 0; ++i)
    {
        switch (lex.peakNext(i).type)
        {
            case SYMBOL: status = 1; break;
            case LSB: status = 2; break;
            case LP:
                if (lex.peakNext(i + 1).type == RP)
                    status = 2;
                break;
            default:
                if (first(SN_DECLARATION_SPECIFIERS, lex.peakNext(i), env))
                    status = 2;
                else if (follow(SN_PARAMETER_DECLARATION, lex.peakNext(i).type))
                    status = 3;
                else
                    status = -1;
                break;
        }
    }
    return status;
}
sn_parameter_declaration *sn_parameter_declaration::parse(Lexer &lex,
                                                          Environment *env)
{
    sn_parameter_declaration *pd = new sn_parameter_declaration();
    pd->s = sn_declaration_specifiers::parse(lex, env);
    pd->data.ad = nullptr;
    switch (declarator_or_abstract_declarator(lex, env))
    {
        case 1:
            pd->branch = DECL;
            pd->data.d = sn_declarator::parse(lex, env);
            break;
        case 2:
            pd->branch = ABST_DECL;
            pd->data.ad = sn_abstract_declarator::parse(lex, env);
            break;
        case -1: SyntaxError("Parameter Declaration: unexpected token."); break;
        case 3:
        default: break;
    }
    return pd;
}

// specifier
sn_declaration_specifiers *sn_declaration_specifiers::parse(Lexer &lex,
                                                            Environment *env)
{
    sn_declaration_specifiers *s = new sn_declaration_specifiers();
    s->right = nullptr;
    if (first(SN_STORAGE_SPECIFIER, lex.peakNext(), env))
    {
        s->branch = STORAGE;
        s->data.ss = sn_storage_specifier::parse(lex, env);
    }
    else if (first(SN_TYPE_SPECIFIER, lex.peakNext(), env))
    {
        s->branch = TYPE_SPEC;
        s->data.ts = sn_type_specifier::parse(lex, env);
    }
    else
    {
        s->branch = TYPE_QUAL;
        s->data.tq = sn_type_qualifier::parse(lex, env);
    }

    if (first(SN_DECLARATION_SPECIFIERS, lex.peakNext(), env))
        s->right = parse(lex, env);
    return s;
}
sn_specifier_qualifier_list *sn_specifier_qualifier_list::parse(
    Lexer &lex, Environment *env)
{
    sn_specifier_qualifier_list *sql = new sn_specifier_qualifier_list();
    sql->right = nullptr;
    if (first(SN_TYPE_SPECIFIER, lex.peakNext(), env))
    {
        sql->branch = TYPE_SPEC;
        sql->data.ts = sn_type_specifier::parse(lex, env);
    }
    else
    {
        sql->branch = TYPE_QUAL;
        sql->data.tq = sn_type_qualifier::parse(lex, env);
    }

    if (first(SN_TYPE_SPECIFIER | SN_TYPE_QUALIFIER, lex.peakNext(), env))
        sql->right = parse(lex, env);
    return sql;
}
sn_storage_specifier *sn_storage_specifier::parse(Lexer &lex, Environment *env)
{
    sn_storage_specifier *ss = new sn_storage_specifier();
    switch (lex.peakNext().type)
    {
        case TYPEDEF:
        case EXTERN:
        case STATIC:
        case AUTO:
        case REGISTER: ss->t = lex.getNext().type; break;
        default: SyntaxError("Storage Specifier: unexpected token"); break;
    }
    return ss;
}
sn_type_qualifier *sn_type_qualifier::parse(Lexer &lex, Environment *env)
{
    sn_type_qualifier *tq = new sn_type_qualifier();
    switch (lex.peakNext().type)
    {
        case CONST:
        case VOLATILE: tq->t = lex.getNext().type; break;
        default: SyntaxError("Type Qualifier: unexpected token"); break;
    }
    return tq;
}
sn_type_specifier *sn_type_specifier::parse(Lexer &lex, Environment *env)
{
    sn_type_specifier *ts = new sn_type_specifier();
    switch (lex.peakNext().type)
    {
        case TYPE_VOID:
        case TYPE_CHAR:
        case TYPE_SHORT:
        case TYPE_INT:
        case TYPE_LONG:
        case TYPE_FLOAT:
        case TYPE_DOUBLE:
        case SIGNED:
        case UNSIGNED:
            ts->branch = TYPESPEC_SIMPLE;
            ts->data.t = lex.getNext().type;
            break;
        case TYPE_STRUCT:
        case TYPE_UNION:
            ts->branch = TYPESPEC_STRUCT_UNION;
            ts->data.sus = sn_struct_union_specifier::parse(lex, env);
            break;
        case TYPE_ENUM:
            ts->branch = TYPESPEC_ENUM;
            ts->data.es = sn_enum_specifier::parse(lex, env);
            break;
        case SYMBOL:
            ts->branch = TYPESPEC_TYPEDEF;
            ts->data.tn = sn_typedef_name::parse(lex, env);
            break;
        default: SyntaxError("Type Specifier: unexpected token"); break;
    }
    return ts;
}
sn_type_qualifier_list *sn_type_qualifier_list::parse(Lexer &lex,
                                                      Environment *env)
{
    sn_type_qualifier_list *tql = new sn_type_qualifier_list();
    tql->tq = sn_type_qualifier::parse(lex, env);
    tql->left = nullptr;
    if (first(SN_TYPE_QUALIFIER, lex.peakNext(), env))
    {
        sn_type_qualifier_list *tql2 = parse(lex, env);
        tql2->left = tql;
        tql = tql2;
    }
    return tql;
}
// struct specifier
sn_struct_union_specifier *sn_struct_union_specifier::parse(Lexer &lex,
                                                            Environment *env)
{
    sn_struct_union_specifier *sus = new sn_struct_union_specifier();
    sus->tag = nullptr;
    sus->sdl = nullptr;

    if (lex.peakNext().type == TYPE_STRUCT || lex.peakNext().type == TYPE_UNION)
        sus->t = lex.getNext().type;
    else
        SyntaxError("Struct: unexpected token");

    if (lex.peakNext().type != BLK_BEGIN)
        sus->tag = sn_identifier::parse(lex, env);

    if (lex.peakNext().type == BLK_BEGIN)
    {
        EXPECT(BLK_BEGIN);
        sus->sdl = sn_struct_declaration_list::parse(lex, env);
        EXPECT(BLK_END);
    }
    return sus;
}
sn_struct_declaration *sn_struct_declaration::parse(Lexer &lex,
                                                    Environment *env)
{
    sn_struct_declaration *sd = new sn_struct_declaration();
    sd->sql = sn_specifier_qualifier_list::parse(lex, env);
    sd->sdl = sn_struct_declarator_list::parse(lex, env);
    return sd;
}
sn_struct_declaration_list *sn_struct_declaration_list::parse(Lexer &lex,
                                                              Environment *env)
{
    sn_struct_declaration_list *sdl = new sn_struct_declaration_list();
    sdl->sd = sn_struct_declaration::parse(lex, env);
    sdl->left = nullptr;
    if (lex.peakNext().type == OP_COMMA)
    {
        lex.getNext();
        sn_struct_declaration_list *sdl2 = parse(lex, env);
        sdl2->left = sdl;
        sdl = sdl2;
    }
    return sdl;
}
sn_struct_declarator *sn_struct_declarator::parse(Lexer &lex, Environment *env)
{
    sn_struct_declarator *sd = new sn_struct_declarator();
    sd->d = nullptr;
    sd->bit_field = nullptr;

    if (lex.peakNext().type != OP_COLON)
        sd->d = sn_declarator::parse(lex, env);

    if (lex.peakNext().type == OP_COLON)
    {
        lex.getNext();
        sd->bit_field = sn_const_expression::parse(lex, env);
    }

    return sd;
}
sn_struct_declarator_list *sn_struct_declarator_list::parse(Lexer &lex,
                                                            Environment *env)
{
    sn_struct_declarator_list *sdl = new sn_struct_declarator_list();
    sdl->sd = sn_struct_declarator::parse(lex, env);
    sdl->left = nullptr;
    if (lex.peakNext().type == OP_COMMA)
    {
        lex.getNext();
        sn_struct_declarator_list *sdl2 = parse(lex, env);
        sdl2->left = sdl;
        sdl = sdl2;
    }
    return sdl;
}
// enum specifier
sn_enum_specifier *sn_enum_specifier::parse(Lexer &lex, Environment *env)
{
    sn_enum_specifier *es = new sn_enum_specifier();
    es->tag = nullptr;
    es->el = nullptr;

    EXPECT(TYPE_ENUM);

    if (lex.peakNext().type != BLK_BEGIN)
        es->tag = sn_identifier::parse(lex, env);

    if (lex.peakNext().type == BLK_BEGIN)
    {
        EXPECT(BLK_BEGIN);
        es->el = sn_enumerator_list::parse(lex, env);
        EXPECT(BLK_END);
    }
    return es;
}
sn_enumerator_list *sn_enumerator_list::parse(Lexer &lex, Environment *env)
{
    sn_enumerator_list *el = new sn_enumerator_list();
    el->e = sn_enumerator::parse(lex, env);
    el->left = nullptr;
    if (lex.peakNext().type == OP_COMMA)
    {
        lex.getNext();
        sn_enumerator_list *el2 = parse(lex, env);
        el2->left = el;
        el = el2;
    }
    return el;
}
sn_enumerator *sn_enumerator::parse(Lexer &lex, Environment *env)
{
    sn_enumerator *e = new sn_enumerator();
    e->ec = sn_enumeration_constant::parse(lex, env);
    e->value = nullptr;
    if (lex.peakNext().type == ASSIGN)
    {
        e->value = sn_const_expression::parse(lex, env);
    }
    return e;
}
sn_enumeration_constant *sn_enumeration_constant::parse(Lexer &lex,
                                                        Environment *env)
{
    sn_enumeration_constant *ec = new sn_enumeration_constant();
    ec->id = sn_identifier::parse(lex, env);
    return ec;
}

sn_type_name *sn_type_name::parse(Lexer &lex, Environment *env)
{
    sn_type_name *tn = new sn_type_name();
    tn->sql = sn_specifier_qualifier_list::parse(lex, env);
    tn->ad = nullptr;
    if (!follow(SN_TYPE_NAME, lex.peakNext().type))
        tn->ad = sn_abstract_declarator::parse(lex, env);
    return tn;
}
sn_pointer *sn_pointer::parse(Lexer &lex, Environment *env)
{
    sn_pointer *p = new sn_pointer();
    EXPECT(OP_MUL);
    p->tql = sn_type_qualifier_list::parse(lex, env);
    if (first(SN_POINTER, lex.peakNext(), env))
        p->right = parse(lex, env);
    return p;
}
sn_identifier *sn_identifier::parse(Lexer &lex, Environment *env)
{
    sn_identifier *id = new sn_identifier();
    id->id = EXPECT_GET(SYMBOL);
    return id;
}
sn_identifier_list *sn_identifier_list::parse(Lexer &lex, Environment *env)
{
    sn_identifier_list *il = new sn_identifier_list();
    il->id = sn_identifier::parse(lex, env);
    il->left = nullptr;
    if (lex.peakNext().type == OP_COMMA)
    {
        lex.getNext();
        sn_identifier_list *il2 = parse(lex, env);
        il2->left = il;
        il = il2;
    }
    return il;
}
sn_typedef_name *sn_typedef_name::parse(Lexer &lex, Environment *env)
{
    sn_typedef_name *tn = new sn_typedef_name();
    tn->i = sn_identifier::parse(lex, env);
    return tn;
}

// Statement
sn_statement *sn_statement::parse(Lexer &lex, Environment *env)
{
    sn_statement *node = nullptr;
    switch (lex.peakNext().type)
    {
        case SYMBOL:
            if (lex.peakNext(1).type == OP_COLON)
            {
                node = sn_label_statement::parse(lex, env);
            }
            else
            {
                node = sn_expression_statement::parse(lex, env);
            }
            break;
        case CASE:
        case DEFAULT: node = sn_label_statement::parse(lex, env); break;
        case BLK_BEGIN: node = sn_compound_statement::parse(lex, env); break;
        case IF:
        case SWITCH: node = sn_selection_statement::parse(lex, env); break;
        case WHILE:
        case DO:
        case FOR: node = sn_iteration_statement::parse(lex, env); break;
        case GOTO:
        case CONTINUE:
        case BREAK:
        case RETURN: node = sn_jump_statement::parse(lex, env); break;
        default:
            /* TODO: check FIRST(expr) */
            node = sn_expression_statement::parse(lex, env);
            break;
    }
    if (node == nullptr)
        SyntaxError("Statement: empty statement");
    return node;
}
sn_statement_list *sn_statement_list::parse(Lexer &lex, Environment *env)
{
    sn_statement_list *sl = new sn_statement_list();
    sl->s = sn_statement::parse(lex, env);
    if (lex.peakNext().type != BLK_END)
    {
        sn_statement_list *sl2 = parse(lex, env);
        sl2->left = sl;
        sl = sl2;
    }
    return sl;
}
sn_statement *sn_label_statement::parse(Lexer &lex, Environment *env)
{
    sn_label_statement *ls = new sn_label_statement();
    switch (lex.peakNext().type)
    {
        case SYMBOL:
            ls->branch = LABEL_STAT;
            ls->data.id = sn_identifier::parse(lex, env);
            break;
        case CASE:
            ls->branch = CASE_STAT;
            lex.getNext();
            ls->data.value = sn_const_expression::parse(lex, env);
            break;
        case DEFAULT:
            ls->branch = DEFAULT_STAT;
            lex.getNext();
            break;
        default: SyntaxError("Label Statement: unexpected token."); break;
    }
    EXPECT(OP_COLON);
    ls->stat = sn_statement::parse(lex, env);
    return ls;
}
sn_compound_statement *sn_compound_statement::parse(Lexer &lex,
                                                    Environment *env)
{
    sn_compound_statement *cs = new sn_compound_statement();
    cs->dl = nullptr;
    cs->sl = nullptr;

    EXPECT(BLK_BEGIN);
    if (first(SN_DECLARATION_LIST, lex.peakNext(), env))
        cs->dl = sn_declaration_list::parse(lex, env);
    if (first(SN_STATEMENT_LIST, lex.peakNext(), env))
        cs->sl = sn_statement_list::parse(lex, env);
    EXPECT(BLK_END);

    return cs;
}
sn_statement *sn_expression_statement::parse(Lexer &lex, Environment *env)
{
    sn_expression_statement *stmt = new sn_expression_statement();
    stmt->expr = nullptr;
    if (lex.peakNext().type != STMT_END)
        stmt->expr = sn_comma_expression::parse(lex, env);
    EXPECT(STMT_END);

    return stmt;
}
sn_statement *sn_selection_statement::parse(Lexer &lex, Environment *env)
{
    sn_selection_statement *stmt = new sn_selection_statement();

    switch (lex.peakNext().type)
    {
        case IF: stmt->branch = IF_STAT; break;
        case SWITCH: stmt->branch = SWITCH_STAT; break;
        default: SyntaxError("Selection Statement: unexpected token"); break;
    }
    lex.getNext();
    EXPECT(LP);
    stmt->expr = sn_comma_expression::parse(lex, env);
    EXPECT(RP);
    stmt->stmt = sn_statement::parse(lex, env);
    stmt->stmt2 = nullptr;
    if (stmt->branch == IF_STAT && lex.peakNext().type == ELSE)
    {
        lex.getNext();
        stmt->stmt2 = sn_statement::parse(lex, env);
    }

    return stmt;
}
sn_statement *sn_iteration_statement::parse(Lexer &lex, Environment *env)
{
    sn_iteration_statement *stmt = new sn_iteration_statement();
    stmt->expr = stmt->expr2 = stmt->expr3 = nullptr;
    stmt->stmt = nullptr;
    switch (lex.getNext().type)
    {
        case WHILE:
            stmt->type = WHILE_LOOP;
            EXPECT(LP);
            stmt->expr = sn_comma_expression::parse(lex, env);
            EXPECT(RP);
            stmt->stmt = sn_statement::parse(lex, env);
            break;
        case DO:
            stmt->type = DO_LOOP;
            stmt->stmt = sn_statement::parse(lex, env);
            EXPECT(WHILE);
            EXPECT(LP);
            stmt->expr = sn_comma_expression::parse(lex, env);
            EXPECT(RP);
            EXPECT(STMT_END);
            break;
        case FOR:
            stmt->type = FOR_LOOP;
            EXPECT(LP);
            if (lex.peakNext().type != STMT_END)
                stmt->expr = sn_comma_expression::parse(lex, env);
            EXPECT(STMT_END);
            if (lex.peakNext().type != STMT_END)
                stmt->expr2 = sn_comma_expression::parse(lex, env);
            EXPECT(STMT_END);
            if (lex.peakNext().type != RP)
                stmt->expr3 = sn_comma_expression::parse(lex, env);
            EXPECT(RP);
            stmt->stmt = sn_statement::parse(lex, env);
            break;
        default: SyntaxError("Iteration Statement: unexpected token"); break;
    }
    return stmt;
}
sn_statement *sn_jump_statement::parse(Lexer &lex, Environment *env)
{
    sn_jump_statement *stmt = new sn_jump_statement();
    stmt->data.expr = nullptr;
    switch (lex.getNext().type)
    {
        case GOTO:
            stmt->type = JMP_GOTO;
            stmt->data.id = sn_identifier::parse(lex, env);
            break;
        case CONTINUE: stmt->type = JMP_CONTINUE; break;
        case BREAK: stmt->type = JMP_BREAK; break;
        case RETURN:
            stmt->type = JMP_RETURN;
            if (lex.peakNext().type != STMT_END)
                stmt->data.expr = sn_comma_expression::parse(lex, env);
            break;
        default: SyntaxError("Jump Statement: unexpected token"); break;
    }
    EXPECT(STMT_END);
    return stmt;
}

// Expression
sn_expression *sn_comma_expression::parse(Lexer &lex, Environment *env)
{
    sn_expression *e = sn_assign_expression::parse(lex, env);
    if (lex.peakNext().type == OP_COMMA)
    {
        lex.getNext();
        sn_comma_expression *expr = new sn_comma_expression();
        expr->curr = e;
        expr->next = sn_comma_expression::parse(lex, env);
        if (expr->next)
            expr->type_ = expr->next->type();
        else
            expr->type_ = e->type();
        return expr;
    }
    else
        return e;
}
sn_expression *sn_assign_expression::parse(Lexer &lex, Environment *env)
{
    sn_expression *to = sn_cond_expression::parse(lex, env);
    if (to->nodeType() == SN_UNARY_EXPRESSION &&
        lex.peakNext().category == TOKEN_CATEGORY_ASSIGN_OPERATOR)
    {
        sn_assign_expression *expr = new sn_assign_expression();
        expr->to = to;
        expr->op = lex.getNext().type;
        expr->from = sn_assign_expression::parse(lex, env);
        assert(expr->to != nullptr);
        assert(expr->from != nullptr);
        // EXPECT_TYPE_WITH(target->type(), TOp_ASSIGN);
        // const Type *t =
        //     AssignmentConversion(expr->from->type(), expr->to->type());
        // if (!expr->from->type()->equal(*t))
        //     expr->from = new sn_cast_expression(t, expr->from);
        // expr->type_ = t;
        return expr;
    }
    else
        return to;
}

sn_expression *sn_cond_expression::parse(Lexer &lex, Environment *env)
{
    sn_expression *e = sn_or_expression::parse(lex, env);
    if (lex.peakNext().type == OP_QMARK)
    {
        sn_cond_expression *expr = new sn_cond_expression();
        expr->cond = e;
        lex.getNext();
        expr->left = sn_comma_expression::parse(lex, env);
        EXPECT(OP_COLON);
        expr->right = sn_cond_expression::parse(lex, env);
        // expr->type_ =
        //     CondResultConversion(expr->left->type(), expr->right->type());
        // if (!expr->left->type()->equal(*expr->type()))
        //     expr->left = new sn_cast_expression(expr->type(), expr->left);
        // if (!expr->right->type()->equal(*expr->type()))
        //     expr->right = new sn_cast_expression(expr->type(), expr->right);
        return expr;
    }
    else
    {
        return e;
    }
}
sn_expression *sn_or_expression::parse(Lexer &lex, Environment *env)
{
    sn_expression *e = sn_and_expression::parse(lex, env);
    if (lex.peakNext().type == BOOL_OR)
    {
        lex.getNext();
        sn_or_expression *expr = new sn_or_expression();
        expr->left = e;
        expr->right = sn_or_expression::parse(lex, env);
        // expr->type_ = TypeFactory::Primitive_Int();
        return expr;
    }
    else
        return e;
}
sn_expression *sn_and_expression::parse(Lexer &lex, Environment *env)
{
    sn_expression *e = sn_bitor_expression::parse(lex, env);
    if (lex.peakNext().type == BOOL_AND)
    {
        lex.getNext();
        sn_and_expression *expr = new sn_and_expression();
        expr->left = e;
        expr->right = sn_and_expression::parse(lex, env);
        // expr->type_ = TypeFactory::Primitive_Int();
        return expr;
    }
    else
        return e;
}
sn_expression *sn_bitor_expression::parse(Lexer &lex, Environment *env)
{
    sn_expression *e = sn_bitxor_expression::parse(lex, env);
    if (lex.peakNext().type == BIT_OR)
    {
        lex.getNext();
        sn_bitor_expression *expr = new sn_bitor_expression();
        expr->left = e;
        expr->right = sn_bitor_expression::parse(lex, env);
        // expr->type_ =
        //     UsualArithmeticConversion(expr->left->type(),
        //     expr->right->type());
        // if (!expr->left->type()->equal(*expr->type()))
        //     expr->left = new sn_cast_expression(expr->type(), expr->left);
        // if (!expr->right->type()->equal(*expr->type()))
        //     expr->right = new sn_cast_expression(expr->type(), expr->right);

        return expr;
    }
    else
        return e;
}
sn_expression *sn_bitxor_expression::parse(Lexer &lex, Environment *env)
{
    sn_expression *e = sn_bitand_expression::parse(lex, env);
    if (lex.peakNext().type == BIT_XOR)
    {
        lex.getNext();
        sn_bitxor_expression *expr = new sn_bitxor_expression();
        expr->left = e;
        expr->right = sn_bitxor_expression::parse(lex, env);
        // expr->type_ =
        //     UsualArithmeticConversion(expr->left->type(),
        //     expr->right->type());
        // if (!expr->left->type()->equal(*expr->type()))
        //     expr->left = new sn_cast_expression(expr->type(), expr->left);
        // if (!expr->right->type()->equal(*expr->type()))
        //     expr->right = new sn_cast_expression(expr->type(), expr->right);

        return expr;
    }
    else
        return e;
}
sn_expression *sn_bitand_expression::parse(Lexer &lex, Environment *env)
{
    sn_expression *e = sn_eq_expression::parse(lex, env);
    if (lex.peakNext().type == BIT_AND)
    {
        lex.getNext();
        sn_bitand_expression *expr = new sn_bitand_expression();
        expr->left = e;
        expr->right = sn_bitand_expression::parse(lex, env);
        // expr->type_ =
        //     UsualArithmeticConversion(expr->left->type(),
        //     expr->right->type());
        // if (!expr->left->type()->equal(*expr->type()))
        //     expr->left = new sn_cast_expression(expr->type(), expr->left);
        // if (!expr->right->type()->equal(*expr->type()))
        //     expr->right = new sn_cast_expression(expr->type(), expr->right);

        return expr;
    }
    else
        return e;
}
sn_expression *sn_eq_expression::parse(Lexer &lex, Environment *env)
{
    sn_expression *e = sn_rel_expression::parse(lex, env);
    if (lex.peakNext().type == REL_EQ || lex.peakNext().type == REL_NE)
    {
        sn_eq_expression *expr = new sn_eq_expression();
        expr->op = lex.getNext().type;
        expr->left = e;
        expr->right = sn_eq_expression::parse(lex, env);
        // expr->type_ = TypeFactory::Primitive_Int();
        // if (expr->left->type()->isArithmetic() &&
        //     expr->right->type()->isArithmetic())
        // {
        //     const Type *t = UsualArithmeticConversion(expr->left->type(),
        //                                                   expr->right->type());
        //     expr->left = new sn_cast_expression(t, expr->left);
        //     expr->right = new sn_cast_expression(t, expr->right);
        // }
        // else
        // {
        //     EXPECT_TYPE_IS(expr->left->type(), T_POINTER);
        //     EXPECT_TYPE_IS(expr->right->type(), T_POINTER);
        // }
        return expr;
    }
    else
        return e;
}
sn_expression *sn_rel_expression::parse(Lexer &lex, Environment *env)
{
    sn_expression *e = sn_shift_expression::parse(lex, env);
    if (lex.peakNext().type == REL_LT || lex.peakNext().type == REL_LE ||
        lex.peakNext().type == REL_GT || lex.peakNext().type == REL_GE)
    {
        sn_rel_expression *expr = new sn_rel_expression();
        expr->op = lex.getNext().type;
        expr->left = e;
        expr->right = sn_rel_expression::parse(lex, env);
        // expr->type_ = TypeFactory::Primitive_Int();
        // if (expr->left->type()->isArithmetic() &&
        //     expr->right->type()->isArithmetic())
        // {
        //     const Type *t = UsualArithmeticConversion(expr->left->type(),
        //                                                   expr->right->type());
        //     expr->left = new sn_cast_expression(t, expr->left);
        //     expr->right = new sn_cast_expression(t, expr->right);
        // }
        // else
        // {
        //     EXPECT_TYPE_IS(expr->left->type(), T_POINTER);
        //     EXPECT_TYPE_IS(expr->right->type(), T_POINTER);
        // }
        return expr;
    }
    else
        return e;
}
sn_expression *sn_shift_expression::parse(Lexer &lex, Environment *env)
{
    sn_expression *e = sn_add_expression::parse(lex, env);
    if (lex.peakNext().type == BIT_SLEFT || lex.peakNext().type == BIT_SRIGHT)
    {
        sn_shift_expression *expr = new sn_shift_expression();
        expr->op = lex.getNext().type;

        sn_expression *l = e;
        sn_expression *r = sn_shift_expression::parse(lex, env);
        // assert(l->type()->isIntegral());
        // assert(r->type()->isIntegral());

        // const Type *pl = IntegerPromotion(l->type());
        // const Type *pr = IntegerPromotion(r->type());

        expr->left = l;
        // if (!pl->equal(*l->type()))
        //     expr->left = new sn_cast_expression(pl, l);
        expr->right = r;
        // if (!pr->equal(*r->type()))
        //     expr->right = new sn_cast_expression(pr, r);

        // expr->type_ = pl;
        return expr;
    }
    else
        return e;
}
sn_expression *sn_add_expression::parse(Lexer &lex, Environment *env,
                                        sn_expression *left)
{
    if (left == nullptr)
        left = sn_mul_expression::parse(lex, env);
    if (lex.peakNext().type == OP_ADD || lex.peakNext().type == OP_SUB)
    {
        sn_add_expression *expr = new sn_add_expression();
        expr->op = lex.getNext().type;
        expr->left = left;
        expr->right = sn_mul_expression::parse(lex, env);

        // EXPECT_TYPE_WITH(expr->left->type(), TOp_ADD);
        // EXPECT_TYPE_WITH(expr->right->type(), TOp_ADD);

        // arithmetic type
        // pointer + int | int + pointer
        // pointer - int/pointer
        // const Type *l = expr->left->type();
        // const Type *r = expr->right->type();
        // do
        // {
        //     if (l->isArithmetic() && r->isArithmetic())
        //     {
        //         const Type *t = UsualArithmeticConversion(l, r);
        //         if (!t->equal(*l))
        //             expr->left = new sn_cast_expression(t, expr->left);
        //         if (!t->equal(*r))
        //             expr->right = new sn_cast_expression(t, expr->right);
        //         expr->type_ = t;
        //         break;
        //     }
        //     else if (expr->op == OP_ADD)
        //     {
        //         if (l->type() == T_POINTER && r->isIntegral())
        //         {
        //             expr->type_ = l;
        //             break;
        //         }
        //         else if (l->type() == T_POINTER && r->isIntegral())
        //         {
        //             expr->type_ = r;
        //             break;
        //         }
        //     }
        //     else if (expr->op == OP_SUB)
        //     {
        //         if (l->type() == T_POINTER)
        //         {
        //             if (r->isIntegral())
        //             {
        //                 expr->type_ = l;
        //                 break;
        //             }
        //             else if (r->type() ==
        //                      T_POINTER)  // TODO: type: use ptrdiff_t
        //             {
        //                 expr->type_ = TypeFactory::Primitive_Int();
        //                 break;
        //             }
        //         }
        //     }
        //     SyntaxErrorEx("sn_add_expression: invalid combination");
        // } while (false);

        return parse(lex, env, expr);
    }
    else
        return left;
}
sn_expression *sn_mul_expression::parse(Lexer &lex, Environment *env)
{
    sn_expression *e = sn_cast_expression::parse(lex, env);
    TokenType tt = lex.peakNext().type;
    if (tt == OP_MUL || tt == OP_DIV || tt == OP_MOD)
    {
        sn_mul_expression *expr = new sn_mul_expression();
        expr->op = lex.getNext().type;
        expr->left = e;
        expr->right = sn_mul_expression::parse(lex, env);

        // EXPECT_TYPE_WITH(expr->left->type(), TOp_MUL);
        // EXPECT_TYPE_WITH(expr->right->type(), TOp_MUL);

        // const Type *l = expr->left->type();
        // const Type *r = expr->right->type();
        // do
        // {
        //     if (tt == OP_MOD && l->isIntegral() && r->isIntegral())
        //         break;
        //     if (l->isArithmetic() && r->isArithmetic())
        //         break;
        //     SyntaxErrorEx("sn_mul_expression: invalid combination");
        // } while (false);

        // const Type *t = UsualArithmeticConversion(l, r);
        // if (!t->equal(*l))
        //     expr->left = new sn_cast_expression(t, expr->left);
        // if (!t->equal(*r))
        //     expr->right = new sn_cast_expression(t, expr->right);
        // expr->type_ = t;

        return expr;
    }
    else
        return e;
}
// Explicit Type Conversion
sn_expression *sn_cast_expression::parse(Lexer &lex, Environment *env)
{
    if (lex.peakNext().type == LP && first(SN_TYPE_NAME, lex.peakNext(1), env))
    {
        sn_cast_expression *ce = new sn_cast_expression();
        EXPECT(LP);
        ce->to = sn_type_name::parse(lex, env);
        EXPECT(RP);
        ce->from = parse(lex, env);
        return ce;
    }
    else
    {
        return sn_unary_expression::parse(lex, env);
    }
}
// unary expression <- Value Transformation
sn_expression *sn_unary_expression::parse(Lexer &lex, Environment *env)
{
    if (first(SN_UNARY_EXPRESSION, lex.peakNext(), env))
    {
        sn_unary_expression *expr = new sn_unary_expression();
        expr->e = nullptr;
        expr->tn = nullptr;
        switch (lex.peakNext().type)
        {
            case OP_INC:
            case OP_DEC:
                expr->op = lex.getNext().type;
                expr->e = parse(lex, env);
                // EXPECT_TYPE_WITH(expr->e->type(), TOp_INC);
                // expr->type_ = expr->e->type();
                break;
            case SIZEOF:
                expr->op = lex.getNext().type;
                if (lex.peakNext().type == LP)
                {
                    EXPECT(LP);
                    expr->tn = sn_type_name::parse(lex, env);
                    EXPECT(RP);
                }
                else
                    expr->e = sn_unary_expression::parse(lex, env);
                break;
            case OP_MUL:  // pointer
                expr->op = lex.getNext().type;
                expr->e = sn_cast_expression::parse(lex, env);
                // assert(expr->e != nullptr);
                // EXPECT_TYPE_IS(expr->e->type(), T_POINTER);
                // expr->type_ = expr->e->type()->asPointerType()->target();
                break;
            default:  // unary op: & * + - ~ !
                expr->op = lex.getNext().type;
                expr->e = sn_cast_expression::parse(lex, env);
                break;
        }
        return expr;
    }
    else
    {
        return sn_postfix_expression::parse(
            lex, env, sn_primary_expression::parse(lex, env));
    }
}
/*
 * primary-expr
 * postfix-expr '[' expr ']'
 * postfix-expr '(' [argument-expr-list] ')'
 * postfix-expr '.' identifier
 * postfix-expr '->' identifier
 * postfix-expr '++'
 * postfix-expr '--'
 */
// unary expression <- Value Transformation
sn_expression *sn_postfix_expression::parse(Lexer &lex, Environment *env,
                                            sn_expression *left)
{
    if (first(SN_POSTFIX_EXPRESSION, lex.peakNext(), env))
    {
        sn_postfix_expression *expr = new sn_postfix_expression();
        expr->left = left;
        assert(left != nullptr);
        // assert(target->type() != nullptr);

        switch (lex.peakNext().type)
        {
            case LSB:
                EXPECT(LSB);
                // EXPECT_TYPE_WITH(expr->target->type(), TOp_INDEX);
                expr->op = POSTFIX_INDEX;
                expr->data.e = sn_comma_expression::parse(lex, env);
                // EXPECT_TYPE_IS(expr->index->type(), T_INT);
                // if (dynamic_cast<Indexable *>(expr->target->type()))
                //     expr->type_ =
                //         dynamic_cast<Indexable *>(expr->target->type())
                //             ->indexedType();
                EXPECT(RSB);
                break;
            case LP:
                EXPECT(LP);
                // EXPECT_TYPE_IS(expr->target->type(), T_FUNCTION);
                // EXPECT_TYPE_WITH(expr->target->type(), TOp_CALL);
                expr->op = POSTFIX_CALL;
                expr->data.ael = nullptr;
                if (lex.peakNext().type != RP)
                    expr->data.ael =
                        sn_argument_expression_list::parse(lex, env);
                // expr->type_ =
                //     dynamic_cast<FuncType *>(expr->target->type())->rtype();
                EXPECT(RP);
                break;
            case REFER_TO:
                lex.getNext();
                // EXPECT_TYPE_WITH(expr->target->type(), TOp_OFFSET);
                expr->op = POSTFIX_OBJECT_OFFSET;
                expr->data.id = sn_identifier::parse(lex, env);
                break;
            case POINT_TO:
                lex.getNext();
                // EXPECT_TYPE_IS(expr->target->type(), T_POINTER);
                // EXPECT_TYPE_WITH(
                //     expr->target->type()->asPointerType()->target(),
                //     TOp_OFFSET);
                expr->op = POSTFIX_POINTER_OFFSET;
                expr->data.id = sn_identifier::parse(lex, env);
                // expr->type_ = dynamic_cast<StructType
                // *>(expr->target->type())
                //                   ->getMember(expr->member)
                //                   ->type;
                break;
            case OP_INC:
                lex.getNext();
                expr->op = POSTFIX_INC;
                // EXPECT_TYPE_WITH(expr->target->type(), TOp_INC);
                // expr->type_ = expr->target->type();
                break;
            case OP_DEC:
                lex.getNext();
                expr->op = POSTFIX_DEC;
                // EXPECT_TYPE_WITH(expr->target->type(), TOp_DEC);
                // expr->type_ = expr->target->type();
                break;
            default: SyntaxError("Should not reach here."); break;
        }

        return parse(lex, env, expr);
    }
    else
    {
        return left;
    }
}
// unary expression <- Value Transformation
sn_expression *sn_primary_expression::parse(Lexer &lex, Environment *env)
{
    sn_primary_expression *p = new sn_primary_expression();
    p->t = lex.peakNext();
    switch (lex.peakNext().type)
    {
        case CONST_CHAR:
        case CONST_INT:
        case CONST_FLOAT:
        case STRING: lex.getNext(); break;
        case SYMBOL:
            lex.getNext();
            p->data.id = sn_identifier::parse(lex, env);
            break;
        case LP:
            EXPECT(LP);
            p->data.expr = sn_comma_expression::parse(lex, env);
            EXPECT(RP);
            break;
        default: SyntaxErrorEx("Unsupported primary expression"); break;
    }

    return p;
}

sn_argument_expression_list *sn_argument_expression_list::parse(
    Lexer &lex, Environment *env)
{
    sn_argument_expression_list *ael = new sn_argument_expression_list();
    ael->ae = sn_assign_expression::parse(lex, env);
    ael->left = nullptr;
    if (lex.peakNext().type == OP_COMMA)
    {
        lex.getNext();
        sn_argument_expression_list *ael2 = parse(lex, env);
        ael2->left = ael;
        ael = ael2;
    }
    return ael;
}

sn_const_expression *sn_const_expression::parse(Lexer &lex, Environment *env)
{
    sn_const_expression *ce = new sn_const_expression();
    ce->e = sn_cond_expression::parse(lex, env);
    return ce;
}

std::string SyntaxNode::debugString()
{
    return "~<?~>";
}
std::string sn_translation_unit::debugString()
{
    std::string s = left ? left->debugString() : "~<translation_unit~>";
    s += "\n>\n";
    s += ed->debugString();
    s += "<\n";
    return s;
}
std::string sn_external_declaration::debugString()
{
    std::string s = "~<external_declaration~>";
    s += "\n>\n";
    s += branch == FUNCDEF ? data.fd->debugString() : data.d->debugString();
    s += "<\n";
    return s;
}
std::string sn_function_definition::debugString()
{
    std::string ds = "~<function_definition~>";
    ds += "\n>\n";
    if (s)
        ds += s->debugString();
    if (d)
        ds += d->debugString();
    if (dl)
        ds += dl->debugString();
    if (cs)
        ds += cs->debugString();
    ds += "<\n";
    return ds;
}
std::string sn_declaration::debugString()
{
    std::string ds = "~<declaration~>";
    ds += "\n>\n";
    if (s)
        ds += s->debugString();
    if (idl)
        ds += idl->debugString();
    ds += "<\n";
    return ds;
}
std::string sn_label_statement::debugString()
{
    return "~<sn_label_statement~>";
}
std::string sn_expression_statement::debugString()
{
    std::string s = "~<declaration~>";
    s += "\n>\n";
    if (expr)
        s += expr->debugString();
    s += "<\n";
    return s;
}
// TODO: support switch
std::string sn_selection_statement::debugString()
{
    std::string s;
    s += "if>\n";
    s += expr->debugString();
    s += "<then>\n";
    s += stmt->debugString();
    if (stmt2)
    {
        s += "<else>\n";
        s += stmt2->debugString();
    }
    s += "<\n";
    return s;
}
std::string sn_iteration_statement::debugString()
{
    std::string s;
    switch (type)
    {
        case WHILE_LOOP:
            s += "while>\n";
            if (expr)
                s += expr->debugString();
            s += "<\ndo>\n";
            if (stmt)
                s += stmt->debugString();
            break;
        case DO_LOOP:
            s += "do>\n";
            if (stmt)
                s += stmt->debugString();
            s += "<\nwhile>\n";
            if (expr)
                s += expr->debugString();
            break;
        case FOR_LOOP:
            s += "for>\n";
            if (expr)
                s += expr->debugString();
            if (expr2)
                s += expr2->debugString();
            if (expr3)
                s += expr3->debugString();
            if (stmt)
                s += stmt->debugString();
            break;
        default: s += ">\n"; break;
    }
    s += "<\n";
    return s;
}
std::string sn_jump_statement::debugString()
{
    std::string s;
    switch (type)
    {
        case JMP_BREAK: s += "break"; break;
        case JMP_CONTINUE: s += "continue"; break;
        case JMP_GOTO: s += "goto"; break;
        case JMP_RETURN: s += "return"; break;
    }
    s += ">\n";
    if (data.expr)
        s += data.expr->debugString();
    s += "<\n";
    return s;
}
std::string sn_comma_expression::debugString()
{
    std::string s = ",>\n";
    s += curr->debugString();
    if (next)
        s += next->debugString();
    s += "<\n";
    return s;
}
std::string sn_assign_expression::debugString()
{
    std::string s = "=>\n";
    // for (SyntaxNode *t : targets) s += t->debugString();
    if (to)
        s += to->debugString();
    if (from)
        s += from->debugString();
    s += "<\n";
    return s;
}
std::string sn_cond_expression::debugString()
{
    std::string s = "?:>\n";
    if (cond)
        s += cond->debugString();
    if (left)
        s += left->debugString();
    if (right)
        s += right->debugString();
    s += "<\n";
    return s;
}
std::string sn_or_expression::debugString()
{
    std::string s = "||>\n";
    if (left)
        s += left->debugString();
    if (right)
        s += right->debugString();
    s += "<\n";
    return s;
}
std::string sn_and_expression::debugString()
{
    std::string s = "&&>\n";
    if (left)
        s += left->debugString();
    if (right)
        s += right->debugString();
    s += "<\n";
    return s;
}
std::string sn_bitor_expression::debugString()
{
    std::string s = "|>\n";
    if (left)
        s += left->debugString();
    if (right)
        s += right->debugString();
    s += "<\n";
    return s;
}
std::string sn_bitxor_expression::debugString()
{
    std::string s = "^>\n";
    if (left)
        s += left->debugString();
    if (right)
        s += right->debugString();
    s += "<\n";
    return s;
}
std::string sn_bitand_expression::debugString()
{
    std::string s = "&>\n";
    if (left)
        s += left->debugString();
    if (right)
        s += right->debugString();
    s += "<\n";
    return s;
}
std::string sn_eq_expression::debugString()
{
    std::string s;
    if (op == REL_EQ)
        s += "==";
    else
        s += "!=";
    s += ">\n";
    if (left)
        s += left->debugString();
    if (right)
        s += right->debugString();
    s += "<\n";
    return s;
}
std::string sn_rel_expression::debugString()
{
    std::string s;
    switch (op)
    {
        case REL_GT: s += "~>"; break;
        case REL_GE: s += "~>="; break;
        case REL_LT: s += "~<"; break;
        case REL_LE: s += "~<="; break;
        default: break;
    }
    s += "\n>\n";
    if (left)
        s += left->debugString();
    if (right)
        s += right->debugString();
    s += "<\n";
    return s;
}
std::string sn_shift_expression::debugString()
{
    std::string s;
    switch (op)
    {
        case BIT_SLEFT: s += "~<~<"; break;
        case BIT_SRIGHT: s += "~>~>"; break;
        default: break;
    }
    s += "\n>\n";
    if (left)
        s += left->debugString();
    if (right)
        s += right->debugString();
    s += "<\n";
    return s;
}
std::string sn_add_expression::debugString()
{
    std::string s;
    switch (op)
    {
        case OP_ADD: s += "+"; break;
        case OP_SUB: s += "-"; break;
        default: break;
    }
    s += "\n>\n";
    if (left)
        s += left->debugString();
    if (right)
        s += right->debugString();
    s += "<\n";
    return s;
}
std::string sn_mul_expression::debugString()
{
    std::string s;
    switch (op)
    {
        case OP_MUL: s += "*"; break;
        case OP_DIV: s += "/"; break;
        case OP_MOD: s += "%"; break;
        default: break;
    }
    s += "\n>\n";
    if (left)
        s += left->debugString();
    if (right)
        s += right->debugString();
    s += "<\n";
    return s;
}
std::string sn_cast_expression::debugString()
{
    std::string s;
    // s += implicit_ ? "imp:" : "exp:";
    // s += from ? from->type()->toString() : "<null>";
    // s += " -~> ";
    // s += type_ ? type_->toString() : "<null>";
    s += "~<cast_expression~>";
    s += "\n>\n";
    if (to)
        s += to->debugString();
    if (from)
        s += from->debugString();
    s += "<\n";
    return s;
}
std::string sn_unary_expression::debugString()
{
    std::string s;
    switch (op)
    {
        case OP_INC:
        case OP_DEC:
        case BIT_AND:
        case OP_MUL:
        case OP_ADD:
        case OP_SUB:
        case BIT_NOT:
        case BOOL_NOT:
        case SIZEOF: s += Token::DebugTokenType(op); break;
        default: break;
    }
    s += "\n>\n";
    if (e)
        s += e->debugString();
    else if (tn)
        s += tn->debugString();
    s += "<\n";
    return s;
}
std::string sn_postfix_expression::debugString()
{
    std::string s;
    s += ">\n";
    s += left->debugString();
    s += "<\n";
    switch (op)
    {
        case POSTFIX_INDEX:
            s += "[]>\n";
            s += data.e->debugString();
            break;
        case POSTFIX_CALL:
            s += "()>\n";
            s += data.ael->debugString();
            break;
        case POSTFIX_OBJECT_OFFSET:
            s += ".>\n";
            s += data.id->debugString();
            break;
        case POSTFIX_POINTER_OFFSET:
            s += "-~>>\n";
            s += data.id->debugString();
            break;
        case POSTFIX_INC: s += "++(post)>\n"; break;
        case POSTFIX_DEC: s += "--(post)>\n"; break;
        default: break;
    }
    s += "<\n";
    return s;
}
std::string sn_primary_expression::debugString()
{
    std::string s;
    switch (t.type)
    {
        case SYMBOL: s += data.id->debugString(); break;
        case CONST_CHAR:
        case CONST_INT:
        case CONST_FLOAT: s += Token::DebugTokenType(t.type); break;
        case STRING:
            s += '"';
            s += t.string_.toString();
            s += '"';
            break;
        case LP: s += data.expr->debugString(); break;
        default: break;
    }
    s += '\n';
    return s;
}

void Parser::__debugPrint(string &&s)
{
    std::string tabs = "  ";
    std::string line = "  ";
    bool escape = false, empty = true;
    bool printenv = false;
    uintptr_t env = 0;
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
