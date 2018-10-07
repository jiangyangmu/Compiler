#include "GrammerDefinition.h"

#define TOKEN(s) PRODUCTION(TokenFromString(s))

// clang-format off
Grammer CLanguageGrammer() {
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
          declaration_or_function_definition & *(declaration_or_function_definition)
        ;
    declaration_or_function_definition =
          declaration_specifiers &
          (
              TOKEN(";")
          |   (
                  declarator &
                  (
                      compound_stmt
                  |   ( *( TOKEN(",") & declarator ) & TOKEN(";") )
                  )
              )
          )
        ;
    declaration =
          declaration_specifiers &
          ~(
              declarator & *( TOKEN(",") & declarator )
          ) &
          TOKEN(";")
        ;
    declaration_specifiers =
          ( storage_class_specifier | type_specifier | type_qualifier ) & *( storage_class_specifier | type_specifier | type_qualifier )
        ;
    storage_class_specifier =
          TOKEN("typedef")
        | TOKEN("extern")
        | TOKEN("static")
        | TOKEN("auto")
        | TOKEN("register")
        ;
    type_specifier =
          TOKEN("void")
        | TOKEN("char")
        | TOKEN("int")
        | TOKEN("float")
        | TOKEN("signed")
        | struct_or_union_specifier
        ;
    type_qualifier =
          TOKEN("const")
        | TOKEN("volatile")
        ;
    struct_or_union_specifier =
          (
              TOKEN("struct")
          |   TOKEN("union")
          ) &
          ~( TOKEN("id") ) &
          TOKEN("{") &
          struct_declaration & *(struct_declaration) &
          TOKEN("}")
        ;
    struct_declaration =
          ( type_specifier | type_qualifier ) & *( type_specifier | type_qualifier ) &
          struct_declarator & *( TOKEN(",") & struct_declarator ) &
          TOKEN(";")
        ;
    struct_declarator =
          declarator
        ;

    declarator =
          declarator_recursive
        ;
    declarator_recursive =
          ( pointer & ~(direct_declarator) )
        | direct_declarator
        ;
    direct_declarator =
          ( TOKEN("id") & ~(direct_declarator_tail) )
        | (
              TOKEN("(") & declarator_recursive  & TOKEN(")") &
              ~(
                  direct_declarator_tail
              )
          )
        | direct_declarator_tail
        ;
    direct_declarator_tail =
          (
              ( TOKEN("[") & TOKEN("1") & TOKEN("]") )
          |   ( TOKEN("(") & ~(parameter_list) & TOKEN(")") )
          ) &
          *(
              ( TOKEN("[") & TOKEN("1") & TOKEN("]") )
          |   ( TOKEN("(") & ~(parameter_list) & TOKEN(")") )
          )
        ;

    parameter_list =
          parameter_declaration &
          *(
              TOKEN(",") &
              (
                  parameter_declaration
              |   TOKEN("...")
              )
          )
        ;
    parameter_declaration =
          declaration_specifiers & ~( declarator )
        ;
    pointer =
          TOKEN("*") & *(type_qualifier) & *( TOKEN("*") & *(type_qualifier) )
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
          ~(expression) & TOKEN(";")
        ;
    labeled_stmt =
          ( TOKEN("id") & TOKEN(":") & statement & TOKEN(";") ) /* TODO: remove this after FOLLOW computation is fixed. */
        | ( TOKEN("case") & TOKEN("1") & TOKEN(":") & statement & TOKEN(";") ) /* TODO: remove this after FOLLOW computation is fixed. */
        | ( TOKEN("default") & TOKEN(":") & statement & TOKEN(";") ) /* TODO: remove this after FOLLOW computation is fixed. */
        ;
    selection_stmt =
          TOKEN("if") & TOKEN("(") & expression & TOKEN(")") &
          statement & TOKEN(";") /* TODO: remove this after FOLLOW computation is fixed. */
        ;

    expression =
          comma_expr /* TODO: change to comma_expr */
        ;
    comma_expr =
          add_expr & *( TOKEN(",") & add_expr )
        ;
    add_expr =
          primary_expr & *( TOKEN("+") & primary_expr )
        ;
    primary_expr =
          TOKEN("id")
        | TOKEN("1")
        | ( TOKEN("(") & expression & TOKEN(")") )
        ;

    GM_END(G);
    return G;
}
// clang-format on
