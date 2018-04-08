#include "ast.h"

// TODO: check if symbol is typedef-name
bool FIRST(uint64_t types, Token t)
{
    static std::map<TokenType, uint64_t> m;
    if (m.empty())
    {
        m[LP] = SN_STATEMENT_LIST | SN_POSTFIX_EXPRESSION |
                SN_UNARY_EXPRESSION | SN_INITIALIZER_LIST |
                SN_DIRECT_DECLARATOR | SN_DIRECT_ABSTRACT_DECLARATOR |
                SN_ABSTRACT_DECLARATOR;
        m[LSB] = SN_DIRECT_DECLARATOR | SN_DIRECT_ABSTRACT_DECLARATOR |
                 SN_ABSTRACT_DECLARATOR;
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
                    SN_DIRECT_DECLARATOR | SN_DECLARATION_SPECIFIERS |
                    SN_INITIALIZER_LIST | SN_PARAMETER_TYPE_LIST;
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
        return false;  // env->recursiveFindTypename(t.symbol);
    else
        return (kv->second & types) != 0;
}

bool FOLLOW(uint64_t types, TokenType t)
{
    static std::map<TokenType, uint64_t> m;
    if (m.empty())
    {
        m[RP] =
            SN_TYPE_NAME | SN_PARAMETER_TYPE_LIST | SN_PARAMETER_DECLARATION;
        m[OP_COMMA] = SN_PARAMETER_DECLARATION;
    }
    auto kv = m.find(t);
    // if (kv == m.end())
    //     SyntaxError("FOLLOW: type not implemented.");
    return kv != m.end() && (kv->second & types) != 0;
}

sn_translation_unit *sn_translation_unit::parse(Lexer &lex)
{
    sn_translation_unit *t = new sn_translation_unit();
    while (lex.hasNext())
    {
        t->addChild(sn_external_declaration::parse(lex));
    }
    return t;
}
sn_external_declaration *sn_external_declaration::parse(Lexer &lex)
{
    sn_external_declaration *ed = new sn_external_declaration();

    sn_declaration_specifiers *s = sn_declaration_specifiers::parse(lex);
    sn_declarator *d = sn_declarator::parse(lex);
    if (FIRST(SN_COMPOUND_STATEMENT | SN_DECLARATION_LIST, lex.peakNext()))
    {
        ed->addChild(sn_function_definition::parse(lex, s, d));
    }
    else
    {
        ed->addChild(sn_declaration::parse(lex, s, d));
    }
    return ed;
}
sn_function_definition *sn_function_definition::parse(
    Lexer &lex, sn_declaration_specifiers *s, sn_declarator *d)
{
    sn_function_definition *fd = new sn_function_definition();
    fd->addChild(s);
    fd->addChild(d);
    if (FIRST(SN_DECLARATION_LIST, lex.peakNext()))
        fd->addChild(sn_declaration_list::parse(lex));
    fd->addChild(sn_compound_statement::parse(lex));
    return fd;
}
sn_declaration *sn_declaration::parse(Lexer &lex)
{
    sn_declaration *dn = new sn_declaration();
    dn->addChild(sn_declaration_specifiers::parse(lex));
    if (FIRST(SN_INITIALIZER_LIST, lex.peakNext()))
        dn->addChild(sn_init_declarator_list::parse(lex));
    EXPECT(STMT_END);
    return dn;
}
sn_declaration *sn_declaration::parse(Lexer &lex, sn_declaration_specifiers *s,
                                      sn_declarator *d)
{
    sn_declaration *dn = new sn_declaration();

    if (s == nullptr)
        SyntaxError("Declaration: missing specifiers.");
    dn->addChild(s);

    if (d != nullptr)
    {
        sn_init_declarator *id = new sn_init_declarator();
        id->addChild(d);
        if (SKIP(ASSIGN))
            id->addChild(sn_initializer::parse(lex));

        dn->addChild(sn_init_declarator_list::parse(lex, id));
    }
    EXPECT(STMT_END);
    return dn;
}
sn_declaration_list *sn_declaration_list::parse(Lexer &lex)
{
    sn_declaration_list *dl = new sn_declaration_list();
    while (FIRST(SN_DECLARATION, lex.peakNext()))
        dl->addChild(sn_declaration::parse(lex));
    return dl;
}

// Declaration
sn_init_declarator *sn_init_declarator::parse(Lexer &lex)
{
    sn_init_declarator *id = new sn_init_declarator();
    id->addChild(sn_declarator::parse(lex));
    if (SKIP(ASSIGN))
        id->addChild(sn_initializer::parse(lex));
    return id;
}
sn_init_declarator_list *sn_init_declarator_list::parse(Lexer &lex,
                                                        sn_init_declarator *id)
{
    sn_init_declarator_list *idl = new sn_init_declarator_list();
    if (id)
        idl->addChild(id);
    else
        idl->addChild(sn_init_declarator::parse(lex));
    while (SKIP(OP_COMMA))
        idl->addChild(sn_init_declarator::parse(lex));
    return idl;
}
sn_declarator *sn_declarator::parse(Lexer &lex)
{
    sn_declarator *d = new sn_declarator();

    if (FIRST(SN_POINTER, lex.peakNext()))
        d->addChild(sn_pointer::parse(lex));
    d->addChild(sn_direct_declarator::parse(lex));
    return d;
}
sn_direct_declarator *sn_direct_declarator::parse(Lexer &lex)
{
    sn_direct_declarator *dd = new sn_direct_declarator();

    switch (lex.peakNext().type)
    {
        case SYMBOL: dd->addChild(sn_identifier::parse(lex)); break;
        case LP:
            EXPECT(LP);
            dd->addChild(sn_declarator::parse(lex));
            EXPECT(RP);
            break;
        default:
            SyntaxErrorDebug("Direct Declarator: unexpected token.");
            break;
    }

    while (FIRST(SN_DIRECT_DECLARATOR, lex.peakNext()))
    {
        switch (lex.peakNext().type)
        {
            case LSB:
                EXPECT(LSB);
                if (lex.peakNext().type != RSB)
                    dd->addChild(sn_const_expression::parse(lex));
                else
                    dd->addChild(new sn_const_expression());
                EXPECT(RSB);
                break;
            case LP:
                EXPECT(LP);
                if (FIRST(SN_PARAMETER_TYPE_LIST, lex.peakNext()))
                    dd->addChild(sn_parameter_type_list::parse(lex));
                else if (FIRST(SN_IDENTIFIER_LIST, lex.peakNext()))
                    dd->addChild(sn_identifier_list::parse(lex));
                else if (lex.peakNext().type == RP)
                    dd->addChild(new sn_identifier_list());
                else
                    SyntaxError("unexpected token");
                EXPECT(RP);
                break;
            default: SyntaxError("unexpected token"); break;
        }
    }

    return dd;
}
sn_abstract_declarator *sn_abstract_declarator::parse(Lexer &lex)
{
    sn_abstract_declarator *d = new sn_abstract_declarator();
    if (FIRST(SN_POINTER, lex.peakNext()))
    {
        d->addChild(sn_pointer::parse(lex));
        if (FIRST(SN_DIRECT_ABSTRACT_DECLARATOR, lex.peakNext()))
            d->addChild(sn_direct_abstract_declarator::parse(lex));
    }
    else
        d->addChild(sn_direct_abstract_declarator::parse(lex));
    return d;
}
sn_direct_abstract_declarator *sn_direct_abstract_declarator::parse(Lexer &lex)
{
    sn_direct_abstract_declarator *dad = new sn_direct_abstract_declarator();

    bool is_first = true;
    do
    {
        switch (lex.peakNext().type)
        {
            case LP:
                EXPECT(LP);
                if (is_first && FIRST(SN_ABSTRACT_DECLARATOR, lex.peakNext()))
                {
                    dad->addChild(sn_abstract_declarator::parse(lex));
                }
                else
                {
                    dad->addChild(
                        lex.peakNext().type == RP
                            ? (SyntaxNode *)new sn_parameter_type_list()
                            : (SyntaxNode *)sn_parameter_type_list::parse(lex));
                }
                EXPECT(RP);
            case LSB:
                EXPECT(LSB);
                if (lex.peakNext().type != RSB)
                    dad->addChild(sn_const_expression::parse(lex));
                else
                    dad->addChild(new sn_const_expression());
                EXPECT(RSB);
                break;
            default:
                SyntaxError("Direct Abstract Declarator: unexpected token.");
                break;
        }
        is_first = false;
    } while (FIRST(SN_DIRECT_ABSTRACT_DECLARATOR, lex.peakNext()));

    return dad;
}
sn_initializer *sn_initializer::parse(Lexer &lex)
{
    sn_initializer *i = new sn_initializer();
    if (lex.peakNext().type == BLK_BEGIN)
    {
        EXPECT(BLK_BEGIN);
        i->addChild(sn_initializer_list::parse(lex));
        if (lex.peakNext().type == OP_COMMA)
            lex.getNext();
        EXPECT(BLK_END);
    }
    else
    {
        i->addChild(sn_assign_expression::parse(lex));
    }
    return i;
}
sn_initializer_list *sn_initializer_list::parse(Lexer &lex)
{
    sn_initializer_list *il = new sn_initializer_list();
    il->addChild(sn_initializer::parse(lex));
    while (SKIP(OP_COMMA))
        il->addChild(sn_initializer::parse(lex));
    return il;
}
sn_parameter_type_list *sn_parameter_type_list::parse(Lexer &lex)
{
    sn_parameter_type_list *ptl = new sn_parameter_type_list();
    ptl->addChild(sn_parameter_list::parse(lex));
    ptl->varlist = false;
    if (SKIP(OP_COMMA))
    {
        EXPECT(VAR_PARAM);
        ptl->varlist = true;
    }
    return ptl;
}
sn_parameter_list *sn_parameter_list::parse(Lexer &lex)
{
    sn_parameter_list *pl = new sn_parameter_list();
    pl->addChild(sn_parameter_declaration::parse(lex));
    while (lex.peakNext().type == OP_COMMA && lex.peakNext(1).type != VAR_PARAM)
    {
        lex.getNext();
        pl->addChild(sn_parameter_declaration::parse(lex));
    }
    return pl;
}
int declarator_or_abstract_declarator(Lexer &lex)
{
    int status = 0;  // 1: declarator 2: abstract declarator 3: none
    bool inner = false;
    for (size_t i = 0; status == 0; ++i)
    {
        while (lex.peakNext(i).type == OP_MUL)
        {
            ++i;
            if (lex.peakNext(i).type == CONST)
                ++i;
        }
        if (inner && lex.peakNext(i).type == RP)
        {
            status = 2;
            break;
        }

        switch (lex.peakNext(i).type)
        {
            case SYMBOL: status = 1; break;
            case LSB: status = 2; break;
            case LP:
                if (lex.peakNext(i + 1).type == RP)
                    status = 2;
                else
                    inner = true;
                break;
            default:
                if (FIRST(SN_DECLARATION_SPECIFIERS, lex.peakNext(i)))
                    status = 2;
                else if (FOLLOW(SN_PARAMETER_DECLARATION, lex.peakNext(i).type))
                    status = 3;
                else
                    status = -1;
                break;
        }
    }
    return status;
}
sn_parameter_declaration *sn_parameter_declaration::parse(Lexer &lex)
{
    sn_parameter_declaration *pd = new sn_parameter_declaration();
    pd->addChild(sn_declaration_specifiers::parse(lex));
    switch (declarator_or_abstract_declarator(lex))
    {
        case 1: pd->addChild(sn_declarator::parse(lex)); break;
        case 2: pd->addChild(sn_abstract_declarator::parse(lex)); break;
        case -1: SyntaxError("Parameter Declaration: unexpected token."); break;
        case 3:
        default: break;
    }
    return pd;
}

// specifier
sn_declaration_specifiers *sn_declaration_specifiers::parse(Lexer &lex)
{
    sn_declaration_specifiers *s = new sn_declaration_specifiers();

    do
    {
        if (FIRST(SN_STORAGE_SPECIFIER, lex.peakNext()))
            s->addChild(sn_storage_specifier::parse(lex));
        else if (FIRST(SN_TYPE_SPECIFIER, lex.peakNext()))
            s->addChild(sn_type_specifier::parse(lex));
        else
            s->addChild(sn_type_qualifier::parse(lex));
    } while (FIRST(SN_DECLARATION_SPECIFIERS, lex.peakNext()));

    return s;
}
sn_specifier_qualifier_list *sn_specifier_qualifier_list::parse(Lexer &lex)
{
    sn_specifier_qualifier_list *sql = new sn_specifier_qualifier_list();

    do
    {
        if (FIRST(SN_TYPE_SPECIFIER, lex.peakNext()))
            sql->addChild(sn_type_specifier::parse(lex));
        else
            sql->addChild(sn_type_qualifier::parse(lex));
    } while (FIRST(SN_TYPE_SPECIFIER | SN_TYPE_QUALIFIER, lex.peakNext()));

    return sql;
}
sn_storage_specifier *sn_storage_specifier::parse(Lexer &lex)
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
sn_type_qualifier *sn_type_qualifier::parse(Lexer &lex)
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
sn_type_specifier *sn_type_specifier::parse(Lexer &lex)
{
    sn_type_specifier *ts = new sn_type_specifier();
    ts->t = NONE;
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
        case UNSIGNED: ts->t = lex.getNext().type; break;
        case TYPE_STRUCT:
        case TYPE_UNION:
            ts->t = lex.peakNext().type;
            ts->addChild(sn_struct_union_specifier::parse(lex));
            break;
        case TYPE_ENUM:
            ts->t = lex.peakNext().type;
            ts->addChild(sn_enum_specifier::parse(lex));
            break;
        case SYMBOL:
            ts->t = lex.peakNext().type;
            ts->addChild(sn_typedef_name::parse(lex));
            break;
        default: SyntaxError("Type Specifier: unexpected token"); break;
    }
    return ts;
}
sn_type_qualifier_list *sn_type_qualifier_list::parse(Lexer &lex)
{
    sn_type_qualifier_list *tql = new sn_type_qualifier_list();

    do
    {
        tql->addChild(sn_type_qualifier::parse(lex));
    } while (FIRST(SN_TYPE_QUALIFIER, lex.peakNext()));

    return tql;
}
// struct specifier
sn_struct_union_specifier *sn_struct_union_specifier::parse(Lexer &lex)
{
    sn_struct_union_specifier *sus = new sn_struct_union_specifier();

    if (lex.peakNext().type == TYPE_STRUCT || lex.peakNext().type == TYPE_UNION)
        sus->t = lex.getNext().type;
    else
        SyntaxError("Struct: unexpected token");

    if (lex.peakNext().type != BLK_BEGIN)
        sus->addChild(sn_identifier::parse(lex));

    if (lex.peakNext().type == BLK_BEGIN)
    {
        EXPECT(BLK_BEGIN);
        sus->addChild(sn_struct_declaration_list::parse(lex));
        EXPECT(BLK_END);
    }

    return sus;
}
sn_struct_declaration *sn_struct_declaration::parse(Lexer &lex)
{
    sn_struct_declaration *sd = new sn_struct_declaration();

    sd->addChild(sn_specifier_qualifier_list::parse(lex));
    sd->addChild(sn_struct_declarator_list::parse(lex));
    EXPECT(STMT_END);

    return sd;
}
sn_struct_declaration_list *sn_struct_declaration_list::parse(Lexer &lex)
{
    sn_struct_declaration_list *sdl = new sn_struct_declaration_list();

    do
    {
        sdl->addChild(sn_struct_declaration::parse(lex));
    } while (lex.peakNext().type != BLK_END);

    return sdl;
}
sn_struct_declarator *sn_struct_declarator::parse(Lexer &lex)
{
    sn_struct_declarator *sd = new sn_struct_declarator();

    if (lex.peakNext().type != OP_COLON)
        sd->addChild(sn_declarator::parse(lex));
    if (SKIP(OP_COLON))
        sd->addChild(sn_const_expression::parse(lex));

    return sd;
}
sn_struct_declarator_list *sn_struct_declarator_list::parse(Lexer &lex)
{
    sn_struct_declarator_list *sdl = new sn_struct_declarator_list();

    do
    {
        sdl->addChild(sn_struct_declarator::parse(lex));
    } while (SKIP(OP_COMMA));

    return sdl;
}
// enum specifier
sn_enum_specifier *sn_enum_specifier::parse(Lexer &lex)
{
    sn_enum_specifier *es = new sn_enum_specifier();

    EXPECT(TYPE_ENUM);

    if (lex.peakNext().type != BLK_BEGIN)
        es->addChild(sn_identifier::parse(lex));

    if (lex.peakNext().type == BLK_BEGIN)
    {
        EXPECT(BLK_BEGIN);
        es->addChild(sn_enumerator_list::parse(lex));
        EXPECT(BLK_END);
    }
    return es;
}
sn_enumerator_list *sn_enumerator_list::parse(Lexer &lex)
{
    sn_enumerator_list *el = new sn_enumerator_list();

    do
    {
        el->addChild(sn_enumerator::parse(lex));
    } while (SKIP(OP_COMMA));

    return el;
}
sn_enumerator *sn_enumerator::parse(Lexer &lex)
{
    sn_enumerator *e = new sn_enumerator();

    e->addChild(sn_enumeration_constant::parse(lex));
    if (SKIP(ASSIGN))
        e->addChild(sn_const_expression::parse(lex));

    return e;
}
sn_enumeration_constant *sn_enumeration_constant::parse(Lexer &lex)
{
    sn_enumeration_constant *ec = new sn_enumeration_constant();
    ec->addChild(sn_identifier::parse(lex));
    return ec;
}

sn_type_name *sn_type_name::parse(Lexer &lex)
{
    sn_type_name *tn = new sn_type_name();

    tn->addChild(sn_specifier_qualifier_list::parse(lex));
    if (!FOLLOW(SN_TYPE_NAME, lex.peakNext().type))
        tn->addChild(sn_abstract_declarator::parse(lex));

    return tn;
}
sn_pointer *sn_pointer::parse(Lexer &lex)
{
    sn_pointer *p = new sn_pointer();

    EXPECT(OP_MUL);
    if (FIRST(SN_TYPE_QUALIFIER, lex.peakNext()))
        p->addChild(sn_type_qualifier_list::parse(lex));
    if (FIRST(SN_POINTER, lex.peakNext()))
        p->addChild(parse(lex));

    return p;
}
sn_identifier *sn_identifier::parse(Lexer &lex)
{
    sn_identifier *id = new sn_identifier();
    id->id = EXPECT_GET(SYMBOL);
    return id;
}
sn_identifier_list *sn_identifier_list::parse(Lexer &lex)
{
    sn_identifier_list *il = new sn_identifier_list();

    do
    {
        il->addChild(sn_identifier::parse(lex));
    } while (SKIP(OP_COMMA));

    return il;
}
sn_typedef_name *sn_typedef_name::parse(Lexer &lex)
{
    sn_typedef_name *tn = new sn_typedef_name();
    tn->addChild(sn_identifier::parse(lex));
    return tn;
}

// Statement
sn_statement *sn_statement::parse(Lexer &lex)
{
    sn_statement *node = nullptr;
    switch (lex.peakNext().type)
    {
        case SYMBOL:
            if (lex.peakNext(1).type == OP_COLON)
            {
                node = sn_label_statement::parse(lex);
            }
            else
            {
                node = sn_expression_statement::parse(lex);
            }
            break;
        case CASE:
        case DEFAULT: node = sn_label_statement::parse(lex); break;
        case BLK_BEGIN: node = sn_compound_statement::parse(lex); break;
        case IF:
        case SWITCH: node = sn_selection_statement::parse(lex); break;
        case WHILE:
        case DO:
        case FOR: node = sn_iteration_statement::parse(lex); break;
        case GOTO:
        case CONTINUE:
        case BREAK:
        case RETURN: node = sn_jump_statement::parse(lex); break;
        default:
            /* TODO: check FIRST(expr) */
            node = sn_expression_statement::parse(lex);
            break;
    }
    if (node == nullptr)
        SyntaxError("Statement: empty statement");
    return node;
}
sn_statement_list *sn_statement_list::parse(Lexer &lex)
{
    sn_statement_list *sl = new sn_statement_list();

    do
    {
        sl->addChild(sn_statement::parse(lex));
    } while (lex.peakNext().type != BLK_END);

    return sl;
}
sn_statement *sn_label_statement::parse(Lexer &lex)
{
    sn_label_statement *ls = new sn_label_statement();

    switch (lex.peakNext().type)
    {
        case SYMBOL: ls->addChild(sn_identifier::parse(lex)); break;
        case CASE:
            lex.getNext();
            ls->addChild(sn_const_expression::parse(lex));
            break;
        case DEFAULT: lex.getNext(); break;
        default: SyntaxError("Label Statement: unexpected token."); break;
    }
    EXPECT(OP_COLON);
    ls->addChild(sn_statement::parse(lex));

    return ls;
}
sn_compound_statement *sn_compound_statement::parse(Lexer &lex)
{
    sn_compound_statement *cs = new sn_compound_statement();

    EXPECT(BLK_BEGIN);
    if (FIRST(SN_DECLARATION_LIST, lex.peakNext()))
        cs->addChild(sn_declaration_list::parse(lex));
    if (FIRST(SN_STATEMENT_LIST, lex.peakNext()))
        cs->addChild(sn_statement_list::parse(lex));
    EXPECT(BLK_END);

    return cs;
}
sn_statement *sn_expression_statement::parse(Lexer &lex)
{
    sn_expression_statement *stmt = new sn_expression_statement();

    if (lex.peakNext().type != STMT_END)
        stmt->addChild(sn_comma_expression::parse(lex));
    EXPECT(STMT_END);

    return stmt;
}
sn_statement *sn_selection_statement::parse(Lexer &lex)
{
    sn_selection_statement *stmt = new sn_selection_statement();

    switch (lex.peakNext().type)
    {
        case IF:
        case SWITCH: stmt->t = lex.getNext().type; break;
        default: SyntaxError("Selection Statement: unexpected token"); break;
    }

    EXPECT(LP);
    stmt->addChild(sn_comma_expression::parse(lex));
    EXPECT(RP);
    stmt->addChild(sn_statement::parse(lex));

    if (stmt->t == IF && SKIP(ELSE))
    {
        stmt->addChild(sn_statement::parse(lex));
    }

    return stmt;
}
sn_statement *sn_iteration_statement::parse(Lexer &lex)
{
    sn_iteration_statement *stmt = new sn_iteration_statement();

    stmt->t = lex.peakNext().type;
    switch (lex.getNext().type)
    {
        case WHILE:
            EXPECT(LP);
            stmt->addChild(sn_comma_expression::parse(lex));
            EXPECT(RP);
            stmt->addChild(sn_statement::parse(lex));
            break;
        case DO:
            stmt->addChild(sn_statement::parse(lex));
            EXPECT(WHILE);
            EXPECT(LP);
            stmt->addChild(sn_comma_expression::parse(lex));
            EXPECT(RP);
            EXPECT(STMT_END);
            break;
        case FOR:
            EXPECT(LP);
            stmt->pre = stmt->mid = stmt->post = false;
            if (lex.peakNext().type != STMT_END)
                stmt->addChild(sn_comma_expression::parse(lex)),
                    stmt->pre = true;
            EXPECT(STMT_END);
            if (lex.peakNext().type != STMT_END)
                stmt->addChild(sn_comma_expression::parse(lex)),
                    stmt->mid = true;
            EXPECT(STMT_END);
            if (lex.peakNext().type != RP)
                stmt->addChild(sn_comma_expression::parse(lex)),
                    stmt->post = true;
            EXPECT(RP);
            stmt->addChild(sn_statement::parse(lex));
            break;
        default: SyntaxError("Iteration Statement: unexpected token"); break;
    }

    return stmt;
}
sn_statement *sn_jump_statement::parse(Lexer &lex)
{
    sn_jump_statement *stmt = new sn_jump_statement();

    stmt->t = lex.peakNext().type;
    switch (lex.getNext().type)
    {
        case BREAK:
        case CONTINUE: break;
        case GOTO: stmt->addChild(sn_identifier::parse(lex)); break;
        case RETURN:
            if (lex.peakNext().type != STMT_END)
                stmt->addChild(sn_comma_expression::parse(lex));
            break;
        default: SyntaxError("Jump Statement: unexpected token"); break;
    }
    EXPECT(STMT_END);

    return stmt;
}

// Expression
sn_expression *sn_comma_expression::parse(Lexer &lex)
{
    sn_expression *e = sn_assign_expression::parse(lex);
    if (SKIP(OP_COMMA))
    {
        sn_comma_expression *expr = new sn_comma_expression();

        expr->addChild(e);
        do
        {
            expr->addChild(sn_assign_expression::parse(lex));
        } while (SKIP(OP_COMMA));

        return expr;
    }
    else
        return e;
}
sn_expression *sn_assign_expression::parse(Lexer &lex)
{
    sn_expression *to = sn_cond_expression::parse(lex);
    if ((to->nodeType() == SN_UNARY_EXPRESSION ||
         to->nodeType() == SN_POSTFIX_EXPRESSION ||
         to->nodeType() == SN_PRIMARY_EXPRESSION) &&
        lex.peakNext().category == TOKEN_CATEGORY_ASSIGN_OPERATOR)
    {
        sn_expression *from = nullptr;
        TokenType op = NONE;

        // parse child
        if (lex.peakNext().category != TOKEN_CATEGORY_ASSIGN_OPERATOR)
            SyntaxError("expect assign operator.");
        op = lex.getNext().type;
        from = sn_assign_expression::parse(lex);

        sn_assign_expression *expr = new sn_assign_expression();
        expr->addChild(to);
        expr->op = op;
        expr->addChild(from);

        return expr;
    }
    else
        return to;
}

sn_expression *sn_cond_expression::parse(Lexer &lex)
{
    sn_expression *test = sn_or_expression::parse(lex);
    if (SKIP(OP_QMARK))
    {
        sn_expression *left = nullptr;
        sn_expression *right = nullptr;

        // parse child
        left = sn_comma_expression::parse(lex);
        EXPECT(OP_COLON);
        right = sn_cond_expression::parse(lex);

        sn_cond_expression *expr = new sn_cond_expression();
        expr->addChild(test);
        expr->addChild(left);
        expr->addChild(right);

        return expr;
    }
    else
    {
        return test;
    }
}
sn_expression *sn_or_expression::parse(Lexer &lex)
{
    sn_expression *left = sn_and_expression::parse(lex);
    if (SKIP(BOOL_OR))
    {
        sn_expression *right = nullptr;

        // parse child
        right = sn_or_expression::parse(lex);

        sn_or_expression *expr = new sn_or_expression();
        expr->addChild(left);
        expr->addChild(right);

        return expr;
    }
    else
        return left;
}
sn_expression *sn_and_expression::parse(Lexer &lex)
{
    sn_expression *left = sn_bitor_expression::parse(lex);
    if (SKIP(BOOL_AND))
    {
        sn_expression *right = nullptr;

        // parse child
        right = sn_and_expression::parse(lex);

        sn_and_expression *expr = new sn_and_expression();
        expr->addChild(left);
        expr->addChild(right);

        return expr;
    }
    else
        return left;
}
sn_expression *sn_bitor_expression::parse(Lexer &lex)
{
    sn_expression *left = sn_bitxor_expression::parse(lex);
    if (SKIP(BIT_OR))
    {
        sn_expression *right = nullptr;

        // parse child
        right = sn_bitor_expression::parse(lex);

        sn_bitor_expression *expr = new sn_bitor_expression();
        expr->addChild(left);
        expr->addChild(right);

        return expr;
    }
    else
        return left;
}
sn_expression *sn_bitxor_expression::parse(Lexer &lex)
{
    sn_expression *left = sn_bitand_expression::parse(lex);
    if (SKIP(BIT_XOR))
    {
        sn_expression *right = nullptr;

        // parse child
        right = sn_bitxor_expression::parse(lex);

        sn_bitxor_expression *expr = new sn_bitxor_expression();
        expr->addChild(left);
        expr->addChild(right);

        return expr;
    }
    else
        return left;
}
sn_expression *sn_bitand_expression::parse(Lexer &lex)
{
    sn_expression *left = sn_eq_expression::parse(lex);
    if (SKIP(BIT_AND))
    {
        sn_expression *right = nullptr;

        // parse child
        right = sn_bitand_expression::parse(lex);

        sn_bitand_expression *expr = new sn_bitand_expression();
        expr->addChild(left);
        expr->addChild(right);

        return expr;
    }
    else
        return left;
}
sn_expression *sn_eq_expression::parse(Lexer &lex)
{
    sn_expression *left = sn_rel_expression::parse(lex);
    if (lex.peakNext().type == REL_EQ || lex.peakNext().type == REL_NE)
    {
        sn_expression *right = nullptr;
        TokenType op = NONE;

        // parse child
        op = lex.getNext().type;
        right = sn_eq_expression::parse(lex);

        sn_eq_expression *expr = new sn_eq_expression();
        expr->addChild(left);
        expr->op = op;
        expr->addChild(right);

        return expr;
    }
    else
        return left;
}
sn_expression *sn_rel_expression::parse(Lexer &lex)
{
    sn_expression *left = sn_shift_expression::parse(lex);
    if (lex.peakNext().category == TOKEN_CATEGORY_RELATIONAL_OPERATOR)
    {
        sn_expression *right = nullptr;
        TokenType op = NONE;

        // parse child
        op = lex.getNext().type;
        right = sn_rel_expression::parse(lex);

        sn_rel_expression *expr = new sn_rel_expression();
        expr->addChild(left);
        expr->op = op;
        expr->addChild(right);

        return expr;
    }
    else
        return left;
}
sn_expression *sn_shift_expression::parse(Lexer &lex)
{
    sn_expression *left = sn_add_expression::parse(lex);
    if (lex.peakNext().type == BIT_SLEFT || lex.peakNext().type == BIT_SRIGHT)
    {
        sn_expression *right = nullptr;
        TokenType op = NONE;

        // parse child
        op = lex.getNext().type;
        right = sn_shift_expression::parse(lex);

        sn_shift_expression *expr = new sn_shift_expression();
        expr->addChild(left);
        expr->op = op;
        expr->addChild(right);

        return expr;
    }
    else
        return left;
}
sn_expression *sn_add_expression::parse(Lexer &lex)
{
    sn_expression *left =
        sn_mul_expression::parse(lex, sn_cast_expression::parse(lex));
    if (lex.peakNext().type == OP_ADD || lex.peakNext().type == OP_SUB)
    {
        sn_expression *right = nullptr;
        TokenType op = NONE;

        // parse child
        op = lex.getNext().type;
        right = sn_add_expression::parse(lex);

        sn_add_expression *expr = new sn_add_expression();
        expr->addChild(left);
        expr->op = op;
        expr->addChild(right);

        return expr;
    }
    else
        return left;
}
sn_expression *sn_mul_expression::parse(Lexer &lex, sn_expression *left)
{
    assert(left != nullptr);
    // sn_expression *left = sn_cast_expression::parse(lex);

    TokenType tt = lex.peakNext().type;
    if (tt == OP_MUL || tt == OP_DIV || tt == OP_MOD)
    {
        sn_mul_expression *expr = new sn_mul_expression();
        expr->addChild(left);
        expr->op = lex.getNext().type;
        expr->addChild(sn_cast_expression::parse(lex));

        return parse(lex, expr);
    }
    else
        return left;
}
// Explicit Type Conversion
sn_expression *sn_cast_expression::parse(Lexer &lex)
{
    if (lex.peakNext().type == LP && FIRST(SN_TYPE_NAME, lex.peakNext(1)))
    {
        sn_cast_expression *ce = new sn_cast_expression();

        EXPECT(LP);
        ce->addChild(sn_type_name::parse(lex));
        EXPECT(RP);
        ce->addChild(sn_cast_expression::parse(lex));

        return ce;
    }
    else
    {
        return sn_unary_expression::parse(lex);
    }
}
// unary expression <- Value Transformation
sn_expression *sn_unary_expression::parse(Lexer &lex)
{
    if (FIRST(SN_UNARY_EXPRESSION, lex.peakNext()) &&
        !FIRST(SN_POSTFIX_EXPRESSION, lex.peakNext()))
    {
        sn_unary_expression *expr = new sn_unary_expression();
        expr->op = NONE;
        switch (lex.peakNext().type)
        {
            case OP_INC:
            case OP_DEC:
                expr->op = lex.getNext().type;
                expr->addChild(sn_unary_expression::parse(lex));
                // expr->type_ = expr->e->type();
                break;
            case SIZEOF:
                expr->op = lex.getNext().type;
                if (SKIP(LP))
                {
                    expr->addChild(sn_type_name::parse(lex));
                    EXPECT(RP);
                }
                else
                    expr->addChild(sn_unary_expression::parse(lex));
                break;
            case BIT_AND:  // unary op: & * + - ~ !
            case OP_MUL:
            case OP_ADD:
            case OP_SUB:
            case BIT_NOT:
            case BOOL_NOT:
                expr->op = lex.getNext().type;
                expr->addChild(sn_cast_expression::parse(lex));
                // assert(expr->e != nullptr);
                // EXPECT_TYPE_IS(expr->e->type(), T_POINTER);
                // expr->type_ = expr->e->type()->asPointerType()->target();
                break;
            default: SyntaxError("unexpected token."); break;
        }
        return expr;
    }
    else
    {
        return sn_postfix_expression::parse(lex,
                                            sn_primary_expression::parse(lex));
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
bool IsPostfixOperator(TokenType t)
{
    bool result = false;
    switch (t)
    {
        case LSB:
        case LP:
        case REFER_TO:
        case POINT_TO:
        case OP_INC:
        case OP_DEC: result = true; break;
        default: break;
    }
    return result;
}
sn_expression *sn_postfix_expression::parse(Lexer &lex, sn_expression *left)
{
    assert(left != nullptr);

    // if (FIRST(SN_POSTFIX_EXPRESSION, lex.peakNext()))
    if (IsPostfixOperator(lex.peakNext().type))
    {
        sn_postfix_expression *expr = new sn_postfix_expression();

        expr->addChild(left);
        expr->op = lex.peakNext().type;
        switch (lex.getNext().type)
        {
            case LSB:
                // EXPECT_TYPE_WITH(expr->target->type(), TOp_INDEX);
                expr->addChild(sn_comma_expression::parse(lex));
                // EXPECT_TYPE_IS(expr->index->type(), T_INT);
                // if (dynamic_cast<Indexable *>(expr->target->type()))
                //     expr->type_ =
                //         dynamic_cast<Indexable *>(expr->target->type())
                //             ->indexedType();
                EXPECT(RSB);
                break;
            case LP:
                // EXPECT_TYPE_IS(expr->target->type(), T_FUNCTION);
                // EXPECT_TYPE_WITH(expr->target->type(), TOp_CALL);
                if (lex.peakNext().type != RP)
                    expr->addChild(sn_argument_expression_list::parse(lex));
                // expr->type_ =
                //     dynamic_cast<FuncType *>(expr->target->type())->rtype();
                EXPECT(RP);
                break;
            case REFER_TO:
                // EXPECT_TYPE_WITH(expr->target->type(), TOp_OFFSET);
                expr->addChild(sn_identifier::parse(lex));
                break;
            case POINT_TO:
                // EXPECT_TYPE_IS(expr->target->type(), T_POINTER);
                // EXPECT_TYPE_WITH(
                //     expr->target->type()->asPointerType()->target(),
                //     TOp_OFFSET);
                expr->addChild(sn_identifier::parse(lex));
                // expr->type_ = dynamic_cast<StructType
                // *>(expr->target->type())
                //                   ->getMember(expr->member)
                //                   ->type;
                break;
            case OP_INC:
                // EXPECT_TYPE_WITH(expr->target->type(), TOp_INC);
                // expr->type_ = expr->target->type();
                break;
            case OP_DEC:
                // EXPECT_TYPE_WITH(expr->target->type(), TOp_DEC);
                // expr->type_ = expr->target->type();
                break;
            default: SyntaxError("Should not reach here."); break;
        }

        return parse(lex, expr);
    }
    else
    {
        return left;
    }
}
// unary expression <- Value Transformation
sn_expression *sn_primary_expression::parse(Lexer &lex)
{
    sn_expression *e = nullptr;
    sn_primary_expression *p = nullptr;

    switch (lex.peakNext().type)
    {
        case CONST_CHAR:
        case CONST_INT:
        case CONST_FLOAT:
        case STRING:
            p = new sn_primary_expression();
            p->t = lex.getNext();
            e = p;
            break;
        case SYMBOL:
            p = new sn_primary_expression();
            p->t = lex.getNext();
            p->addChild(sn_identifier::parse(lex));
            e = p;
            break;
        case LP:
            lex.getNext();
            e = sn_comma_expression::parse(lex);
            EXPECT(RP);
            break;
        default: SyntaxErrorEx("Unsupported primary expression"); break;
    }

    return e;
}

sn_argument_expression_list *sn_argument_expression_list::parse(Lexer &lex)
{
    sn_argument_expression_list *ael = new sn_argument_expression_list();

    do
    {
        ael->addChild(sn_assign_expression::parse(lex));
    } while (SKIP(OP_COMMA));

    return ael;
}

sn_const_expression *sn_const_expression::parse(Lexer &lex)
{
    sn_const_expression *ce = new sn_const_expression();
    ce->addChild(sn_cond_expression::parse(lex));
    return ce;
}

// --------------------------------------------------------------------------
// visit()/afterChildren()
// --------------------------------------------------------------------------

// helper classes and functions

class TypeSpecifiersBuilder
{
    // collect type-specifier set
    // bits:
    //      void,char,short,int,
    //      long,float,double,signed
    //      unsigned,struct/union,enum,typedef-name
    int type_specifier_allow, type_specifier_has;
    // for struct/union, enum, typedef-name
    Type *_type;

   public:
    TypeSpecifiersBuilder()
        : type_specifier_allow(0xffffFFFF),
          type_specifier_has(0),
          _type(nullptr)
    {
    }
    void reset()
    {
        type_specifier_allow = 0xffffFFFF;
        type_specifier_has = 0;
        _type = nullptr;
    }
    void feed_type_specifiers(TokenType t, Type *type, Environment *env);

    Type *build() const;
};

class TypeQualifiersBuilder
{
    // collect type-qualifier
    // bits:
    //      const,volatile
    int type_qualifier_has;

   public:
    TypeQualifiersBuilder() : type_qualifier_has(0) {}
    void reset()
    {
        type_qualifier_has = 0;
    }
    void feed_type_qualifiers(TokenType t);

    int build() const;
};

void TypeSpecifiersBuilder::feed_type_specifiers(TokenType t, Type *type,
                                                 Environment *env)
{
#define __check_set(value)                                                     \
    if ((type_specifier_allow & (value)) == 0 || type_specifier_has & (value)) \
        SyntaxError("DeclarationSpecifiersBuilder: unexpected token");         \
    else                                                                       \
        type_specifier_has |= (value);

    switch (t)
    {
        case TYPE_VOID:
            __check_set(0x1);
            type_specifier_allow &= 0x1;
            break;
        case TYPE_CHAR:
            __check_set(0x2);
            type_specifier_allow &= 0x182;  // char, signed, unsigned
            break;
        case TYPE_SHORT:
            __check_set(0x4);
            type_specifier_allow &= 0x18c;  // short, signed, unsigned, int
            break;
        case TYPE_INT:
            __check_set(0x8);
            type_specifier_allow &=
                0x19c;  // int, signed, unsigned, short, long
            break;
        case TYPE_LONG:
            __check_set(0x10);
            type_specifier_allow &=
                0x1d8;  // long, signed, unsigned, int, double
            break;
        case TYPE_FLOAT:
            __check_set(0x20);
            type_specifier_allow &= 0x20;
            break;
        case TYPE_DOUBLE:
            __check_set(0x40);
            type_specifier_allow &= 0x50;  // double, long
            break;
        case SIGNED:
            __check_set(0x80);
            type_specifier_allow &= 0x9e;  // signed, char, short, int, long
            break;
        case UNSIGNED:
            __check_set(0x100);
            type_specifier_allow &= 0x11e;  // unsigned, char, short, int, long
            break;
        case TYPE_STRUCT:
        case TYPE_UNION:
            __check_set(0x200);
            type_specifier_allow &= 0x200;
            _type = type;
            break;
        case TYPE_ENUM:
            __check_set(0x400);
            type_specifier_allow &= 0x400;
            _type = type;
            break;
        case SYMBOL:
            __check_set(0x800);
            type_specifier_allow &= 0x800;
            _type = type;
            break;
        default:
            SyntaxError("DeclarationSpecifiersBuilder: unexpected token");
            break;
    }

#undef __check_set

    assert((~type_specifier_allow & type_specifier_has) == 0);
}
void TypeQualifiersBuilder::feed_type_qualifiers(TokenType t)
{
    switch (t)
    {
        case CONST:
            if (type_qualifier_has & TP_CONST)
                SyntaxWarning("duplicate type qualifier 'const'");
            type_qualifier_has |= TP_CONST;
            break;
        case VOLATILE:
            if (type_qualifier_has & TP_VOLATILE)
                SyntaxWarning("duplicate type qualifier 'volatile'");
            type_qualifier_has |= TP_VOLATILE;
            break;
        default:
            SyntaxError("DeclarationSpecifiersBuilder: unexpected token");
            break;
    }
}

Type *TypeSpecifiersBuilder::build() const
{
    if (type_specifier_has & 0xe00)  // struct/union/enum/typedef
        return _type;
    else if (type_specifier_has == 0x1)  // void
        return new VoidType();
    else if (type_specifier_has == 0x2)  // char
        return new CharType();
    else if (type_specifier_has == 0x20)  // float
        return new FloatingType("f");
    else if (type_specifier_has & 0x40)  // double/long double
        return new FloatingType((type_specifier_has & 0x10) ? "ld" : "d");
    else
    {
        char *desc = new char[4];
        char *d = desc;
        if (type_specifier_has & 0x80)
            *(d++) = 'S';
        if (type_specifier_has & 0x100)
            *(d++) = 'U';
        if (type_specifier_has & 0x2)
            *(d++) = 'c';
        if (type_specifier_has & 0x4)
            *(d++) = 's';
        if (type_specifier_has & 0x10)
            *(d++) = 'l';
        if (type_specifier_has & 0x8)
            *(d++) = 'i';
        *d = '\0';
        return new IntegerType(desc);
    }
}
int TypeQualifiersBuilder::build() const
{
    return type_qualifier_has;
}

// --------------------------------------------------------------------------
const char *DebugSyntaxNode(ESyntaxNodeType nt);

ESymbolScope SyntaxNode::getScope() const
{
    // which is first meet ?
    //  sn_struct_declaration_list,
    //  sn_compound_statement       => block
    //  parameter-type-list,
    //  identifier_list             => func_proto (valid in function body)
    //  sn_translation_unit         => file
    ESymbolScope scope = SYMBOL_SCOPE_none;

    SyntaxNode *node = parent();
    bool done = false;
    while (!done && node != nullptr)
    {
        done = true;
        switch (node->nodeType())
        {
            case SN_STRUCT_DECLARATION_LIST:
            case SN_COMPOUND_STATEMENT: scope = SYMBOL_SCOPE_block; break;
            case SN_PARAMETER_TYPE_LIST:
            case SN_IDENTIFIER_LIST: scope = SYMBOL_SCOPE_func_proto; break;
            case SN_TRANSLATION_UNIT: scope = SYMBOL_SCOPE_file; break;
            case SN_LABEL_STATEMENT:  // XXX: may cause bug here
                scope = SYMBOL_SCOPE_func;
                break;
            default: done = false; break;
        }
        node = node->parent();
    }

    return scope;
}
void SyntaxNode::visit(ParserParams &params)
{
    beforeChildren(params);
    for (auto *child : all())
    {
        child->visit(params);
    }
    afterChildren(params);
}

void SyntaxNode::beforeChildren(ParserParams &params)
{
    // SyntaxWarning(std::string(DebugSyntaxNode(nodeType())) +
    //               ": beforeChildren() not implemented");
}
void SyntaxNode::afterChildren(ParserParams &params)
{
    // SyntaxWarning(std::string(DebugSyntaxNode(nodeType())) +
    //               ": afterChildren() not implemented");
}

// void sn_translation_unit::afterChildren(ParserParams &params);
// void sn_external_declaration::afterChildren(Environment *&env, const int
// pass);

void sn_function_definition::visit(ParserParams &params)
{
    assert(count() >= 1);

    size_t i = 0;
    SyntaxNode *child = at(i++);

    // declaration_specifiers
    if (child->nodeType() == SN_DECLARATION_SPECIFIERS)
    {
        child->visit(params);
        child = at(i++);
    }

    // declarator, declaration_list
    beforeParamList(params);
    {
        child->visit(params);
        child = at(i++);
        if (child->nodeType() == SN_DECLARATION_LIST)
        {
            child->visit(params);
            child = at(i++);
        }
    }
    afterParamList(params);

    child->visit(params);
    afterChildren(params);
}
void sn_function_definition::beforeParamList(ParserParams &params)
{
    if (params.goal == GOAL_symbol_binding)
    {
        CREATE_SYMBOL_TABLE();
    }

    SET_SYMBOL_TABLE(SYMBOL_TABLE());
}
void sn_function_definition::afterParamList(ParserParams &params)
{
    if (params.goal == GOAL_symbol_binding)
    {
        Type *type = OPTIONAL(SN_DECLARATION_SPECIFIERS)
                        .TYPE_INFO() +
                     REQUIRED(SN_DECLARATOR)
                        .TYPE_INFO();
        StringRef name = REQUIRED(SN_DECLARATOR).NAME_INFO();

        // Same name symbol in file scope.
        const Symbol *same = FIND_SYMBOL(GLOBAL_SYMBOL_TABLE(), name);
        auto linkage = same ? same->linkage : SYMBOL_LINKAGE_external;

        ADD_SYMBOL(CreateSymbol()
                .WithScope(getScope())
                .WithType(type)
                .WithName(name)
                .WithSNSIFS(linkage).Get(),
                PARENT_SYMBOL_TABLE());
    }
}
void sn_function_definition::afterChildren(ParserParams &params)
{
    if (params.goal == GOAL_symbol_binding)
    {
        // nothing to do
    }

    SET_SYMBOL_TABLE(PARENT_SYMBOL_TABLE());
}

void sn_declaration::afterChildren(ParserParams &params)
{
    if (params.goal == GOAL_symbol_binding)
    {
        if (count() != 2)
            return;  // no symbol defined

        SymbolBuilder builder;

        Type *decl_spec = TYPE_INFO(first());
        TokenType storage = STORAGE_INFO(first());
        builder.setStorageSpecifier(storage);
        for (auto *child : last()->all())
        {
            Symbol *symbol = nullptr;
            {
                EXPECT_NODE(SN_INIT_DECLARATOR);
                Type *type = TypeUtil::Concatenate(TYPE_INFO(child), decl_spec);
                StringRef name = NAME_INFO(child);

                const Symbol *same =
                    SymbolTable::SameNameSymbolInFileScope(env, type, name);

                builder.setScope(child->getScope());
                builder.setType(type);
                builder.setName(name,
                                same ? same->linkage : SYMBOL_LINKAGE_external);
                symbol = builder.build();
            }
            assert(symbol != nullptr);

            ADD_SYMBOL(symbol);
        }
    }
}
// void sn_declaration_list::afterChildren(ParserParams &params) {}

// type, name, code
void sn_init_declarator::afterChildren(ParserParams &params)
{
    if (params.goal == GOAL_symbol_binding)
    {
        EXPECT_NODE(first(), SN_DECLARATOR);
        SET_TYPE_INFO(TYPE_INFO(first()));
        SET_NAME_INFO(NAME_INFO(first()));
        if (count() == 2)
            SyntaxError("initializer not implemented");
    }
}
// vector<type, name, expression>
// void sn_init_declarator_list::afterChildren(Environment *&env, const int
// pass) {}

// type, name
void sn_declarator::afterChildren(ParserParams &params)
{
    if (params.goal == GOAL_symbol_binding)
    {
        Type *pointer =
            IS_NODE(first(), SN_POINTER) ? TYPE_INFO(first()) : nullptr;

        EXPECT_NODE(last(), SN_DIRECT_DECLARATOR);
        SET_TYPE_INFO(TypeUtil::Concatenate(TYPE_INFO(last()), pointer));
        SET_NAME_INFO(NAME_INFO(last()));
        // Env::CheckRedecl(name_info_);
    }
}
void sn_direct_declarator::afterChildren(ParserParams &params)
{
    if (params.goal == GOAL_symbol_binding)
    {
        SET_TYPE_INFO(nullptr);
        for (auto *child : all())
        {
            switch (child->nodeType())
            {
                case SN_IDENTIFIER:
                    // assert only set once.
                    SET_NAME_INFO(NAME_INFO(child));
                    break;
                case SN_DECLARATOR:
                    SET_TYPE_INFO(TYPE_INFO(child));
                    SET_NAME_INFO(NAME_INFO(child));
                    break;
                case SN_CONST_EXPRESSION:
                    if (child->isLeaf())
                        SET_TYPE_INFO(TypeUtil::Concatenate(TYPE_INFO(),
                                                            new ArrayType()));
                    else
                        SET_TYPE_INFO(TypeUtil::Concatenate(
                            TYPE_INFO(), new ArrayType(VALUE_INFO(child))));
                    break;
                case SN_PARAMETER_TYPE_LIST:
                case SN_IDENTIFIER_LIST:
                    SET_TYPE_INFO(
                        TypeUtil::Concatenate(TYPE_INFO(), TYPE_INFO(child)));
                    break;
                default: break;
            }
        }
    }
}

// type
void sn_abstract_declarator::afterChildren(ParserParams &params)
{
    if (params.goal == GOAL_symbol_binding)
    {
        if (IS_NODE(first(), SN_POINTER))
        {
            SET_TYPE_INFO(TYPE_INFO(first()));
        }

        if (IS_NODE(last(), SN_DIRECT_ABSTRACT_DECLARATOR))
        {
            SET_TYPE_INFO(
                TypeUtil::Concatenate(TYPE_INFO(last()), TYPE_INFO()));
        }
    }
}
void sn_direct_abstract_declarator::afterChildren(ParserParams &params)
{
    if (params.goal == GOAL_symbol_binding)
    {
        if (IS_NODE(first(), SN_ABSTRACT_DECLARATOR))
        {
            SET_TYPE_INFO(TYPE_INFO(first()));
        }

        for (auto *child : all())
        {
            if (IS_NODE(child, SN_CONST_EXPRESSION))
            {
                if (child->isLeaf())
                    SET_TYPE_INFO(
                        TypeUtil::Concatenate(TYPE_INFO(), new ArrayType()));
                else
                    SET_TYPE_INFO(TypeUtil::Concatenate(
                        TYPE_INFO(), new ArrayType(VALUE_INFO(child))));
            }
            else if (IS_NODE(child, SN_PARAMETER_TYPE_LIST))
            {
                SET_TYPE_INFO(
                    TypeUtil::Concatenate(TYPE_INFO(), TYPE_INFO(child)));
            }
        }
    }
}

// expression(constant-ness)
// void sn_initializer::afterChildren(ParserParams &params);
// void sn_initializer_list::afterChildren(ParserParams &params);

// type(FuncType)
void sn_parameter_type_list::afterChildren(ParserParams &params)
{
    if (params.goal == GOAL_symbol_binding)
    {
        EXPECT_NODE(first(), SN_PARAMETER_LIST);
        SET_TYPE_INFO(TYPE_INFO(first()));
        if (varlist)
            dynamic_cast<FuncType *>(type_info_)->setVarArg();
    }
}
void sn_parameter_list::afterChildren(ParserParams &params)
{
    if (params.goal == GOAL_symbol_binding)
    {
        FuncType *func = new FuncType();
        for (auto *child : all())
        {
            EXPECT_NODE(child, SN_PARAMETER_DECLARATION);
            sn_parameter_declaration *pd =
                dynamic_cast<sn_parameter_declaration *>(child);
            TYPE_INFO(child)->setLvalue();
            func->addParam(TYPE_INFO(child), NAME_INFO(child));
        }
        SET_TYPE_INFO(func);
    }
}
// type, name
void sn_parameter_declaration::afterChildren(ParserParams &params)
{
    if (params.goal == GOAL_symbol_binding)
    {
        EXPECT_NODE(first(), SN_DECLARATION_SPECIFIERS);
        SET_TYPE_INFO(TYPE_INFO(first()));

        if (count() == 2)
        {
            if (IS_NODE(last(), SN_DECLARATOR))
            {
                SET_TYPE_INFO(
                    TypeUtil::Concatenate(TYPE_INFO(last()), TYPE_INFO()));
                SET_NAME_INFO(NAME_INFO(last()));
            }
            else
            {
                EXPECT_NODE(last(), SN_ABSTRACT_DECLARATOR);
                SET_TYPE_INFO(
                    TypeUtil::Concatenate(TYPE_INFO(last()), TYPE_INFO()));
            }
        }
    }
}

// type, linkage(storage_info_)
void sn_declaration_specifiers::afterChildren(ParserParams &params)
{
    if (params.goal == GOAL_symbol_binding)
    {
        TypeSpecifiersBuilder sb;
        TypeQualifiersBuilder qb;

        for (auto *child : all())
        {
            switch (child->nodeType())
            {
                case SN_STORAGE_SPECIFIER:
                    SET_STORAGE_INFO(STORAGE_INFO(child));
                    break;
                case SN_TYPE_SPECIFIER:
                    sb.feed_type_specifiers(TOKEN_INFO(child), TYPE_INFO(child),
                                            SYMBOL_TABLE());
                    break;
                case SN_TYPE_QUALIFIER:
                    qb.feed_type_qualifiers(TOKEN_INFO(child));
                    break;
                default: break;
            }
        }

        SET_TYPE_INFO(sb.build());
        TYPE_INFO()->setQualifier(qb.build());
    }
}
// type
void sn_specifier_qualifier_list::afterChildren(ParserParams &params)
{
    if (params.goal == GOAL_symbol_binding)
    {
        TypeSpecifiersBuilder sb;
        TypeQualifiersBuilder qb;

        for (auto *child : all())
        {
            switch (child->nodeType())
            {
                case SN_TYPE_SPECIFIER:
                    sb.feed_type_specifiers(TOKEN_INFO(child), TYPE_INFO(child),
                                            SYMBOL_TABLE());
                    break;
                case SN_TYPE_QUALIFIER:
                    qb.feed_type_qualifiers(TOKEN_INFO(child));
                    break;
                default: break;
            }
        }

        SET_TYPE_INFO(sb.build());
        TYPE_INFO()->setQualifier(qb.build());
    }
}
// void sn_storage_specifier::afterChildren(ParserParams &params)
// {}
// void sn_type_qualifier::afterChildren(ParserParams &params) {}
// void sn_type_qualifier_list::afterChildren(ParserParams &params)
// {}
void sn_type_specifier::afterChildren(ParserParams &params)
{
    if (params.goal == GOAL_symbol_binding)
    {
        switch (TOKEN_INFO())
        {
            case TYPE_STRUCT:
            case TYPE_UNION:
            case TYPE_ENUM:
            case SYMBOL: SET_TYPE_INFO(TYPE_INFO(first())); break;
            default: break;
        }
    }
}
void sn_struct_union_specifier::visit(ParserParams &params)
{
    SyntaxNode *child = first();

    if (child->nodeType() == SN_IDENTIFIER)
    {
        child->visit(params);
        child = count() == 2 ? last() : nullptr;
    }

    afterTag(params);

    if (child != nullptr && child->nodeType() == SN_STRUCT_DECLARATION_LIST)
    {
        beforeDefinition(params);
        child->visit(params);
        afterDefinition(params);
    }

    afterChildren(params);
}
void sn_struct_union_specifier::afterTag(ParserParams &params)
{
    if (params.goal == GOAL_symbol_binding)
    {
        // tag info
        StringRef tag;
        {
            if (first()->nodeType() == SN_IDENTIFIER)
                tag = NAME_INFO(first());
            else
                tag = TypeUtil::GenerateTag();
        }
        assert(!tag.empty());

        Symbol *s = FIND_SYMBOL(tag, SYMBOL_NAMESPACE_tag);
        if (s == nullptr)
        {
            SET_TYPE_INFO(new TagType(
                (TOKEN_INFO() == TYPE_STRUCT ? T_STRUCT : T_UNION), tag));
            // add tag to env
            SymbolBuilder builder;
            builder.setNamespace(SYMBOL_NAMESPACE_tag);
            builder.setScope(getScope());
            builder.setName(tag);  // TODO: tag doesn't need linkage
            builder.setType(TYPE_INFO());
            ADD_SYMBOL(builder.build());
        }
        else
        {
            SET_TYPE_INFO(s->type);

            // merge or error.
            assert(TYPE_INFO() != nullptr && TYPE_INFO()->getClass() == T_TAG);
            const Type *impl = dynamic_cast<TagType *>(TYPE_INFO())->getImpl();
            if (impl != nullptr &&
                ((TOKEN_INFO() == TYPE_STRUCT &&
                  impl->getClass() != T_STRUCT) ||
                 (TOKEN_INFO() == TYPE_UNION && impl->getClass() != T_UNION)))
            {
                SyntaxError("tag redefined with different type: " +
                            tag.toString());
            }
        }
    }
}
void sn_struct_union_specifier::beforeDefinition(ParserParams &params)
{
    if (params.goal == GOAL_symbol_binding)
    {
        CREATE_SYMBOL_TABLE();
    }

    SET_SYMBOL_TABLE(SYMBOL_TABLE());
}
void sn_struct_union_specifier::afterDefinition(ParserParams &params)
{
    if (params.goal == GOAL_symbol_binding)
    {
    }

    SET_SYMBOL_TABLE(PARENT_SYMBOL_TABLE());
}
void sn_struct_union_specifier::afterChildren(ParserParams &params)
{
    if (params.goal == GOAL_symbol_binding)
    {
        // struct tag impl info
        if (IS_NODE(last(), SN_STRUCT_DECLARATION_LIST))
        {
            StructTypeImpl *st_impl =
                new StructTypeImpl(TOKEN_INFO() == TYPE_UNION);
            for (auto *child : last()->all())
            {
                EXPECT_NODE(child, SN_STRUCT_DECLARATION);
                for (auto *symbol : ALL_LOCAL_SYMBOL(child))
                {
                    st_impl->addMember(symbol->name, symbol->type);
                }
            }
            dynamic_cast<TagType *>(TYPE_INFO())->setImpl(st_impl);
        }
    }
}
void sn_enum_specifier::visit(ParserParams &params)
{
    SyntaxNode *child = first();

    if (child->nodeType() == SN_IDENTIFIER)
    {
        child->visit(params);
        child = count() == 2 ? last() : nullptr;
    }

    afterTag(params);

    if (child != nullptr && child->nodeType() == SN_ENUMERATOR_LIST)
    {
        child->visit(params);
    }

    afterChildren(params);
}
void sn_enum_specifier::afterTag(ParserParams &params)
{
    if (params.goal == GOAL_symbol_binding)
    {
        // tag info
        StringRef tag;
        {
            if (first()->nodeType() == SN_IDENTIFIER)
                tag = NAME_INFO(first());
            else
                tag = TypeUtil::GenerateTag();
        }
        SET_TYPE_INFO(new TagType(T_ENUM, tag));

        // add tag to env
        SymbolBuilder builder;
        builder.setNamespace(SYMBOL_NAMESPACE_tag);
        builder.setScope(getScope());
        builder.setName(tag);  // TODO: tag doesn't need linkage
        builder.setType(type_info_);
        ADD_SYMBOL(builder.build());
    }
}
void sn_enum_specifier::afterChildren(ParserParams &params)
{
    if (params.goal == GOAL_symbol_binding)
    {
        // enum tag impl info
        if (IS_NODE(last(), SN_ENUMERATOR_LIST))
        {
            EnumTypeImpl *et_impl = new EnumTypeImpl();
            int value = 0;

            SymbolBuilder builder;
            for (auto *child : last()->all())
            {
                EXPECT_NODE(child, SN_ENUMERATOR);
                if (IS_NODE(child->last(), SN_CONST_EXPRESSION))
                    value = VALUE_INFO(child->last());

                EnumConstType *e_const =
                    new EnumConstType(NAME_INFO(child), value++);

                // add enumeration const to symbol table
                builder.setScope(getScope());
                builder.setName(name);
                builder.setType(e_const);
                ADD_SYMBOL(builder.build());

                et_impl->addMember(e_const);
            }
            dynamic_cast<TagType *>(TYPE_INFO())->setImpl(et_impl);
        }
    }
}
void sn_struct_declaration::afterChildren(ParserParams &params)
{
    if (params.goal == GOAL_symbol_binding)
    {
        EXPECT_NODE(first(), SN_SPECIFIER_QUALIFIER_LIST);
        Type *decl_spec = TYPE_INFO(first());

        SymbolBuilder builder;
        for (auto *child : last()->all())
        {
            EXPECT_NODE(child, SN_STRUCT_DECLARATOR);

            builder.setName(NAME_INFO(child));
            builder.setType(TypeUtil::Concatenate(TYPE_INFO(child), decl_spec));
            builder.setScope(SCOPE_INFO(child));

            ADD_SYMBOL(builder.build());
        }
    }
}
// void sn_struct_declaration_list::afterChildren(Environment *&env, const int
// pass) {}
void sn_struct_declarator::afterChildren(ParserParams &params)
{
    if (params.goal == GOAL_symbol_binding)
    {
        if (IS_NODE(first(), SN_DECLARATOR))
        {
            SET_TYPE_INFO(TYPE_INFO(first()));
            SET_NAME_INFO(NAME_INFO(first()));
        }
        if (IS_NODE(last(), SN_CONST_EXPRESSION))
        {
            SyntaxError("bit-field not implemented.");
        }
    }
}
// void sn_struct_declarator_list::afterChildren(Environment *&env, const int
// pass) {}
// void sn_enumerator_list::afterChildren(ParserParams &params) {}
// void sn_enumerator::afterChildren(ParserParams &params) {}
// void sn_enumeration_constant::afterChildren(Environment *&env, const int
// pass) {}
void sn_type_name::afterChildren(ParserParams &params)
{
    if (params.goal == GOAL_symbol_binding)
    {
        EXPECT_NODE(first(), SN_SPECIFIER_QUALIFIER_LIST);
        SET_TYPE_INFO(TYPE_INFO(first()));
        if (NODE_IS(last(), SN_ABSTRACT_DECLARATOR))
        {
            SET_TYPE_INFO(TYPE_INFO(last()));
        }
    }
}
// type
void sn_pointer::afterChildren(ParserParams &params)
{
    if (params.goal == GOAL_symbol_binding)
    {
        SET_TYPE_INFO(new PointerType());

        TypeQualifiersBuilder qb;
        if (count() != 0)
        {
            if (NODE_IS(first(), SN_TYPE_QUALIFIER_LIST))
            {
                for (auto *child : first()->all())
                {
                    EXPECT_NODE(child, SN_TYPE_QUALIFIER);
                    qb.feed_type_qualifiers(TYPE_INFO(child));
                }
            }
            TYPE_INFO()->setQualifier(qb.build());

            if (NODE_IS(last(), SN_POINTER))
            {
                SET_TYPE_INFO(
                    TypeUtil::Concatenate(TYPE_INFO(last()), TYPE_INFO()));
            }
        }
    }
}

// name
void sn_identifier::afterChildren(ParserParams &params)
{
    if (params.goal == GOAL_symbol_binding)
    {
        SET_NAME_INFO(id.symbol);
    }
}
void sn_identifier_list::afterChildren(ParserParams &params)
{
    if (params.goal == GOAL_symbol_binding)
    {
        // Should this node know it can only appear in function definition?
        /* FuncType *func = new FuncType();
        for (auto *child : all())
        {
            func->addParam(nullptr,
                           dynamic_cast<sn_identifier *>(child)->name_info_);
        }
        type_info_ = func; */
    }
}
void sn_typedef_name::afterChildren(ParserParams &params)
{
    if (params.goal == GOAL_symbol_binding)
    {
        EXPECT_NODE(first(), SN_IDENTIFIER);
        StringRef name = NAME_INFO(first());
        Symbol *s = FIND_SYMBOL(SYMBOL_NAMESPACE_id, name);
        if (s == nullptr || s->type == nullptr ||
                s->type->getClass() != T_TYPEDEF)
            SyntaxError(name.toString() + " is not a typedef name.");

        SET_TYPE_INFO(s->type);
    }
}

// statement
// void sn_statement::afterChildren(ParserParams &params) {}
// void sn_statement_list::afterChildren(ParserParams &params) {}
void sn_label_statement::afterChildren(ParserParams &params)
{
    if (params.goal == GOAL_symbol_binding)
    {
        if (NODE_IS(first(), SN_IDENTIFIER))
        {
            SymbolBuilder builder;
            builder.setName(NAME_INFO(first()));
            builder.setNamespace(SYMBOL_NAMESPACE_label);
            builder.setScope(SYMBOL_SCOPE_func);
            builder.setType(new LabelType());
            ADD_SYMBOL(builder.build());
        }
    }
}
void sn_compound_statement::visit(ParserParams &params)
{
    beforeChildren(params);
    if (count() > 0)
    {
        if (first()->nodeType() == SN_DECLARATION_LIST)
            first()->visit(params);
        afterDeclarations(params);
        if (last()->nodeType() == SN_STATEMENT_LIST)
            last()->visit(params);
    }
    afterChildren(params);
}
void sn_compound_statement::beforeChildren(ParserParams &params)
{
    if (params.goal == GOAL_symbol_binding)
    {
        CREATE_SYMBOL_TABLE();
    }

    SET_SYMBOL_TABLE(SYMBOL_TABLE());
}
// void sn_compound_statement::afterDeclarations(ParserParams &params);
void sn_compound_statement::afterChildren(ParserParams &params)
{
    SET_SYMBOL_TABLE(PARENT_SYMBOL_TABLE());
}
/*
void sn_expression_statement::afterChildren(ParserParams &params)
{
}
void sn_selection_statement::beforeChildren(ParserParams &params)
{
}
void sn_selection_statement::afterChildren(ParserParams &params)
{
}
void sn_iteration_statement::beforeChildren(ParserParams &params)
{
}
void sn_iteration_statement::afterChildren(ParserParams &params)
{
}
void sn_jump_statement::afterChildren(ParserParams &params)
{
}

// expression(constant-ness)
// DEBUG default, should not implement
void sn_expression::afterChildren(ParserParams &params)
{
}
void sn_comma_expression::afterChildren(ParserParams &params)
{
}
void sn_assign_expression::afterChildren(ParserParams &params)
{
}
void sn_cond_expression::afterChildren(ParserParams &params)
{
}

void sn_or_expression::afterChildren(ParserParams &params)
{
}
void sn_and_expression::afterChildren(ParserParams &params)
{
}

void sn_bitor_expression::afterChildren(ParserParams &params)
{
}
void sn_bitxor_expression::afterChildren(ParserParams &params)
{
}
void sn_bitand_expression::afterChildren(ParserParams &params)
{
}

void sn_eq_expression::afterChildren(ParserParams &params)
{
}
void sn_rel_expression::afterChildren(ParserParams &params)
{
}
void sn_shift_expression::afterChildren(ParserParams &params)
{
}
void sn_add_expression::afterChildren(ParserParams &params)
{
}
void sn_mul_expression::afterChildren(ParserParams &params)
{
}
void sn_cast_expression::afterChildren(ParserParams &params)
{
}
void sn_unary_expression::afterChildren(ParserParams &params)
{
}
void sn_postfix_expression::afterChildren(ParserParams &params)
{
}
void sn_primary_expression::afterChildren(ParserParams &params)
{
}
void sn_const_expression::afterChildren(ParserParams &params)
{
    if (params.goal == GOAL_symbol_binding)
    {
        // if (count() == 0)
        // {
        // }
        // else if (first()->nodeType() == SN_PRIMARY_EXPRESSION)
        // {
        //     sn_primary_expression *e =
        //         dynamic_cast<sn_primary_expression *>(first());
        //     _value = e->t.ival;
        // }
        // else
        // {
        //     SyntaxError("not implemented.");
        // }
    }
}
// void sn_argument_expression_list::afterChildren(Environment *&env, const int
// pass);
//
*/

// --------------------------------------------------------------------------
// toString()
// --------------------------------------------------------------------------
const char *DebugSyntaxNode(ESyntaxNodeType nt)
{
    static const char *names[] = {"none",
                                  "translation_unit",
                                  "external_declaration",
                                  "function_definition",
                                  "declaration",
                                  "declaration_list",
                                  "init_declarator",
                                  "init_declarator_list",
                                  "declarator",
                                  "direct_declarator",
                                  "abstract_declarator",
                                  "direct_abstract_declarator",
                                  "initializer",
                                  "initializer_list",
                                  "parameter_type_list",
                                  "parameter_list",
                                  "parameter_declaration",
                                  "declaration_specifiers",
                                  "specifier_qualifier_list",
                                  "storage_specifier",
                                  "type_qualifier",
                                  "type_qualifier_list",
                                  "type_specifier",
                                  "struct_union_specifier",
                                  "enum_specifier",
                                  "struct_declaration",
                                  "struct_declaration_list",
                                  "struct_declarator",
                                  "struct_declarator_list",
                                  "enumerator_list",
                                  "enumerator",
                                  "enumeration_constant",
                                  "type_name",
                                  "pointer",
                                  "identifier",
                                  "identifier_list",
                                  "typedef_name",
                                  "statement",
                                  "statement_list",
                                  "label_statement",
                                  "compound_statement",
                                  "expression_statement",
                                  "selection_statement",
                                  "iteration_statement",
                                  "jump_statement",
                                  "expression",
                                  "comma_expression",
                                  "assign_expression",
                                  "cond_expression",
                                  "or_expression",
                                  "and_expression",
                                  "bitor_expression",
                                  "bitxor_expression",
                                  "bitand_expression",
                                  "eq_expression",
                                  "rel_expression",
                                  "shift_expression",
                                  "add_expression",
                                  "mul_expression",
                                  "cast_expression",
                                  "unary_expression",
                                  "postfix_expression",
                                  "primary_expression",
                                  "argument_expression_list",
                                  "const_expression"};
    unsigned long i = 0UL, e = nt;
    while (e > 0UL)
    {
        ++i, e >>= 1;
    }
    return names[i];
}

std::string SyntaxNode::toString() const
{
    std::string s = DebugSyntaxNode(node_type_);
    const sn_expression *expr = dynamic_cast<const sn_expression *>(this);
    const sn_statement *stat = dynamic_cast<const sn_statement *>(this);
    if (expr)
    {
        if (expr->nodeType() == SN_CAST_EXPRESSION)
        {
            s += " [";
            s += dynamic_cast<const sn_expression *>(expr->last())
                     ->type_->toString();
            s += "] -~>";
        }
        s += " [";
        if (expr->type_)
            s += expr->type_->toString();
        s += ']';
        s += "\n{>\n";
        s += expr->code_info_.toString();
        s += "\n<}\n";
    }
    else if (stat)
    {
        s += "\n{>\n";
        s += stat->code_info_.toString();
        s += "\n<}\n";
    }
    else if (nodeType() == SN_IDENTIFIER)
    {
        s += " \"";
        s += dynamic_cast<const sn_identifier *>(this)->name_info_.toString();
        s += '"';
    }
    s += "\n>\n";
    for (auto *child : all())
        s += child->toString();
    s += "<\n";
    return s;
}