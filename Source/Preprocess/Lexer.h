#pragma once

#include <string>

#include "../Base/String.h"
#include "RegexMatcher.h"

struct Token
{
    enum Type
    {
        UNKNOWN,

        // Identifier

        ID,
        KW_SIZEOF,
        KW_TYPEDEF,
        KW_AUTO,
        KW_REGISTER,
        KW_VOLATILE,
        KW_EXTERN,
        KW_CONST,
        KW_STATIC,
        KW_UNSIGNED,
        KW_SIGNED,
        KW_VOID,
        KW_INT,
        KW_LONG,
        KW_SHORT,
        KW_CHAR,
        KW_DOUBLE,
        KW_FLOAT,
        KW_ENUM,
        KW_STRUCT,
        KW_UNION,
        KW_IF,
        KW_ELSE,
        KW_DO,
        KW_WHILE,
        KW_FOR,
        KW_SWITCH,
        KW_CASE,
        KW_DEFAULT,
        KW_BREAK,
        KW_CONTINUE,
        KW_RETURN,
        KW_GOTO,

        // Constant

        CONST_INT,
        CONST_CHAR,
        CONST_FLOAT,
        STRING,

        // Operator/Punctuator

        ASSIGN,
        ADD_ASSIGN,
        SUB_ASSIGN,
        MUL_ASSIGN,
        DIV_ASSIGN,
        MOD_ASSIGN,
        AND_ASSIGN,
        OR_ASSIGN,
        SHL_ASSIGN,
        SHR_ASSIGN,
        XOR_ASSIGN,

        BOOL_AND,
        BOOL_NOT,
        BOOL_OR,

        BIT_AND,
        BIT_NOT,
        BIT_OR,
        BIT_SHL,
        BIT_SHR,
        BIT_XOR,

        BLK_BEGIN,
        BLK_END,
        LPAREN,
        RPAREN,
        LSB,
        RSB,

        OP_ADD,
        OP_COLON,
        OP_COMMA,
        OP_DEC,
        OP_DIV,
        OP_DOT,
        OP_INC,
        OP_MOD,
        OP_MUL,
        OP_SUB,
        OP_POINTTO,
        OP_QMARK,
        OP_STRINGIZING,
        OP_TOKEN_PASTING,

        REL_EQ,
        REL_GE,
        REL_GT,
        REL_LE,
        REL_LT,
        REL_NE,

        STMT_END,
        VAR_PARAM,

        // PP Directives

        PPD_IF,
        PPD_IFDEF,
        PPD_IFNDEF,
        PPD_ELIF,
        PPD_ELSE,
        PPD_ENDIF,
        PPD_DEFINE,
        PPD_UNDEF,
        PPD_INCLUDE,
        PPD_LINE,
        PPD_ERROR,
        PPD_PRAGMA,
        PPD_NULL,

        // PP Included File Path

        PP_LOCAL_PATH,
        PP_ENV_PATH,

        // PP L-Paren

        PP_LPAREN,

        // Special

        SPACE,
        NEW_LINE,
        END,
    };

    Type type;
    std::string text;
    struct
    {
        int ival;
        float fval;
        char cval;
    };

    Token() : type(UNKNOWN), text() {}
    Token(Type type_, std::string text_) : type(type_), text(text_) {}
    Token(const Token & t) : type(t.type), text(t.text), ival(t.ival), fval(t.fval), cval(t.cval) {}
    Token & operator = (const Token & t)
    {
        this->~Token();
        new (this) Token(t);
        return *this;
    }
    Token(Token && t) : type(t.type), text(t.text) {}
    Token & operator = (Token && t)
    {
        this->~Token();
        new (this) Token(t);
        return *this;
    }
    ~Token() {}
};
inline bool IsRelOp(Token::Type t)
{
    return Token::REL_EQ < t && t < Token::REL_NE;
}
inline bool IsEqOp(Token::Type t)
{
    return t == Token::REL_EQ || t == Token::REL_NE;
}
inline bool IsAssignOp(Token::Type t)
{
    return Token::ASSIGN <= t && t <= Token::XOR_ASSIGN;
}
inline bool IsShiftOp(Token::Type t)
{
    return t == Token::BIT_SHL || t == Token::BIT_SHR;
}
inline bool IsAddOp(Token::Type t)
{
    return t == Token::OP_ADD || t == Token::OP_SUB;
}
inline bool IsMulOp(Token::Type t)
{
    return t == Token::OP_MUL || t == Token::OP_DIV || t == Token::OP_MOD;
}
inline bool IsUnaryOp(Token::Type t)
{
    return t == Token::BIT_AND ||
        t == Token::OP_MUL ||
        t == Token::OP_ADD ||
        t == Token::OP_SUB ||
        t == Token::BIT_NOT ||
        t == Token::BOOL_NOT;
}

std::vector<Token> LexProcess(std::string fileName, std::string * sourceAfterPreproc = nullptr);

class TokenIterator {
public:
    TokenIterator(std::vector<Token> & tokens);
    bool has() const;
    Token next();
    Token peek() const;
    Token peekN(int n) const;

private:
    std::vector<Token> & tokens_;
    size_t i_;
};
