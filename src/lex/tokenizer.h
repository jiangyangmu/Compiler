#pragma once

#include <vector>

#include "../common.h"

class Token {
public:
    enum Type {
        // clang-format off

        UNKNOWN,

        // identifier
        ID,

        // reserved word
        // sizeof: reserved word + operator
        KW_TYPEDEF, KW_SIZEOF,
        KW_AUTO, KW_REGISTER, KW_VOLATILE, KW_EXTERN,
        KW_CONST, KW_STATIC, KW_UNSIGNED, KW_SIGNED,
        KW_VOID, KW_INT, KW_LONG, KW_SHORT, KW_CHAR,
        KW_DOUBLE, KW_FLOAT, KW_ENUM, KW_STRUCT, KW_UNION,
        KW_IF, KW_ELSE, KW_DO, KW_WHILE, KW_FOR, KW_SWITCH,
        KW_CASE, KW_DEFAULT, KW_BREAK, KW_CONTINUE,
        KW_RETURN, KW_GOTO,

        // constant
        // CONST_ENUM is identifier
        CONST_INT, CONST_CHAR, CONST_FLOAT, STRING,

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

        // clang-format on
    };

    Type type;
    StringRef text;
    union {
        double fval;
        int ival;
        int cval;
    };

    bool operator==(const Token & other) const {
        return type == other.type && text == other.text;
    }
};

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

// Maybe just use Token::Type
class TokenMatcher {
public:
    TokenMatcher();
    TokenMatcher(Token token);

    bool match(const Token & token) const;

    std::string toString() const;

    bool operator==(const TokenMatcher & other) const;

private:
    Token token_;
};

// Maybe just use std::set<Token::Type>
class TokenMatcherSet {
public:
    void addMatcher(TokenMatcher matcher);
    void addMatchers(const TokenMatcherSet & matchers);

    bool match(const Token & token) const;

    size_t size() const;
    bool empty() const;
    const std::vector<TokenMatcher> & matchers() const;

private:
    std::vector<TokenMatcher> matchers_;
};

class SourceSanner {
public:
    SourceSanner() = default;
    explicit SourceSanner(StringRef source);
    StringRef readLine();
    bool eof() const;

private:
    std::string source_;
    size_t i_;
};

class Tokenizer {
public:
    void compile(SourceSanner & scanner);
    TokenIterator getIterator();

private:
    std::vector<Token> tokens_;
};

Token TokenFromString(const char *s);

#define EOL ('\n')
