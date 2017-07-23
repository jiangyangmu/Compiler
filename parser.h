#pragma once

#include "codegen.h"
// #include "env.h"
#include "lexer.h"

#include <iostream>
#include <string>

class Environment
{
};
class Type
{
};

// BNF correctness
// [expression] internal node elimination

enum ESyntaxNodeType
{
    SN_NONE = 0UL,
    SN_TRANSLATION_UNIT = 1UL,
    SN_EXTERNAL_DECLARATION = (1UL << 1),
    SN_FUNCTION_DEFINITION = (1UL << 2),
    SN_DECLARATION = (1UL << 3),
    SN_DECLARATION_LIST = (1UL << 4),
    SN_INIT_DECLARATOR = (1UL << 5),
    SN_INIT_DECLARATOR_LIST = (1UL << 6),
    SN_DECLARATOR = (1UL << 7),
    SN_DIRECT_DECLARATOR = (1UL << 8),
    SN_ABSTRACT_DECLARATOR = (1UL << 9),
    SN_DIRECT_ABSTRACT_DECLARATOR = (1UL << 10),
    SN_INITIALIZER = (1UL << 11),
    SN_INITIALIZER_LIST = (1UL << 12),
    SN_PARAMETER_TYPE_LIST = (1UL << 13),
    SN_PARAMETER_LIST = (1UL << 14),
    SN_PARAMETER_DECLARATION = (1UL << 15),
    SN_DECLARATION_SPECIFIERS = (1UL << 16),
    SN_SPECIFIER_QUALIFIER_LIST = (1UL << 17),
    SN_STORAGE_SPECIFIER = (1UL << 18),
    SN_TYPE_QUALIFIER = (1UL << 19),
    SN_TYPE_QUALIFIER_LIST = (1UL << 20),
    SN_TYPE_SPECIFIER = (1UL << 21),
    SN_STRUCT_UNION_SPECIFIER = (1UL << 22),
    SN_ENUM_SPECIFIER = (1UL << 23),
    SN_STRUCT_DECLARATION = (1UL << 24),
    SN_STRUCT_DECLARATION_LIST = (1UL << 25),
    SN_STRUCT_DECLARATOR = (1UL << 26),
    SN_STRUCT_DECLARATOR_LIST = (1UL << 27),
    SN_ENUMERATOR_LIST = (1UL << 28),
    SN_ENUMERATOR = (1UL << 29),
    SN_ENUMERATION_CONSTANT = (1UL << 30),
    SN_TYPE_NAME = (1UL << 31),
    SN_POINTER = (1UL << 32),
    SN_IDENTIFIER = (1UL << 33),
    SN_IDENTIFIER_LIST = (1UL << 34),
    SN_TYPEDEF_NAME = (1UL << 35),
    SN_STATEMENT = (1UL << 36),
    SN_STATEMENT_LIST = (1UL << 37),
    SN_LABEL_STATEMENT = (1UL << 38),
    SN_COMPOUND_STATEMENT = (1UL << 39),
    SN_EXPRESSION_STATEMENT = (1UL << 40),
    SN_SELECTION_STATEMENT = (1UL << 41),
    SN_ITERATION_STATEMENT = (1UL << 42),
    SN_JUMP_STATEMENT = (1UL << 43),
    SN_EXPRESSION = (1UL << 44),
    SN_COMMA_EXPRESSION = (1UL << 45),
    SN_ASSIGN_EXPRESSION = (1UL << 46),
    SN_COND_EXPRESSION = (1UL << 47),
    SN_OR_EXPRESSION = (1UL << 48),
    SN_AND_EXPRESSION = (1UL << 49),
    SN_BITOR_EXPRESSION = (1UL << 50),
    SN_BITXOR_EXPRESSION = (1UL << 51),
    SN_BITAND_EXPRESSION = (1UL << 52),
    SN_EQ_EXPRESSION = (1UL << 53),
    SN_REL_EXPRESSION = (1UL << 54),
    SN_SHIFT_EXPRESSION = (1UL << 55),
    SN_ADD_EXPRESSION = (1UL << 56),
    SN_MUL_EXPRESSION = (1UL << 57),
    SN_CAST_EXPRESSION = (1UL << 58),
    SN_UNARY_EXPRESSION = (1UL << 59),
    SN_POSTFIX_EXPRESSION = (1UL << 60),
    SN_PRIMARY_EXPRESSION = (1UL << 61),
    SN_ARGUMENT_EXPRESSION_LIST = (1UL << 62),
    SN_CONST_EXPRESSION = (1UL << 63),
};

class sn_translation_unit;
class sn_external_declaration;
class sn_function_definition;
class sn_declaration;
class sn_declaration_list;
class sn_init_declarator;
class sn_init_declarator_list;
class sn_declarator;
class sn_direct_declarator;
class sn_abstract_declarator;
class sn_direct_abstract_declarator;
class sn_initializer;
class sn_initializer_list;
class sn_parameter_type_list;
class sn_parameter_list;
class sn_parameter_declaration;
class sn_declaration_specifiers;
class sn_specifier_qualifier_list;
class sn_storage_specifier;
class sn_type_qualifier;
class sn_type_qualifier_list;
class sn_type_specifier;
class sn_struct_union_specifier;
class sn_enum_specifier;
class sn_struct_declaration;
class sn_struct_declaration_list;
class sn_struct_declarator;
class sn_struct_declarator_list;
class sn_enumerator_list;
class sn_enumerator;
class sn_enumeration_constant;
class sn_type_name;
class sn_pointer;
class sn_identifier;
class sn_identifier_list;
class sn_typedef_name;
class sn_statement;
class sn_statement_list;
class sn_label_statement;
class sn_compound_statement;
class sn_expression_statement;
class sn_selection_statement;
class sn_iteration_statement;
class sn_jump_statement;
class sn_expression;
class sn_comma_expression;
class sn_assign_expression;
class sn_cond_expression;
class sn_or_expression;
class sn_and_expression;
class sn_bitor_expression;
class sn_bitxor_expression;
class sn_bitand_expression;
class sn_eq_expression;
class sn_rel_expression;
class sn_shift_expression;
class sn_add_expression;
class sn_mul_expression;
class sn_cast_expression;
class sn_unary_expression;
class sn_postfix_expression;
class sn_primary_expression;
class sn_argument_expression_list;
class sn_const_expression;

class SyntaxNode : public CodeGenerator
{
    ESyntaxNodeType node_type_;

   public:
    SyntaxNode(ESyntaxNodeType nt = SN_NONE) : node_type_(nt)
    {
    }
    ESyntaxNodeType nodeType() const
    {
        return node_type_;
    }

    virtual std::string debugString();
    // virtual void emit(Environment *env, EEmitGoal goal) const;
};

// Foundation
class sn_translation_unit : public SyntaxNode
{
    sn_external_declaration *ed;
    sn_translation_unit *right;

   public:
    static sn_translation_unit *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
    virtual void emit(Environment *env, EEmitGoal goal) const;
};
class sn_external_declaration : public SyntaxNode
{
    enum
    {
        FUNCDEF,
        DECL
    } branch;
    union {
        sn_function_definition *fd;
        sn_declaration *d;
    } data;

   public:
    static sn_external_declaration *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
};
class sn_function_definition : public SyntaxNode
{
    sn_declaration_specifiers *s;
    sn_declarator *d;
    sn_declaration_list *dl;
    sn_compound_statement *cs;

   public:
    static sn_function_definition *parse(Lexer &lex, Environment *env,
                                         sn_declaration_specifiers *s,
                                         sn_declarator *d);
    virtual std::string debugString();
};
class sn_declaration : public SyntaxNode
{
    sn_declaration_specifiers *s;
    sn_init_declarator_list *idl;

   public:
    static sn_declaration *parse(Lexer &lex, Environment *env);
    static sn_declaration *parse(Lexer &lex, Environment *env,
                                 sn_declaration_specifiers *s,
                                 sn_declarator *d);
    virtual std::string debugString();
};
class sn_declaration_list : public SyntaxNode
{
    sn_declaration *d;
    sn_declaration_list *right;

   public:
    static sn_declaration_list *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
};

// Declaration
class sn_init_declarator : public SyntaxNode
{
    sn_declarator *d;
    sn_initializer *i;

   public:
    sn_init_declarator()
    {
    }
    sn_init_declarator(sn_declarator *_d, sn_initializer *_i) : d(_d), i(_i)
    {
    }
    static sn_init_declarator *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
};
class sn_init_declarator_list : public SyntaxNode
{
    sn_init_declarator *id;
    sn_init_declarator_list *right;

   public:
    static sn_init_declarator_list *parse(Lexer &lex, Environment *env,
                                          sn_init_declarator *id = nullptr);
    virtual std::string debugString();
};
// declarator
class sn_declarator : public SyntaxNode
{
    sn_pointer *p;
    sn_direct_declarator *dd;

   public:
    static sn_declarator *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
};
class sn_direct_declarator : public SyntaxNode
{
    enum
    {
        ID,
        DECLARATOR,
        ARRAY,
        PARAM_LIST,
        ID_LIST // maybe empty, thus idlist == nullptr
    } branch;
    union {
        sn_identifier *id;
        sn_declarator *d;
        sn_const_expression *arr;
        sn_parameter_type_list *ptlist;
        sn_identifier_list *idlist;
    } data;
    sn_direct_declarator *right;

   public:
    static sn_direct_declarator *parse(Lexer &lex, Environment *env,
                                       bool head = true);
    virtual std::string debugString();
};
class sn_abstract_declarator : public SyntaxNode
{
    sn_pointer *p;
    sn_direct_abstract_declarator *dad;

   public:
    static sn_abstract_declarator *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
};
class sn_direct_abstract_declarator : public SyntaxNode
{
    enum
    {
        ABST_DECL,
        ARRAY,
        PARAM_LIST
    } branch;
    union {
        sn_abstract_declarator *ad;
        sn_const_expression *arr;
        sn_parameter_type_list *ptlist;
    } data;
    sn_direct_abstract_declarator *right;

   public:
    static sn_direct_abstract_declarator *parse(Lexer &lex, Environment *env,
                                                bool head = true);
    virtual std::string debugString();
};
// initializer
class sn_initializer : public SyntaxNode
{
    sn_expression *e;
    sn_initializer_list *il;

   public:
    static sn_initializer *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
};
class sn_initializer_list : public SyntaxNode
{
    sn_initializer *i;
    sn_initializer_list *right;

   public:
    static sn_initializer_list *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
};
// declarator-tail
class sn_parameter_type_list : public SyntaxNode
{
    sn_parameter_list *pl;
    bool varlist;

   public:
    static sn_parameter_type_list *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
};
class sn_parameter_list : public SyntaxNode
{
    sn_parameter_declaration *pd;
    sn_parameter_list *right;

   public:
    static sn_parameter_list *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
};
class sn_parameter_declaration : public SyntaxNode
{
    sn_declaration_specifiers *s;
    enum
    {
        DECL,
        ABST_DECL
    } branch;
    union {
        sn_declarator *d;
        sn_abstract_declarator *ad;
    } data;

   public:
    static sn_parameter_declaration *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
};

// specifier
class sn_declaration_specifiers : public SyntaxNode
{
    enum
    {
        STORAGE,
        TYPE_SPEC,
        TYPE_QUAL
    } branch;
    union {
        sn_storage_specifier *ss;
        sn_type_specifier *ts;
        sn_type_qualifier *tq;
    } data;
    sn_declaration_specifiers *right;

   public:
    static sn_declaration_specifiers *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
};
class sn_specifier_qualifier_list : public SyntaxNode
{
    enum
    {
        TYPE_SPEC,
        TYPE_QUAL
    } branch;
    union {
        sn_type_specifier *ts;
        sn_type_qualifier *tq;
    } data;
    sn_specifier_qualifier_list *right;

   public:
    static sn_specifier_qualifier_list *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
};
class sn_storage_specifier : public SyntaxNode
{
    TokenType t;

   public:
    static sn_storage_specifier *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
};
class sn_type_qualifier : public SyntaxNode
{
    TokenType t;

   public:
    static sn_type_qualifier *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
};
class sn_type_qualifier_list : public SyntaxNode
{
    sn_type_qualifier *tq;
    sn_type_qualifier_list *right;

   public:
    static sn_type_qualifier_list *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
};
class sn_type_specifier : public SyntaxNode
{
    enum
    {
        TYPESPEC_SIMPLE,
        TYPESPEC_STRUCT_UNION,
        TYPESPEC_ENUM,
        TYPESPEC_TYPEDEF
    } branch;
    union {
        TokenType t;
        sn_struct_union_specifier *sus;
        sn_enum_specifier *es;
        sn_typedef_name *tn;
    } data;

   public:
    static sn_type_specifier *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
};
class sn_struct_union_specifier : public SyntaxNode
{
    TokenType t;  // struct or union
    sn_identifier *tag;
    sn_struct_declaration_list *sdl;

   public:
    static sn_struct_union_specifier *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
};
class sn_enum_specifier : public SyntaxNode
{
    sn_identifier *tag;
    sn_enumerator_list *el;

   public:
    static sn_enum_specifier *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
};
// struct/union/enum definition
class sn_struct_declaration : public SyntaxNode
{
    sn_specifier_qualifier_list *sql;
    sn_struct_declarator_list *sdl;

   public:
    static sn_struct_declaration *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
};
class sn_struct_declaration_list : public SyntaxNode
{
    sn_struct_declaration *sd;
    sn_struct_declaration_list *right;

   public:
    static sn_struct_declaration_list *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
};
class sn_struct_declarator : public SyntaxNode
{
    sn_declarator *d;
    sn_const_expression *bit_field;

   public:
    static sn_struct_declarator *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
};
class sn_struct_declarator_list : public SyntaxNode
{
    sn_struct_declarator *sd;
    sn_struct_declarator_list *right;

   public:
    static sn_struct_declarator_list *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
};
class sn_enumerator_list : public SyntaxNode
{
    sn_enumerator *e;
    sn_enumerator_list *right;

   public:
    static sn_enumerator_list *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
};
class sn_enumerator : public SyntaxNode
{
    sn_enumeration_constant *ec;
    sn_const_expression *value;

   public:
    static sn_enumerator *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
};
class sn_enumeration_constant
{
    sn_identifier *id;

   public:
    static sn_enumeration_constant *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
};

// others
class sn_type_name : public SyntaxNode
{
    sn_specifier_qualifier_list *sql;
    sn_abstract_declarator *ad;

   public:
    static sn_type_name *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
};
class sn_pointer : public SyntaxNode
{
    sn_type_qualifier_list *tql;
    sn_pointer *right;

   public:
    static sn_pointer *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
};
class sn_identifier : public SyntaxNode
{
    Token id;

   public:
    static sn_identifier *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
};
class sn_identifier_list : public SyntaxNode
{
    sn_identifier *id;
    sn_identifier_list *right;

   public:
    static sn_identifier_list *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
};
class sn_typedef_name : public SyntaxNode
{
    sn_identifier *i;

   public:
    static sn_typedef_name *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
};

// Statement
class sn_statement : public SyntaxNode
{
   public:
    static sn_statement *parse(Lexer &lex, Environment *env);
};
class sn_statement_list : public SyntaxNode
{
    sn_statement *s;
    sn_statement_list *right;

   public:
    static sn_statement_list *parse(Lexer &lex, Environment *env);
};
class sn_label_statement : public sn_statement
{
    enum
    {
        LABEL_STAT,
        CASE_STAT,
        DEFAULT_STAT
    } branch;
    union {
        sn_identifier *id;
        sn_const_expression *value;
    } data;
    sn_statement *stat;

   public:
    static sn_statement *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
};
class sn_compound_statement : public sn_statement
{
    sn_declaration_list *dl;
    sn_statement_list *sl;

   public:
    static sn_compound_statement *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
    // virtual void emit(Environment *__not_used, EEmitGoal goal) const;
};
class sn_expression_statement : public sn_statement
{
    sn_expression *expr;

   public:
    static sn_statement *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
    // virtual void emit(Environment *env, EEmitGoal goal) const;
};
class sn_selection_statement : public sn_statement
{
    enum
    {
        IF_STAT,
        SWITCH_STAT
    } branch;
    sn_expression *expr;
    sn_statement *stmt;
    sn_statement *stmt2;

   public:
    static sn_statement *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
    // virtual void emit(Environment *env, EEmitGoal goal) const;
};
class sn_iteration_statement : public sn_statement
{
    typedef enum IterationType { WHILE_LOOP, DO_LOOP, FOR_LOOP } IterationType;

    IterationType type;
    sn_expression *expr;
    sn_expression *expr2;
    sn_expression *expr3;
    sn_statement *stmt;

   public:
    static sn_statement *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
    // virtual void emit(Environment *env, EEmitGoal goal) const;
};
class sn_jump_statement : public sn_statement
{
    typedef enum JumpType {
        JMP_GOTO,
        JMP_CONTINUE,
        JMP_BREAK,
        JMP_RETURN
    } JumpType;

    JumpType type;
    union {
        sn_identifier *id;  // for goto Label
        sn_expression *expr;
    } data;

   public:
    static sn_statement *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
    // virtual void emit(Environment *env, EEmitGoal goal) const;
};

// Expression
class sn_expression : public SyntaxNode
{
   protected:
    const Type *type_;

   public:
    sn_expression() : type_(nullptr)
    {
    }
    sn_expression(ESyntaxNodeType nt) : SyntaxNode(nt), type_(nullptr)
    {
    }
    const Type *type() const
    {
        return type_;
    }
};
class sn_comma_expression : public sn_expression
{
    sn_expression *curr, *next;

   public:
    static sn_expression *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
};
class sn_assign_expression : public sn_expression
{
    sn_expression *to, *from;  // from maybe nullptr
    TokenType op;

   public:
    static sn_expression *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
    // virtual void emit(Environment *env, EEmitGoal goal) const;
};
class sn_cond_expression : public sn_expression
{
    sn_expression *cond;
    sn_expression *left;
    sn_expression *right;

   public:
    static sn_expression *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
    // virtual void emit(Environment *env, EEmitGoal goal) const;
};
class sn_or_expression : public sn_expression
{
    sn_expression *left, *right;

   public:
    static sn_expression *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
    // virtual void emit(Environment *env, EEmitGoal goal) const;
};
class sn_and_expression : public sn_expression
{
    sn_expression *left, *right;

   public:
    static sn_expression *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
    // virtual void emit(Environment *env, EEmitGoal goal) const;
};
class sn_bitor_expression : public sn_expression
{
    sn_expression *left, *right;

   public:
    static sn_expression *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
    // virtual void emit(Environment *env, EEmitGoal goal) const;
};
class sn_bitxor_expression : public sn_expression
{
    sn_expression *left, *right;

   public:
    static sn_expression *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
    // virtual void emit(Environment *env, EEmitGoal goal) const;
};
class sn_bitand_expression : public sn_expression
{
    sn_expression *left, *right;

   public:
    static sn_expression *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
    // virtual void emit(Environment *env, EEmitGoal goal) const;
};
class sn_eq_expression : public sn_expression
{
    sn_expression *left, *right;
    TokenType op;

   public:
    static sn_expression *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
    // virtual void emit(Environment *env, EEmitGoal goal) const;
};
class sn_rel_expression : public sn_expression
{
    sn_expression *left, *right;
    TokenType op;

   public:
    static sn_expression *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
    // virtual void emit(Environment *env, EEmitGoal goal) const;
};
class sn_shift_expression : public sn_expression
{
    sn_expression *left, *right;
    TokenType op;

   public:
    static sn_expression *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
    // virtual void emit(Environment *env, EEmitGoal goal) const;
};
class sn_add_expression : public sn_expression
{
    sn_expression *left, *right;
    TokenType op;

   public:
    static sn_expression *parse(Lexer &lex, Environment *env,
                                sn_expression *left = nullptr);
    virtual std::string debugString();
    // virtual void emit(Environment *env, EEmitGoal goal) const;
};
class sn_mul_expression : public sn_expression
{
    sn_expression *left, *right;
    TokenType op;

   public:
    static sn_expression *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
};
class sn_cast_expression : public sn_expression
{
    sn_expression *from;
    sn_type_name *to;
    // bool implicit_;

   public:
    // sn_cast_expression(const Type *to, sn_expression *from, bool
    // is_implicit = true)
    //     : target(from), implicit_(is_implicit)
    // {
    //     type_ = to;
    // }
    static sn_expression *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
};
class sn_unary_expression : public sn_expression
{
    TokenType op;
    sn_expression *e;
    sn_type_name *tn;

   public:
    sn_unary_expression() : sn_expression(SN_UNARY_EXPRESSION)
    {
    }
    static sn_expression *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
    // virtual void emit(Environment *env, EEmitGoal goal) const;
};
class sn_postfix_expression : public sn_expression
{
    typedef enum PostfixOperation {
        POSTFIX_INDEX,
        POSTFIX_CALL,
        POSTFIX_OBJECT_OFFSET,
        POSTFIX_POINTER_OFFSET,
        POSTFIX_INC,
        POSTFIX_DEC
    } PostfixOperation;
    PostfixOperation op;
    sn_expression *left;
    union {
        sn_expression *e;  // array index
        sn_argument_expression_list *ael;
        sn_identifier *id;
    } data;

   public:
    static sn_expression *parse(Lexer &lex, Environment *env,
                                sn_expression *left);
    virtual std::string debugString();
    // virtual void emit(Environment *env, EEmitGoal goal) const;
};
class sn_primary_expression : public sn_expression
{
    friend sn_postfix_expression;

    // constant, string-literal
    Token t;
    union {
        sn_identifier *id;
        sn_expression *expr;
    } data;

   public:
    static sn_expression *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
    // virtual void emit(Environment *env, EEmitGoal goal) const;
};
class sn_argument_expression_list : public SyntaxNode
{
    sn_expression *ae;
    sn_argument_expression_list *right;

   public:
    static sn_argument_expression_list *parse(Lexer &lex, Environment *env);
};

class sn_const_expression : public sn_expression
{
    sn_expression *e;

   public:
    // Token eval();
    static sn_const_expression *parse(Lexer &lex, Environment *env);
    virtual std::string debugString();
};

class Parser
{
    // Environment env;
    Lexer &lex;
    sn_translation_unit *tu;

    void __debugPrint(string &&s);

   public:
    Parser(Lexer &l) : lex(l)
    {
    }
    void parse()
    {
        tu = sn_translation_unit::parse(lex, nullptr);
        __debugPrint(tu->debugString());
        // Environment::ParseGlobalDeclaration(lex, &env);
        // env.debugPrint();
        // SymbolFactory::check();
    }
    void emit()
    {
        // env.emit();
    }
    // Token eval();
};
