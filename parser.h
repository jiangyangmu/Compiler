#pragma once

// #include "codegen.h"
// #include "env.h"
#include "common.h"
#include "env.h"
#include "lexer.h"
#include "symbol.h"
#include "type.h"

#include <iostream>

// TODO: support sn_const_expression

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

class SyntaxNode : public TreeLike<SyntaxNode>, public Stringable
{
    ESyntaxNodeType node_type_;

   public:
    // Identification
    // TODO: remove default value after implement all nodes,
    // default value only let it compiles when only implement
    // part of nodes.
    SyntaxNode(ESyntaxNodeType node_type = SN_NONE) : node_type_(node_type) {}
    ESyntaxNodeType nodeType() const
    {
        return node_type_;
    }

    // Visitor
    virtual void visit(Environment *&env, const int pass);
    virtual void beforeChildren(Environment *&env, const int pass);
    virtual void afterChildren(Environment *&env, const int pass);
    // Visitor Helper, only used by declarations
    ESymbolScope getScope() const;
    // Stringable
    virtual std::string toString() const;
};

class sn_declaration_specifiers;
class sn_declarator;
class sn_initializer;

class sn_translation_unit : public SyntaxNode
{
   public:
    sn_translation_unit() : SyntaxNode(SN_TRANSLATION_UNIT) {}
    static sn_translation_unit *parse(Lexer &lex);

    // Visitor
    // virtual void afterChildren(Environment *&env, const int pass);
};
class sn_external_declaration : public SyntaxNode
{
   public:
    sn_external_declaration() : SyntaxNode(SN_EXTERNAL_DECLARATION) {}
    static sn_external_declaration *parse(Lexer &lex);

    // Visitor
    // virtual void afterChildren(Environment *&env, const int pass);
};
class sn_function_definition : public SyntaxNode
{
    Environment *body_env_;

    // Annotation
   public:
    Type *type_info_;
    StringRef name_info_;

   public:
    sn_function_definition() : SyntaxNode(SN_FUNCTION_DEFINITION) {}
    static sn_function_definition *parse(Lexer &lex,
                                         sn_declaration_specifiers *s,
                                         sn_declarator *d);

    // Visitor
    virtual void visit(Environment *&env, const int pass);
    virtual void afterChildren(Environment *&env, const int pass);
    // special impl
    void beforeParamList(Environment *&env, const int pass);
    void afterParamList(Environment *&env, const int pass);
};
class sn_declaration : public SyntaxNode
{
   public:
    sn_declaration() : SyntaxNode(SN_DECLARATION) {}
    static sn_declaration *parse(Lexer &lex);
    static sn_declaration *parse(Lexer &lex, sn_declaration_specifiers *s,
                                 sn_declarator *d);

    // Visitor
    virtual void afterChildren(Environment *&env, const int pass);
};
class sn_declaration_list : public SyntaxNode
{
   public:
    sn_declaration_list() : SyntaxNode(SN_DECLARATION_LIST) {}
    static sn_declaration_list *parse(Lexer &lex);

    // Visitor
    // virtual void afterChildren(Environment *&env, const int pass);
};

// Declaration
class sn_init_declarator : public SyntaxNode
{
   public:
    Type *type_info_;
    StringRef name_info_;
    // vector<IROperation> code_info_;

   public:
    sn_init_declarator() : SyntaxNode(SN_INIT_DECLARATOR) {}
    static sn_init_declarator *parse(Lexer &lex);

    // Visitor
    virtual void afterChildren(Environment *&env, const int pass);
};
class sn_init_declarator_list : public SyntaxNode
{
   public:
    sn_init_declarator_list() : SyntaxNode(SN_INIT_DECLARATOR_LIST) {}
    static sn_init_declarator_list *parse(Lexer &lex,
                                          sn_init_declarator *id = nullptr);

    // Visitor
    // virtual void afterChildren(Environment *&env, const int pass);
};
// declarator
class sn_declarator : public SyntaxNode
{
   public:
    // Annotation
    Type *type_info_;
    StringRef name_info_;

   public:
    sn_declarator() : SyntaxNode(SN_DECLARATOR) {}
    static sn_declarator *parse(Lexer &lex);

    // Visitor
    virtual void afterChildren(Environment *&env, const int pass);
};
class sn_direct_declarator : public SyntaxNode
{
   public:
    // Annotation
    Type *type_info_;
    StringRef name_info_;

   public:
    sn_direct_declarator() : SyntaxNode(SN_DIRECT_DECLARATOR) {}
    static sn_direct_declarator *parse(Lexer &lex);

    // Visitor
    virtual void afterChildren(Environment *&env, const int pass);
};
class sn_abstract_declarator : public SyntaxNode
{
   public:
    Type *type_info_;

   public:
    sn_abstract_declarator() : SyntaxNode(SN_ABSTRACT_DECLARATOR) {}
    static sn_abstract_declarator *parse(Lexer &lex);

    // Visitor
    virtual void afterChildren(Environment *&env, const int pass);
};
class sn_direct_abstract_declarator : public SyntaxNode
{
   public:
    Type *type_info_;

   public:
    sn_direct_abstract_declarator() : SyntaxNode(SN_DIRECT_ABSTRACT_DECLARATOR)
    {
    }
    static sn_direct_abstract_declarator *parse(Lexer &lex);

    // Visitor
    virtual void afterChildren(Environment *&env, const int pass);
};
// initializer
class sn_initializer : public SyntaxNode
{
   public:
    sn_initializer() : SyntaxNode(SN_INITIALIZER) {}
    static sn_initializer *parse(Lexer &lex);

    // Visitor
    // virtual void afterChildren(Environment *&env, const int pass);
};
class sn_initializer_list : public SyntaxNode
{
   public:
    sn_initializer_list() : SyntaxNode(SN_INITIALIZER_LIST) {}
    static sn_initializer_list *parse(Lexer &lex);

    // Visitor
    // virtual void afterChildren(Environment *&env, const int pass);
};
// declarator-tail
class sn_parameter_type_list : public SyntaxNode
{
    bool varlist;

   public:
    Type *type_info_;

   public:
    sn_parameter_type_list() : SyntaxNode(SN_PARAMETER_TYPE_LIST) {}
    static sn_parameter_type_list *parse(Lexer &lex);

    // Visitor
    virtual void afterChildren(Environment *&env, const int pass);
};
class sn_parameter_list : public SyntaxNode
{
   public:
    Type *type_info_;

   public:
    sn_parameter_list() : SyntaxNode(SN_PARAMETER_LIST) {}
    static sn_parameter_list *parse(Lexer &lex);

    // Visitor
    virtual void afterChildren(Environment *&env, const int pass);
};
class sn_parameter_declaration : public SyntaxNode
{
   public:
    Type *type_info_;
    StringRef name_info_;

   public:
    sn_parameter_declaration() : SyntaxNode(SN_PARAMETER_DECLARATION) {}
    static sn_parameter_declaration *parse(Lexer &lex);

    // Visitor
    virtual void afterChildren(Environment *&env, const int pass);
};
// specifier
class sn_declaration_specifiers : public SyntaxNode
{
    // Annotation
   public:
    Type *type_info_;
    TokenType storage_info_;

   public:
    sn_declaration_specifiers() : SyntaxNode(SN_DECLARATION_SPECIFIERS) {}
    static sn_declaration_specifiers *parse(Lexer &lex);

    // Visitor
    virtual void afterChildren(Environment *&env, const int pass);
};
class sn_specifier_qualifier_list : public SyntaxNode
{
    // Annotation
   public:
    Type *type_info_;

   public:
    sn_specifier_qualifier_list() : SyntaxNode(SN_SPECIFIER_QUALIFIER_LIST) {}
    static sn_specifier_qualifier_list *parse(Lexer &lex);

    // Visitor
    virtual void afterChildren(Environment *&env, const int pass);
};
class sn_storage_specifier : public SyntaxNode
{
   public:
    TokenType t;

   public:
    sn_storage_specifier() : SyntaxNode(SN_STORAGE_SPECIFIER) {}
    static sn_storage_specifier *parse(Lexer &lex);

    // Visitor
    // virtual void afterChildren(Environment *&env, const int pass);
};
class sn_type_qualifier : public SyntaxNode
{
   public:
    TokenType t;

   public:
    sn_type_qualifier() : SyntaxNode(SN_TYPE_QUALIFIER) {}
    static sn_type_qualifier *parse(Lexer &lex);

    // Visitor
    // virtual void afterChildren(Environment *&env, const int pass);
};
class sn_type_qualifier_list : public SyntaxNode
{
   public:
    sn_type_qualifier_list() : SyntaxNode(SN_TYPE_QUALIFIER_LIST) {}
    static sn_type_qualifier_list *parse(Lexer &lex);

    // Visitor
    // virtual void afterChildren(Environment *&env, const int pass);
};
class sn_type_specifier : public SyntaxNode
{
   public:
    TokenType t;

    // Annotation
   public:
    Type *type_info_;

   public:
    sn_type_specifier() : SyntaxNode(SN_TYPE_SPECIFIER) {}
    static sn_type_specifier *parse(Lexer &lex);

    // Visitor
    virtual void afterChildren(Environment *&env, const int pass);
};
class sn_struct_union_specifier : public SyntaxNode
{
    TokenType t;  // struct or union
                  // sn_identifier *tag; [optional]
                  // sn_struct_declaration_list *sdl; [optional]

    // Annotation
   public:
    Type *type_info_;

   public:
    sn_struct_union_specifier() : SyntaxNode(SN_STRUCT_UNION_SPECIFIER) {}
    static sn_struct_union_specifier *parse(Lexer &lex);

    // Visitor
    virtual void beforeChildren(Environment *&env, const int pass);
    virtual void afterChildren(Environment *&env, const int pass);
};
class sn_enum_specifier : public SyntaxNode
{
    // Annotation
   public:
    Type *type_info_;

   public:
    sn_enum_specifier() : SyntaxNode(SN_ENUM_SPECIFIER) {}
    static sn_enum_specifier *parse(Lexer &lex);

    // Visitor
    virtual void beforeChildren(Environment *&env, const int pass);
    virtual void afterChildren(Environment *&env, const int pass);
};
// struct/union/enum definition
class sn_struct_declaration : public SyntaxNode
{
   public:
    vector<Symbol *> symbols_info_;

   public:
    sn_struct_declaration() : SyntaxNode(SN_STRUCT_DECLARATION) {}
    static sn_struct_declaration *parse(Lexer &lex);

    // Visitor
    virtual void afterChildren(Environment *&env, const int pass);
};
class sn_struct_declaration_list : public SyntaxNode
{
   public:
    sn_struct_declaration_list() : SyntaxNode(SN_STRUCT_DECLARATION_LIST) {}
    static sn_struct_declaration_list *parse(Lexer &lex);

    // Visitor
    // virtual void afterChildren(Environment *&env, const int pass);
};
class sn_struct_declarator : public SyntaxNode
{
   public:
    Type *type_info_;
    StringRef name_info_;

   public:
    sn_struct_declarator() : SyntaxNode(SN_STRUCT_DECLARATOR) {}
    static sn_struct_declarator *parse(Lexer &lex);

    // Visitor
    virtual void afterChildren(Environment *&env, const int pass);
};
class sn_struct_declarator_list : public SyntaxNode
{
   public:
    sn_struct_declarator_list() : SyntaxNode(SN_STRUCT_DECLARATOR_LIST) {}
    static sn_struct_declarator_list *parse(Lexer &lex);

    // Visitor
    // virtual void afterChildren(Environment *&env, const int pass);
};
class sn_enumerator_list : public SyntaxNode
{
   public:
    sn_enumerator_list() : SyntaxNode(SN_ENUMERATOR_LIST) {}
    static sn_enumerator_list *parse(Lexer &lex);

    // Visitor
    // virtual void afterChildren(Environment *&env, const int pass);
};
class sn_enumerator : public SyntaxNode
{
   public:
    sn_enumerator() : SyntaxNode(SN_ENUMERATOR) {}
    static sn_enumerator *parse(Lexer &lex);

    // Visitor
    // virtual void afterChildren(Environment *&env, const int pass);
};
class sn_enumeration_constant : public SyntaxNode
{
   public:
    sn_enumeration_constant() : SyntaxNode(SN_ENUMERATION_CONSTANT) {}
    static sn_enumeration_constant *parse(Lexer &lex);

    // Visitor
    // virtual void afterChildren(Environment *&env, const int pass);
};
// others
class sn_type_name : public SyntaxNode
{
   public:
    Type *type_info_;

   public:
    sn_type_name() : SyntaxNode(SN_TYPE_NAME) {}
    static sn_type_name *parse(Lexer &lex);

    // Visitor
    virtual void afterChildren(Environment *&env, const int pass);
};
class sn_pointer : public SyntaxNode
{
   public:
    Type *type_info_;

   public:
    sn_pointer() : SyntaxNode(SN_POINTER) {}
    static sn_pointer *parse(Lexer &lex);

    // Visitor
    virtual void afterChildren(Environment *&env, const int pass);
};
class sn_identifier : public SyntaxNode
{
    Token id;

    // Annotation
   public:
    StringRef name_info_;

   public:
    sn_identifier() : SyntaxNode(SN_IDENTIFIER) {}
    static sn_identifier *parse(Lexer &lex);

    // Visitor
    virtual void afterChildren(Environment *&env, const int pass);
};
class sn_identifier_list : public SyntaxNode
{
    // Annotation
   public:
    Type *type_info_;

   public:
    sn_identifier_list() : SyntaxNode(SN_IDENTIFIER_LIST) {}
    static sn_identifier_list *parse(Lexer &lex);

    // Visitor
    virtual void afterChildren(Environment *&env, const int pass);
};
class sn_typedef_name : public SyntaxNode
{
    // Annotation
   public:
    Type *type_info_;

   public:
    sn_typedef_name() : SyntaxNode(SN_TYPEDEF_NAME) {}
    static sn_typedef_name *parse(Lexer &lex);

    // Visitor
    virtual void afterChildren(Environment *&env, const int pass);
};

// Statement
class sn_statement : public SyntaxNode
{
   protected:
    sn_statement(ESyntaxNodeType nt = SN_STATEMENT) : SyntaxNode(nt) {}

   public:
    vector<IROperation> code_info_;

   public:
    static sn_statement *parse(Lexer &lex);
};
class sn_statement_list : public SyntaxNode
{
    // vector<sn_statement *> s;

   public:
    sn_statement_list() : SyntaxNode(SN_STATEMENT_LIST) {}
    static sn_statement_list *parse(Lexer &lex);
};
class sn_label_statement : public sn_statement
{
    // union {
    //     sn_identifier *id;
    //     sn_const_expression *value;
    // };
    // sn_statement *stat;

   public:
    sn_label_statement() : sn_statement(SN_LABEL_STATEMENT) {}
    static sn_statement *parse(Lexer &lex);

    // Visitor
    virtual void afterChildren(Environment *&env, const int pass);
};
class sn_compound_statement : public sn_statement
{
    // sn_declaration_list *dl;
    // sn_statement_list *sl;

   public:
    sn_compound_statement() : sn_statement(SN_COMPOUND_STATEMENT) {}
    static sn_compound_statement *parse(Lexer &lex);

    // Visitor
    virtual void visit(Environment *&env, const int pass);
    virtual void afterDeclarations(Environment *&env, const int pass);
    virtual void afterChildren(Environment *&env, const int pass);
};
class sn_expression_statement : public sn_statement
{
    // sn_expression *expr;

   public:
    sn_expression_statement() : sn_statement(SN_EXPRESSION_STATEMENT) {}
    static sn_statement *parse(Lexer &lex);

    // Visitor
    virtual void afterChildren(Environment *&env, const int pass);
};
class sn_selection_statement : public sn_statement
{
    TokenType t;  // if or switch
                  // sn_expression *expr;
                  // sn_statement *stmt;
                  // sn_statement *stmt2;

   public:
    sn_selection_statement() : sn_statement(SN_SELECTION_STATEMENT) {}
    static sn_statement *parse(Lexer &lex);

    // Visitor
    virtual void afterChildren(Environment *&env, const int pass);
};
class sn_iteration_statement : public sn_statement
{
    TokenType t;  // while, do, for
                  // sn_expression *expr;
                  // sn_expression *expr2;
                  // sn_expression *expr3;
                  // sn_statement *stmt;

   public:
    sn_iteration_statement() : sn_statement(SN_ITERATION_STATEMENT) {}
    static sn_statement *parse(Lexer &lex);

    // Visitor
    virtual void afterChildren(Environment *&env, const int pass);
};
class sn_jump_statement : public sn_statement
{
    TokenType t;  // goto, continue, break, return
                  // union {
                  //     sn_identifier *id;  // for goto Label
                  //     sn_expression *expr;
                  // };

   public:
    sn_jump_statement() : sn_statement(SN_JUMP_STATEMENT) {}
    static sn_statement *parse(Lexer &lex);

    // Visitor
    virtual void afterChildren(Environment *&env, const int pass);
};

// Expression
class sn_expression : public SyntaxNode
{
   public:
    Type *type_;

   public:
    vector<IROperation> code_info_;
    IRAddress result_info_;

   public:
    explicit sn_expression(ESyntaxNodeType nt = SN_EXPRESSION)
        : SyntaxNode(nt), type_(nullptr)
    {
    }

    // Visitor
    virtual void afterChildren(Environment *&env, const int pass);
};
class sn_comma_expression : public sn_expression
{
    // vector<sn_expression *> curr;

   public:
    sn_comma_expression() : sn_expression(SN_COMMA_EXPRESSION) {}
    static sn_expression *parse(Lexer &lex);

    // Visitor
    virtual void afterChildren(Environment *&env, const int pass);
};
class sn_assign_expression : public sn_expression
{
    // sn_expression *to, *from;  // from maybe nullptr
    TokenType op;

   public:
    sn_assign_expression() : sn_expression(SN_ASSIGN_EXPRESSION) {}
    static sn_expression *parse(Lexer &lex);

    // Visitor
    virtual void afterChildren(Environment *&env, const int pass);
};
class sn_cond_expression : public sn_expression
{
    // sn_expression *cond;
    // sn_expression *left;
    // sn_expression *right;

   public:
    sn_cond_expression() : sn_expression(SN_COND_EXPRESSION) {}
    static sn_expression *parse(Lexer &lex);

    // Visitor
    virtual void afterChildren(Environment *&env, const int pass);
};
class sn_or_expression : public sn_expression
{
    // sn_expression *left, *right;

   public:
    sn_or_expression() : sn_expression(SN_OR_EXPRESSION) {}
    static sn_expression *parse(Lexer &lex);

    // Visitor
    virtual void afterChildren(Environment *&env, const int pass);
};
class sn_and_expression : public sn_expression
{
    // sn_expression *left, *right;

   public:
    sn_and_expression() : sn_expression(SN_AND_EXPRESSION) {}
    static sn_expression *parse(Lexer &lex);

    // Visitor
    virtual void afterChildren(Environment *&env, const int pass);
};
class sn_bitor_expression : public sn_expression
{
    // sn_expression *left, *right;

   public:
    sn_bitor_expression() : sn_expression(SN_BITOR_EXPRESSION) {}
    static sn_expression *parse(Lexer &lex);

    // Visitor
    virtual void afterChildren(Environment *&env, const int pass);
};
class sn_bitxor_expression : public sn_expression
{
    // sn_expression *left, *right;

   public:
    sn_bitxor_expression() : sn_expression(SN_BITXOR_EXPRESSION) {}
    static sn_expression *parse(Lexer &lex);

    // Visitor
    virtual void afterChildren(Environment *&env, const int pass);
};
class sn_bitand_expression : public sn_expression
{
    // sn_expression *left, *right;

   public:
    sn_bitand_expression() : sn_expression(SN_BITAND_EXPRESSION) {}
    static sn_expression *parse(Lexer &lex);

    // Visitor
    virtual void afterChildren(Environment *&env, const int pass);
};
class sn_eq_expression : public sn_expression
{
    // sn_expression *left, *right;
    TokenType op;

   public:
    sn_eq_expression() : sn_expression(SN_EQ_EXPRESSION) {}
    static sn_expression *parse(Lexer &lex);

    // Visitor
    virtual void afterChildren(Environment *&env, const int pass);
};
class sn_rel_expression : public sn_expression
{
    // sn_expression *left, *right;
    TokenType op;

   public:
    sn_rel_expression() : sn_expression(SN_REL_EXPRESSION) {}
    static sn_expression *parse(Lexer &lex);

    // Visitor
    virtual void afterChildren(Environment *&env, const int pass);
};
class sn_shift_expression : public sn_expression
{
    // sn_expression *left, *right;
    TokenType op;

   public:
    sn_shift_expression() : sn_expression(SN_SHIFT_EXPRESSION) {}
    static sn_expression *parse(Lexer &lex);

    // Visitor
    virtual void afterChildren(Environment *&env, const int pass);
};
class sn_add_expression : public sn_expression
{
    // sn_expression *left, *right;
    TokenType op;

   public:
    sn_add_expression() : sn_expression(SN_ADD_EXPRESSION) {}
    static sn_expression *parse(Lexer &lex);

    // Visitor
    virtual void afterChildren(Environment *&env, const int pass);
};
class sn_mul_expression : public sn_expression
{
    // sn_expression *left, *right;
    TokenType op;

   public:
    sn_mul_expression() : sn_expression(SN_MUL_EXPRESSION) {}
    static sn_expression *parse(Lexer &lex);

    // Visitor
    virtual void afterChildren(Environment *&env, const int pass);
};
class sn_cast_expression : public sn_expression
{
    // sn_type_name *to;
    // sn_expression *from;
    // bool implicit_;

   public:
    sn_cast_expression() : sn_expression(SN_CAST_EXPRESSION) {}
    // sn_cast_expression(const Type *to, sn_expression *from, bool
    // is_implicit = true)
    //     : target(from), implicit_(is_implicit)
    // {
    //     type_ = to;
    // }
    static sn_expression *parse(Lexer &lex);

    // Visitor
    virtual void afterChildren(Environment *&env, const int pass);
};
class sn_unary_expression : public sn_expression
{
    TokenType op;
    // union {
    //     sn_expression *e;
    //     sn_type_name *tn;
    // };

   public:
    sn_unary_expression() : sn_expression(SN_UNARY_EXPRESSION) {}
    static sn_expression *parse(Lexer &lex);

    // Visitor
    virtual void afterChildren(Environment *&env, const int pass);
};
class sn_postfix_expression : public sn_expression
{
    TokenType op;
    // sn_expression *left;
    // union {
    //     sn_expression *e;  // array index
    //     sn_argument_expression_list *ael;
    //     sn_identifier *id;
    // };

   public:
    sn_postfix_expression() : sn_expression(SN_POSTFIX_EXPRESSION) {}
    static sn_expression *parse(Lexer &lex, sn_expression *left);

    // Visitor
    virtual void afterChildren(Environment *&env, const int pass);
};
class sn_const_expression;
class sn_primary_expression : public sn_expression
{
    friend sn_postfix_expression;
    friend sn_const_expression;

    Token t;
    // union {
    //     Token t; // constant, string-literal
    //     sn_identifier *id;
    //     sn_expression *expr;
    // };

   public:
    sn_primary_expression() : sn_expression(SN_PRIMARY_EXPRESSION) {}
    static sn_expression *parse(Lexer &lex);

    // Visitor
    virtual void afterChildren(Environment *&env, const int pass);
};
class sn_argument_expression_list : public SyntaxNode
{
    // vector<sn_expression *> ae;

   public:
    sn_argument_expression_list() : SyntaxNode(SN_ARGUMENT_EXPRESSION_LIST) {}
    static sn_argument_expression_list *parse(Lexer &lex);
};

class sn_const_expression : public sn_expression
{
    // sn_expression *e;
    int _value;

   public:
    sn_const_expression() : sn_expression(SN_CONST_EXPRESSION) {}
    int value() const
    {
        return _value;
    }
    static sn_const_expression *parse(Lexer &lex);

    // Visitor
    virtual void afterChildren(Environment *&env, const int pass);
};

void __debugPrint(string &&s);

// Pass 0: parse()
// Pass 1: type derivation & code generation
class Parser
{
    // Environment env;
    Lexer &lex;
    sn_translation_unit *tu;
    Environment *env;

   public:
    Parser(Lexer &l) : lex(l) {}
    void parse()
    {
        tu = sn_translation_unit::parse(lex);
        env = new Environment();
        tu->visit(env, 0);
        tu->visit(env, 1);

        // SymbolFactory::check();
    }
    void debugPrint(bool verbose)
    {
        if (verbose)
            __debugPrint(tu->toString());
        env->debugPrint();
    }
    void emit()
    {
        // env.emit();
    }
};
