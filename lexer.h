#pragma once

#include <cassert>
#include <cctype>
#include <deque>
#include <iostream>
#include <string>
#include <vector>
using namespace std;

#include "common.h"

typedef enum TokenType
{
	NONE,
	/* punc: ( ) [ ] { } ; ... */
    LP, RP, LSB, RSB, BLK_BEGIN, BLK_END, STMT_END, VAR_PARAM,
	/* constant */
	CONST_INT, CONST_CHAR, CONST_FLOAT, STRING, // CONST_ENUM is SYMBOL
	/* reserved word */
    TYPEDEF, SIZEOF,
	AUTO, REGISTER, VOLATILE, EXTERN, CONST, STATIC, UNSIGNED, SIGNED,
    TYPE_VOID, TYPE_INT, TYPE_LONG, TYPE_SHORT, TYPE_CHAR,
    TYPE_DOUBLE, TYPE_FLOAT, TYPE_ENUM, TYPE_STRUCT, TYPE_UNION,
    IF, ELSE, DO, WHILE, FOR, SWITCH, CASE, DEFAULT, BREAK, CONTINUE,
    RETURN, GOTO,
	/* identifier */
	SYMBOL,

    // sizeof: reserved word + operator
    // *: dereference + multiplier

	/* address operator */
    REFER_TO, POINT_TO,
    /* condition operator */
    OP_QMARK, OP_COLON,
    /* comma operator */
    OP_COMMA,
	/* assignment operator */
    ASSIGN, ASSIGN_ADD, ASSIGN_SUB, ASSIGN_MUL, ASSIGN_DIV, ASSIGN_MOD,
    ASSIGN_SLEFT, ASSIGN_SRIGHT, ASSIGN_AND, ASSIGN_OR, ASSIGN_XOR,
    OP_INC, OP_DEC,
    /* arithmatic operator */
	OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD,
	/* bool operator */
	BOOL_AND, BOOL_OR, BOOL_NOT,
	/* relation operator */
	REL_EQ, REL_NE, REL_GT, REL_GE, REL_LT, REL_LE,
	/* bit operator */
	BIT_AND, BIT_OR, BIT_XOR, BIT_NOT, BIT_SLEFT, BIT_SRIGHT
} TokenType;

typedef enum TokenTypeCategory
{
    TOKEN_CATEGORY_NONE = 0,
    TOKEN_CATEGORY_ASSIGN_OPERATOR = 1,
} TokenTypeCategory;

struct Token
{
	TokenType type;
    int category;
	union
	{
        StringRef symbol;
        StringRef string_;
		int ival;
		char cval;
        float fval;
        double dval;
	};
    // debug
    int line;

    Token() : symbol() {}

    static string DebugTokenType(TokenType t)
    {
        string s = "";
        switch (t)
        {
            case LP: s = "("; break;
            case RP: s = ")"; break;
            case LSB: s = "["; break;
            case RSB: s = "]"; break;
            case BLK_BEGIN: s = "{"; break;
            case BLK_END: s = "}"; break;
            case STMT_END: s = ";"; break;
            case VAR_PARAM: s = "..."; break;
            case SYMBOL: s = "id"; break;
            case REFER_TO: s = "."; break;
            case POINT_TO: s = "->"; break;
            case TYPEDEF: s = "typedef"; break;
            case SIZEOF: s = "sizeof"; break;
            case AUTO: s = "auto"; break;
            case REGISTER: s = "register"; break;
            case VOLATILE: s = "volatile"; break;
            case EXTERN: s = "extern"; break;
            case CONST: s = "const"; break;
            case STATIC: s = "static"; break;
            case UNSIGNED: s = "unsigned"; break;
            case SIGNED: s = "signed"; break;
            case TYPE_VOID: s = "void"; break;
            case TYPE_INT: s = "int"; break;
            case TYPE_LONG: s = "long"; break;
            case TYPE_SHORT: s = "short"; break;
            case TYPE_CHAR: s = "char"; break;
            case TYPE_DOUBLE: s = "double"; break;
            case TYPE_FLOAT: s = "float"; break;
            case TYPE_ENUM: s = "enum"; break;
            case TYPE_STRUCT: s = "struct"; break;
            case TYPE_UNION: s = "union"; break;
            case IF: s = "if"; break;
            case ELSE: s = "else"; break;
            case DO: s = "do"; break;
            case WHILE: s = "while"; break;
            case FOR: s = "for"; break;
            case SWITCH: s = "switch"; break;
            case CASE: s = "case"; break;
            case DEFAULT: s = "default"; break;
            case BREAK: s = "break"; break;
            case CONTINUE: s = "continue"; break;
            case RETURN: s = "return"; break;
            case GOTO: s = "goto"; break;
            case OP_QMARK: s = "?"; break;
            case OP_COLON: s = ":"; break;
            case OP_COMMA: s = ","; break;
            case ASSIGN: s = "="; break;
            case ASSIGN_ADD: s = "+="; break;
            case ASSIGN_SUB: s = "-="; break;
            case ASSIGN_MUL: s = "*="; break;
            case ASSIGN_DIV: s = "/="; break;
            case ASSIGN_MOD: s = "%="; break;
            case ASSIGN_SLEFT: s = "<<="; break;
            case ASSIGN_SRIGHT: s = ">>="; break;
            case ASSIGN_AND: s = "&="; break;
            case ASSIGN_OR: s = "|="; break;
            case ASSIGN_XOR: s = "^="; break;
            case OP_INC: s = "++"; break;
            case OP_DEC: s = "--"; break;
            case OP_ADD: s = "+"; break;
            case OP_SUB: s = "-"; break;
            case OP_MUL: s = "*"; break;
            case OP_DIV: s = "/"; break;
            case OP_MOD: s = "%"; break;
            case BOOL_AND: s = "&&"; break;
            case BOOL_OR: s = "||"; break;
            case BOOL_NOT: s = "!"; break;
            case REL_EQ: s = "=="; break;
            case REL_NE: s = "!="; break;
            case REL_GT: s = ">"; break;
            case REL_GE: s = ">="; break;
            case REL_LT: s = "<"; break;
            case REL_LE: s = "<="; break;
            case BIT_AND: s = "&"; break;
            case BIT_OR: s = "|"; break;
            case BIT_XOR: s = "^"; break;
            case BIT_NOT: s = "~"; break;
            case BIT_SLEFT: s = "<<"; break;
            case BIT_SRIGHT: s = ">>"; break;

            case CONST_CHAR: s = "char"; break;
            case CONST_INT: s = "int"; break;
            case CONST_FLOAT: s = "float"; break;
            case STRING: s = "string"; break;
            default: s = "unknown"; break;
        }
        return s;
    }
    friend ostream & operator << (ostream &o, const Token &t)
    {
        switch (t.type)
        {
            case LP: o << '('; break;
            case RP: o << ')'; break;
            case LSB: o << '['; break;
            case RSB: o << ']'; break;
            case BLK_BEGIN: o << '{'; break;
            case BLK_END: o << '}'; break;
            case STMT_END: o << ';'; break;
            case VAR_PARAM: o << "..."; break;
            case SYMBOL: o << "id(" << t.symbol.toString() << ')'; break;
            case REFER_TO: o << '.'; break;
            case POINT_TO: o << "->"; break;
            case TYPEDEF: o << "typedef"; break;
            case SIZEOF: o << "sizeof"; break;
            case AUTO: o << "auto"; break;
            case REGISTER: o << "register"; break;
            case VOLATILE: o << "volatile"; break;
            case EXTERN: o << "extern"; break;
            case CONST: o << "const"; break;
            case STATIC: o << "static"; break;
            case UNSIGNED: o << "unsigned"; break;
            case SIGNED: o << "signed"; break;
            case TYPE_VOID: o << "void"; break;
            case TYPE_INT: o << "int"; break;
            case TYPE_LONG: o << "long"; break;
            case TYPE_SHORT: o << "short"; break;
            case TYPE_CHAR: o << "char"; break;
            case TYPE_DOUBLE: o << "double"; break;
            case TYPE_FLOAT: o << "float"; break;
            case TYPE_ENUM: o << "enum"; break;
            case TYPE_STRUCT: o << "struct"; break;
            case TYPE_UNION: o << "union"; break;
            case IF: o << "if"; break;
            case ELSE: o << "else"; break;
            case DO: o << "do"; break;
            case WHILE: o << "while"; break;
            case FOR: o << "for"; break;
            case SWITCH: o << "switch"; break;
            case CASE: o << "case"; break;
            case DEFAULT: o << "default"; break;
            case BREAK: o << "break"; break;
            case CONTINUE: o << "continue"; break;
            case RETURN: o << "return"; break;
            case GOTO: o << "goto"; break;
            case OP_QMARK: o << "?"; break;
            case OP_COLON: o << ":"; break;
            case OP_COMMA: o << ","; break;
            case ASSIGN: o << "="; break;
            case ASSIGN_ADD: o << "+="; break;
            case ASSIGN_SUB: o << "-="; break;
            case ASSIGN_MUL: o << "*="; break;
            case ASSIGN_DIV: o << "/="; break;
            case ASSIGN_MOD: o << "%="; break;
            case ASSIGN_SLEFT: o << "<<="; break;
            case ASSIGN_SRIGHT: o << ">>="; break;
            case ASSIGN_AND: o << "&="; break;
            case ASSIGN_OR: o << "|="; break;
            case ASSIGN_XOR: o << "^="; break;
            case OP_INC: o << "++"; break;
            case OP_DEC: o << "--"; break;
            case OP_ADD: o << "+"; break;
            case OP_SUB: o << "-"; break;
            case OP_MUL: o << "*"; break;
            case OP_DIV: o << "/"; break;
            case OP_MOD: o << "%"; break;
            case BOOL_AND: o << "&&"; break;
            case BOOL_OR: o << "||"; break;
            case BOOL_NOT: o << "!"; break;
            case REL_EQ: o << "=="; break;
            case REL_NE: o << "!="; break;
            case REL_GT: o << ">"; break;
            case REL_GE: o << ">="; break;
            case REL_LT: o << "<"; break;
            case REL_LE: o << "<="; break;
            case BIT_AND: o << "&"; break;
            case BIT_OR: o << "|"; break;
            case BIT_XOR: o << "^"; break;
            case BIT_NOT: o << "~"; break;
            case BIT_SLEFT: o << "<<"; break;
            case BIT_SRIGHT: o << ">>"; break;

            case CONST_CHAR: o << "char('" << t.cval << "')"; break;
            case CONST_INT: o << "int(" << t.ival << ")"; break;
            case CONST_FLOAT: o << "float(--)"; break;
            case STRING: o << "string(\"" << t.string_ << "\")"; break;
            default: o << "unknown"; break;
        }
        return o;
    }
};

static const char * ALL_KEYWORD[] = {
    "auto","double","int","struct",
    "break","else","long","switch",
    "case","enum","register","typedef",
    "char","extern","return","union",
    "const","float","short","unsigned",
    "continue","for","signed","void",
    "default","goto","sizeof","volatile",
    "do","if","static","while"
};

static TokenType ALL_KEYWORD_TYPE[] = {
    AUTO, TYPE_DOUBLE, TYPE_INT, TYPE_STRUCT,
    BREAK, ELSE, TYPE_LONG, SWITCH,
    CASE, TYPE_ENUM, REGISTER, TYPEDEF,
    TYPE_CHAR, EXTERN, RETURN, TYPE_UNION,
    CONST, TYPE_FLOAT, TYPE_SHORT, UNSIGNED,
    CONTINUE, FOR, SIGNED, TYPE_VOID,
    DEFAULT, GOTO, SIZEOF, VOLATILE,
    DO, IF, STATIC, WHILE
};

class Lexer
{
    void _skip_spaces(StringBuf &input)
    {
        while (isspace(input.peak()))
        {
            if (input.peak() == '\n')
            {
                ++lnum;
                lstart = input.data();
            }
            input.pop();
        }
    }

    int _parse_int(const char *start, int &ival)
    {
        const char *p = start;
        int iv = 0;
        if (*p == '0')
        {
            ++p;
            if (*p == 'x' || *p == 'X')
            {
                ++p;
                // hex
                while (true)
                {
                    iv *= 16;
                    if (*p >= '0' && *p <= '9')
                        iv += *p - '0';
                    else if (*p >= 'a' && *p <= 'z')
                        iv += 10 + (*p - 'a');
                    else if (*p >= 'A' && *p <= 'Z')
                        iv += 10 + (*p - 'A');
                    else
                        break;
                    ++p;
                }
            }
            else
            {
                // oct
                while (*p >= '0' && *p <= '7')
                {
                    iv *= 8;
                    iv += *p - '0';
                    ++p;
                }
            }
        }
        else if (*p >= '1' && *p <= '9')
        {
            // dec
            do
            {
                iv *= 10;
                iv += *p - '0';
                ++p;
            } while (*p >= '0' && *p <= '9');
        }

        if (p > start)
            ival = iv;
        return p - start;
    }

    bool _read_token(StringBuf &input, Token &t)
    {
        t.type = NONE;
        t.category = TOKEN_CATEGORY_NONE;

        // handle punc & REFER_TO
        const char *str_begin, *str_end;
        switch (input.peak())
        {
            case '(':
                t.type = LP;
                break;
            case ')':
                t.type = RP;
                break;
            case '[':
                t.type = LSB;
                break;
            case ']':
                t.type = RSB;
                break;
            case '{':
                t.type = BLK_BEGIN;
                break;
            case '}':
                t.type = BLK_END;
                break;
            case ';':
                t.type = STMT_END;
                break;
            case '.':
                t.type = (input.peak(1) == '.' && input.peak(2) == '.')
                             ? VAR_PARAM
                             : REFER_TO;
                break;
            case '"':
                t.type = STRING;
                input.pop();
                str_begin = input.data();
                str_end = str_begin;
                while (*str_end != '"')
                {
                    if (*str_end == '\0')
                        LexError("unmatched \"");
                    else
                        ++str_end;
                }
                t.string_ = StringRef(str_begin, str_end - str_begin);
                input.pop(str_end - str_begin);
                break;
            case '\'':
                // TODO: support '\' syntax
                t.type = CONST_CHAR;
                input.pop();
                t.cval = input.peak();
                if (t.cval == '\0')
                    LexError("unmatched '");
                else if (t.cval == '\'')
                    LexError("invalid character constant, empty declaration");
                else if (t.cval == '\\')
                {
                    input.pop();
                    t.cval = input.peak() - '0';
                }
                input.pop();
                if (input.peak() != '\'')
                    LexError("expect '");
                break;
            default:
                break;
        }
        if (t.type != NONE)
        {
            if (t.type == VAR_PARAM)
                input.pop(3);
            else
                input.pop();
            return true;
        }

        // TODO: handle float constant
        int len = _parse_int(input.data(), t.ival);
        if (len > 0)
        {
            t.type = CONST_INT;
            input.pop(len);
            return true;
        }

        // handle keyword & id & OP_SIZEOF
        const char *l = input.data();
        const char *r = l;
        if (*r == '_' || (*r >= 'a' && *r <= 'z') || (*r >= 'A' && *r <= 'Z'))
        {
            ++r;
            while (*r == '_' || (*r >= 'a' && *r <= 'z') ||
                   (*r >= 'A' && *r <= 'Z') || (*r >= '0' && *r <= '9'))
            {
                ++r;
            }

            StringRef sp(l, r - l);
            for (size_t i = 0; i < ELEMENT_COUNT(ALL_KEYWORD); ++i)
            {
                if (sp == ALL_KEYWORD[i])
                {
                    t.type = ALL_KEYWORD_TYPE[i];
                    break;
                }
            }
            if (r != l && t.type == NONE)
            {
                t.type = SYMBOL;
                t.symbol = sp;
            }
        }
        if (t.type != NONE)
        {
            input.pop(r - l);
            return true;
        }

        // handle operator
        int oplen = 0;
        switch (input.peak())
        {
            case '?':
                t.type = OP_QMARK, oplen = 1;
                break;
            case ':':
                t.type = OP_COLON, oplen = 1;
                break;
            case ',':
                t.type = OP_COMMA, oplen = 1;
                break;
            case '=':
                if (input.peak(1) == '=')
                    t.type = REL_EQ, oplen = 2;
                else
                    t.type = ASSIGN, t.category |= TOKEN_CATEGORY_ASSIGN_OPERATOR, oplen = 1;
                break;
            case '+':
                if (input.peak(1) == '=')
                    t.type = ASSIGN_ADD, t.category |= TOKEN_CATEGORY_ASSIGN_OPERATOR, oplen = 2;
                else if (input.peak(1) == '+')
                    t.type = OP_INC, oplen = 2;
                else
                    t.type = OP_ADD, oplen = 1;
                break;
            case '-':
                if (input.peak(1) == '=')
                    t.type = ASSIGN_SUB, t.category |= TOKEN_CATEGORY_ASSIGN_OPERATOR, oplen = 2;
                else if (input.peak(1) == '>')
                    t.type = POINT_TO, oplen = 2;
                else if (input.peak(1) == '-')
                    t.type = OP_DEC, oplen = 2;
                else
                    t.type = OP_SUB, oplen = 1;
                break;
            case '*':
                if (input.peak(1) == '=')
                    t.type = ASSIGN_MUL, t.category |= TOKEN_CATEGORY_ASSIGN_OPERATOR, oplen = 2;
                else
                    t.type = OP_MUL, oplen = 1;
                break;
            case '/':
                if (input.peak(1) == '=')
                    t.type = ASSIGN_DIV, t.category |= TOKEN_CATEGORY_ASSIGN_OPERATOR, oplen = 2;
                else
                    t.type = OP_DIV, oplen = 1;
                break;
            case '%':
                if (input.peak(1) == '=')
                    t.type = ASSIGN_MOD, t.category |= TOKEN_CATEGORY_ASSIGN_OPERATOR, oplen = 2;
                else
                    t.type = OP_MOD, oplen = 1;
                break;
            case '<':
                if (input.peak(1) == '=')
                    t.type = REL_LE, oplen = 2;
                else if (input.peak(1) == '<')
                {
                    if (input.peak(2) == '=')
                        t.type = ASSIGN_SLEFT, t.category |= TOKEN_CATEGORY_ASSIGN_OPERATOR, oplen = 3;
                    else
                        t.type = BIT_SLEFT, oplen = 2;
                }
                else
                    t.type = REL_LT, oplen = 1;
                break;
            case '>':
                if (input.peak(1) == '=')
                    t.type = REL_GE, oplen = 2;
                else if (input.peak(1) == '>')
                {
                    if (input.peak(2) == '=')
                        t.type = ASSIGN_SRIGHT, t.category |= TOKEN_CATEGORY_ASSIGN_OPERATOR, oplen = 3;
                    else
                        t.type = BIT_SRIGHT, oplen = 2;
                }
                else
                    t.type = REL_GT, oplen = 1;
                break;
            case '&':
                if (input.peak(1) == '=')
                    t.type = ASSIGN_AND, t.category |= TOKEN_CATEGORY_ASSIGN_OPERATOR, oplen = 2;
                else if (input.peak(1) == '&')
                    t.type = BOOL_AND, oplen = 2;
                else
                    t.type = BIT_AND, oplen = 1;
                break;
            case '|':
                if (input.peak(1) == '=')
                    t.type = ASSIGN_OR, t.category |= TOKEN_CATEGORY_ASSIGN_OPERATOR, oplen = 2;
                else if (input.peak(1) == '|')
                    t.type = BOOL_OR, oplen = 2;
                else
                    t.type = BIT_OR, oplen = 1;
                break;
            case '^':
                if (input.peak(1) == '=')
                    t.type = ASSIGN_XOR, t.category |= TOKEN_CATEGORY_ASSIGN_OPERATOR, oplen = 2;
                else
                    t.type = BIT_XOR, oplen = 1;
                break;
            case '~':
                t.type = BIT_NOT, oplen = 1;
                break;
            case '!':
                if (input.peak(1) == '=')
                    t.type = REL_NE, oplen = 2;
                else
                    t.type = BOOL_NOT, oplen = 1;
                break;
            default:
                break;
        }
        if (t.type != NONE)
        {
            input.pop(oplen);
            return true;
        }

        return false;
    }

   public:
    void tokenize(StringBuf &input)
    {
        tokens.clear();
        lnum = 1;
        lstart = input.data();
        while (true)
        {
            _skip_spaces(input);
            if (input.empty())
                break;
            Token t;
            t.line = lnum;
            if (_read_token(input, t))
            {
                // cout << "Token: " << t << endl;
                tokens.push_back(t);
            }
            else
            {
                string msg = "Unrecognized Token at line " + to_string(lnum) +
                             ":" + to_string(input.data() - lstart + 1);
                LexError(msg.data());
            }
        }
    }

    bool hasNext() const { return !tokens.empty(); }
    Token getNext()
    {
        if (tokens.empty())
        {
            LexError("No more tokens");
        }
        Token t = tokens.front();
        tokens.pop_front();
        return t;
    }

    Token peakNext(size_t n = 0)
    {
        if (tokens.size() < n + 1)
        {
            LexError("Not enough tokens");
        }
        return tokens[n];
    }

    // void expect(TokenType t)
    // {
    //     expectGet(t);
    // }
    // Token expectGet(TokenType t)
    // {
    //     if (tokens.empty() || tokens[0].type != t)
    //     {
    //         Token tmp;
    //         tmp.type = t;
    //         cout << "Syntax: Expect token " << tmp << endl;
    //         exit(1);
    //     }
    //     Token tk = tokens.front();
    //     tokens.pop_front();
    //     return tk;
    // }

    void debugPrint()
    {
        deque<Token> cp = tokens;
        while (hasNext())
        {
            Token t = peakNext();
            cout << "Token: " << t << endl;
            getNext();
        }
        tokens = cp;
    }

   private:
    deque<Token> tokens;
    int lnum;
    const char *lstart;
};

