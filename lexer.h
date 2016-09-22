#pragma once

#include <vector>
#include <deque>
#include <string>
#include <iostream>
#include <cassert>
#include <cctype>
using namespace std;

#define ELEMENT_COUNT(a) (sizeof(a) / sizeof((a)[0]))

class StringBuf
{
public:
    explicit StringBuf(const char *data)
    {
        assert(data != nullptr);

        size_t count = static_cast<size_t>(strlen(data));
        char * data2 = new char [count + 1];

        copy(data, data + count, data2);
        data2[count] = '\0';

        begin = now = data2;
        end = begin + count;
    }

    StringBuf(const char *data, size_t n)
    {
        assert(data != nullptr);

        size_t count = static_cast<size_t>(strlen(data));
        count = (count > n) ? n : count;
        char * data2 = new char [count + 1];

        copy(data, data + count, data2);
        data2[count] = '\0';

        begin = now = data2;
        end = begin + count;
    }

    ~StringBuf()
    {
        delete[] begin;
    }

    char peak(size_t offset = 0)
    {
        if (now + offset < end) return now[offset];
        else return '\0';
    }
    void pop(size_t count = 1)
    {
        const char * now2 = now + count;
        if (now2 < now || now2 > end) now = end;
        else now = now2;
    }
    bool empty()
    {
        return now == end;
    }
    size_t size()
    {
        return end - now;
    }
    const char * data()
    {
        return now;
    }

private:
    const char *begin, *end, *now;
};

class StringRef
{
public:
    StringRef()
    {
        begin = end = nullptr;
    }
    explicit StringRef(const char *data)
    {
        assert(data != nullptr);

        begin = data;
        end = data + strlen(begin);
    }
    // fast or safety ? fast
    StringRef(const char *data, size_t n)
    {
        assert(data != nullptr);

        begin = data;
        end = data + n;
    }

    friend bool operator == (const StringRef &s1, const char *s2)
    {
        assert(s2 != nullptr);

        const char *p1 = s1.begin, *p2 = s2;
        while (p1 != s1.end && *p2 != '\0')
        {
            if (*p1 != *p2) return false;
            ++p1, ++p2;
        }
        return p1 == s1.end && *p2 == '\0';
    }
    friend bool operator == (const char *s1, const StringRef &s2)
    {
        return s2 == s1;
    }
    friend ostream & operator << (ostream &o, const StringRef &s)
    {
        copy(s.begin, s.end, ostream_iterator<char>(o));
        return o;
    }

private:
    const char *begin, *end;
};

typedef enum TokenType
{
	NONE,
	/* punc: ( ) [ ] { } ; ... */
    LP, RP, LSB, RSB, BLK_BEGIN, BLK_END, STMT_END, VAR_PARAM,
	/* constant */
	CONST_INT, CONST_CHAR, CONST_FLOAT, CONST_ENUM, STRING,
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
	/* sizeof operator */
    OP_SIZEOF,
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

struct Token
{
	TokenType type;
    //long typeid_; // to support user defined type
	union
	{
        int symid;
        int strid;
		int ival;
		char cval;
        float fval;
        double dval;
	};

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
            case SYMBOL: o << "id(" << t.symid << ')'; break;
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
            case OP_SIZEOF: o << "sizeof"; break;
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

	        //CONST_INT, CONST_CHAR, CONST_FLOAT, CONST_ENUM, STRING,
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

    bool _read_token(StringBuf &input, Token &t)
    {
        t.type = NONE;

        // handle punc & REFER_TO
        switch (input.peak())
        {
            case '(': t.type = LP; break;
            case ')': t.type = RP; break;
            case '[': t.type = LSB; break;
            case ']': t.type = RSB; break;
            case '{': t.type = BLK_BEGIN; break;
            case '}': t.type = BLK_END; break;
            case ';': t.type = STMT_END; break;
            case '.':
                t.type = (input.peak(1) == '.' && input.peak(2) == '.') ?
                    VAR_PARAM : REFER_TO;
                break;
            default:
                break;
        }
        if (t.type != NONE)
        {
            if (t.type == VAR_PARAM) input.pop(3);
            else input.pop();
            return true;
        }

        // TODO: handle constant

        // handle keyword & id & OP_SIZEOF
        const char *l = input.data();
        const char *r = l;
        if (*r == '_'
           || (*r >= 'a' && *r <= 'z')
           || (*r >= 'A' && *r <= 'Z'))
        {
            ++r;
            while (*r == '_'
                    || (*r >= 'a' && *r <= 'z')
                    || (*r >= 'A' && *r <= 'Z')
                    || (*r >= '0' && *r <= '9'))
            {
                ++r;
            }

            StringRef sp(l, r - l);
            for (int i = 0; i < ELEMENT_COUNT(ALL_KEYWORD); ++i)
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
                t.symid = symbols.size();
                symbols.push_back(sp);
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
            case '?': t.type = OP_QMARK, oplen = 1; break;
            case ':': t.type = OP_COLON, oplen = 1; break;
            case ',': t.type = OP_COMMA, oplen = 1; break;
            case '=':
                if (input.peak(1) == '=') t.type = REL_EQ, oplen = 2;
                else t.type = ASSIGN, oplen = 1;
                break;
            case '+':
                if (input.peak(1) == '=') t.type = ASSIGN_ADD, oplen = 2;
                else if (input.peak(1) == '+') t.type = OP_INC, oplen = 2;
                else t.type = OP_ADD, oplen = 1;
                break;
            case '-':
                if (input.peak(1) == '=') t.type = ASSIGN_SUB, oplen = 2;
                else if (input.peak(1) == '>') t.type = POINT_TO, oplen = 2;
                else if (input.peak(1) == '-') t.type = OP_DEC, oplen = 2;
                else t.type = OP_SUB, oplen = 1;
                break;
            case '*':
                if (input.peak(1) == '=') t.type = ASSIGN_MUL, oplen = 2;
                else t.type = OP_MUL, oplen = 1;
                break;
            case '/':
                if (input.peak(1) == '=') t.type = ASSIGN_DIV, oplen = 2;
                else t.type = OP_DIV, oplen = 1;
                break;
            case '%':
                if (input.peak(1) == '=') t.type = ASSIGN_MOD, oplen = 2;
                else t.type = OP_MOD, oplen = 1;
                break;
            case '<':
                if (input.peak(1) == '=') t.type = REL_LE, oplen = 2;
                else if (input.peak(1) == '<')
                {
                    if (input.peak(2) == '=') t.type = ASSIGN_SLEFT, oplen = 3;
                    else t.type = BIT_SLEFT, oplen = 2;
                }
                else t.type = REL_LT, oplen = 1;
                break;
            case '>':
                if (input.peak(1) == '=') t.type = REL_GE, oplen = 1;
                else if (input.peak(1) == '>')
                {
                    if (input.peak(2) == '=') t.type = ASSIGN_SRIGHT, oplen = 3;
                    else t.type = BIT_SRIGHT, oplen = 2;
                }
                else t.type = REL_GT, oplen = 1;
                break;
            case '&':
                if (input.peak(1) == '=') t.type = ASSIGN_AND, oplen = 2;
                else if (input.peak(1) == '&') t.type = BOOL_AND, oplen = 2;
                else t.type = BIT_AND, oplen = 1;
                break;
            case '|':
                if (input.peak(1) == '=') t.type = ASSIGN_OR, oplen = 2;
                else if (input.peak(1) == '|') t.type = BOOL_OR, oplen = 2;
                else t.type = BIT_OR, oplen = 1;
                break;
            case '^':
                if (input.peak(1) == '=') t.type = ASSIGN_XOR, oplen = 2;
                else t.type = BIT_XOR, oplen = 1;
                break;
            case '~': t.type = BIT_NOT, oplen = 1; break;
            case '!':
                if (input.peak(1) == '=') t.type = REL_NE, oplen = 2;
                else t.type = BOOL_NOT, oplen = 1;
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

	void LexError(const char *msg) const
	{
		cout << "Lexer: " << msg << endl;
		exit(1);
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
			if (_read_token(input, t))
			{
				//cout << "Token: " << t << endl;
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

    bool hasNext() const
    {
        return !tokens.empty();
    }

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

    StringRef symbolName(size_t symid)
    {
        if (symid < symbols.size()) return symbols[symid];
        else return StringRef();
    }

private:
    vector<StringRef> symbols; // symbol names
    deque<Token> tokens;
    int lnum;
    const char * lstart;
    //vector<StringRef> strings; // string constants
};

