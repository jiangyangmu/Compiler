#pragma once

#include <vector>

#include "../Base/String.h"

struct Token
{
    enum Type
    {
        UNKNOWN,

        // Identifier

        ID,
        // reserved word
        // sizeof: reserved word + operator
        KW_SIZEOF,
        KW_TYPEDEF,
        KW_AUTO, KW_REGISTER, KW_VOLATILE, KW_EXTERN,
        KW_CONST, KW_STATIC,
        KW_UNSIGNED, KW_SIGNED,
        KW_VOID,
        KW_INT, KW_LONG, KW_SHORT,
        KW_CHAR,
        KW_DOUBLE, KW_FLOAT,
        KW_ENUM,
        KW_STRUCT, KW_UNION,
        KW_IF, KW_ELSE,
        KW_DO, KW_WHILE, KW_FOR,
        KW_SWITCH, KW_CASE, KW_DEFAULT,
        KW_BREAK, KW_CONTINUE,
        KW_RETURN, KW_GOTO,

        // Constant

        // enum constant is identifier
        CONST_INT,
        CONST_CHAR,
        CONST_FLOAT,
        STRING,

        // Operator/Punctuator

        // address operator
        OP_DOT, OP_POINTTO,
        // condition operator
        OP_QMARK, OP_COLON,
        // comma operator
        OP_COMMA,
        // assignment operator
        ASSIGN, ADD_ASSIGN, SUB_ASSIGN, MUL_ASSIGN, DIV_ASSIGN, MOD_ASSIGN,
        SHL_ASSIGN, SHR_ASSIGN, AND_ASSIGN, OR_ASSIGN, XOR_ASSIGN,
        OP_INC, OP_DEC,
        // arithmetic operator
        // *: dereference + multiplier
        OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD,
        // bool operator
        BOOL_AND, BOOL_OR, BOOL_NOT,
        // relation operator
        REL_EQ, REL_NE, REL_GT, REL_GE, REL_LT, REL_LE,
        // bit operator
        BIT_AND, BIT_OR, BIT_XOR, BIT_NOT, BIT_SHL, BIT_SHR,

        // punctuator: ( ) [ ] { } ; ... 
        LP, RP, LSB, RSB, BLK_BEGIN, BLK_END, STMT_END, VAR_PARAM,

        // PP Directives
        PPD_IF, PPD_IFDEF, PPD_IFNDEF, PPD_ELIF, PPD_ELSE, PPD_ENDIF,
        PPD_DEFINE, PPD_UNDEF,
        PPD_INCLUDE,
        PPD_LINE,
        PPD_ERROR, PPD_PRAGMA, PPD_NULL,
        // PP Included File Path
        PP_LOCAL_PATH, PP_ENV_PATH,
        // PP L-Paren
        PP_LPAREN,

        // Special

        LINE_START, FILE_END,
    };
    
    Type type;

    StringRef text;
    union {
        double fval;
        int ival;
        int cval;
    };
};

std::vector<Token> Tokenize(std::vector<std::string> vLines);

std::vector<Token> Tokenize(std::string & text);
