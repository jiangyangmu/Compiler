#include "Tokenizer.h"

#include <cctype>
#include <exception>
#include <unordered_map>
#include <xutility>

const char * ParsePPIncludedFilePath(const char * pcBegin);
const char * ParsePPLParan(const char * pcBegin);

static inline bool isoctdigit(int c) {
    return c >= '0' && c <= '7';
}

// \' \" \? \\ \a \b \f \n \r \t \v
static inline bool isescape(int c) {
    bool escape = false;
    switch (c)
    {
        case '\'':
        case '"':
        case '?':
        case '\\':
        case 'a':
        case 'b':
        case 'f':
        case 'n':
        case 'r':
        case 't':
        case 'v':
            escape = true;
        default:
            break;
    }
    return escape;
}

struct TokenRecog {
    enum Category {
        UNKNOWN,
        ID,
        FLOAT,
        INT,
        CHAR,
        STRING,
        OP_OR_PUNC,
        PP_DIRECTIVE,
    };

    struct Info {
        Category category;

        Info()
            : category(UNKNOWN) {
        }
    };
};

// Stop at the first non-space character.
static inline const char * TokenStart(const char * p) {
    while (isspace(*p))
        ++p;
    return p;
}

// Stop at the first character after current "id", or start if not match.
static inline const char * IdEnd(const char * start) {
    if (*start == '_' || isalpha(*start))
    {
        ++start;
        while (*start == '_' || isalnum(*start))
            ++start;
    }
    return start;
}

// Stop at the first character after current "float", or start if not match.
static inline const char * FloatConstEnd(const char * start) {
    const char * end = start;

    // fractional constant
    bool has_dot = false;
    bool has_digit = false;
    while (isdigit(*end))
        ++end, has_digit = true;
    if (*end == '.')
        ++end, has_dot = true;
    while (isdigit(*end))
        ++end, has_digit = true;
    if (!has_digit)
        return start;

    // exponent part
    if (!has_dot && tolower(*end) != 'e')
        return start;
    if (tolower(*end) == 'e')
    {
        ++end;
        if (*end == '+' || *end == '-')
            ++end;
        while (isdigit(*end))
            ++end;
    }

    // floating suffix
    if (tolower(*end) == 'f' || tolower(*end) == 'l')
        ++end;

    return end;
}

// Stop at the first character after current "int", or start if not match.
static inline const char * IntConstEnd(const char * start) {
    const char * end = start;

    if (*end >= '0' && *end <= '9') // decimal constant
    {
        ++end;
        while (isdigit(*end))
            ++end;
    }
    else if (*end == '0' && tolower(*(end + 1)) != 'x') // octal constant
    {
        ++end;
        while (isoctdigit(*end))
            ++end;
    }
    else if (*end == '0' && tolower(*(end + 1)) == 'x') // hexadecimal constant
    {
        ++end;

        if (!isxdigit(*end))
            return start;
        ++end;

        while (isxdigit(*end))
            ++end;
    }

    // integer suffix
    while (tolower(*end) == 'u' || tolower(*end) == 'l')
        ++end;

    return end;
}

// Stop at the first character after current "escape sequence", or start if not
// match.
static inline const char * EscapeSequenceEnd(const char * start) {
    if (*start != '\\')
        return start;
    ++start;

    if (isescape(*start)) // simple escape sequence
        ++start;
    else if (isoctdigit(*start)) // octal escape sequence
    {
        ++start;
        if (isoctdigit(*start))
            ++start;
        if (isoctdigit(*start))
            ++start;
    }
    else if (*start == 'x') // hexadecimal escape sequence
    {
        ++start;
        while (isxdigit(*start))
            ++start;
    }
    return start;
}

// Stop at the first character after current "char", or start if not match.
static inline const char * CharConstEnd(const char * start) {
    const char * end = start;

    if (*end == 'L')
        ++end;

    if (*end != '\'')
        return start;
    ++end;
    if (*end == '\'')
        return start;

    do
    {
        if (*end == 0)
            return start;

        if (*end == '\\') // escape sequence
        {
            const char * p = EscapeSequenceEnd(end);
            if (p == end)
                return start;
            end = p;
        }
        else // non escape character
            ++end;
    } while (*end != '\'');
    ++end;

    return end;
}

// Stop at the first character after current "string", or start if not match.
static inline const char * StringLiteralEnd(const char * start) {
    const char * end = start;

    if (*end == 'L')
        ++end;

    if (*end != '"')
        return start;
    ++end;

    while (*end != '"')
    {
        if (*end == 0)
            return start;

        if (*end == '\\') // escape sequence
        {
            const char * p = EscapeSequenceEnd(end);
            if (p == end)
                return start;
            end = p;
        }
        else // non escape character
            ++end;
    };
    ++end;

    return end;
}

// Stop at the first character after current "operator/punctuator", or start if
// not match.
static inline const char * OperatorPunctuatorEnd(const char * start) {
    switch (*start)
    {
        case '(':
        case ')':
        case ',':
        case ':':
        case ';':
        case '?':
        case '[':
        case ']':
        case '{':
        case '}':
            ++start;
            break;
        case '!':
        case '%':
        case '*':
        case '/':
        case '^':
        case '~':
        case '=':
            ++start;
            if (*start == '=')
                ++start;
            break;
        case '+':
            ++start;
            if (*start == '=' || *start == '+')
                ++start;
            break;
        case '&':
            ++start;
            if (*start == '=' || *start == '&')
                ++start;
            break;
        case '|':
            ++start;
            if (*start == '=' || *start == '|')
                ++start;
            break;
        case '.':
            ++start;
            if (*start == '.' && *(start + 1) == '.')
                start += 2;
            break;
        case '-':
            ++start;
            if (*start == '-' || *start == '=' || *start == '>')
                ++start;
            break;
        case '<':
            ++start;
            if (*start == '=')
                ++start;
            else if (*start == '<')
            {
                ++start;
                if (*start == '=')
                    ++start;
            }
            break;
        case '>':
            ++start;
            if (*start == '=')
                ++start;
            else if (*start == '>')
            {
                ++start;
                if (*start == '=')
                    ++start;
            }
            break;
        default:
            break;
    }
    return start;
}

static inline const char * PPDirectiveEnd(const char * start)
{
    if (*start == '#')
    {
        ++start;
        return IdEnd(start);
    }
    return start;
}

// Stop at the first character after current token, or start if not match.
static inline const char * TokenEnd(const char * start,
                                    TokenRecog::Info * info) {
    const char * end;

    if ((end = IdEnd(start)) > start) // id or keyword
    {
        info->category = TokenRecog::ID;
        return end;
    }
    if ((end = FloatConstEnd(start)) > start) // float-constant
    {
        info->category = TokenRecog::FLOAT;
        return end;
    }
    if ((end = IntConstEnd(start)) > start) // int-constant
    {
        info->category = TokenRecog::INT;
        return end;
    }
    if ((end = CharConstEnd(start)) > start) // char-constant
    {
        info->category = TokenRecog::CHAR;
        return end;
    }
    if ((end = StringLiteralEnd(start)) > start) // string
    {
        info->category = TokenRecog::STRING;
        return end;
    }
    if ((end = OperatorPunctuatorEnd(start)) > start) // operator or punctuator
    {
        info->category = TokenRecog::OP_OR_PUNC;
        return end;
    }
    if ((end = PPDirectiveEnd(start)) > start) // preprocess directive
    {
        info->category = TokenRecog::PP_DIRECTIVE;
        return end;
    }

    return start;
}

Token::Type KeywordType(StringRef text) {
    static std::unordered_map<std::string, Token::Type> keywords = {
        {"auto", Token::KW_AUTO},         {"break", Token::KW_BREAK},
        {"case", Token::KW_CASE},         {"char", Token::KW_CHAR},
        {"const", Token::KW_CONST},       {"continue", Token::KW_CONTINUE},
        {"default", Token::KW_DEFAULT},   {"do", Token::KW_DO},
        {"double", Token::KW_DOUBLE},     {"else", Token::KW_ELSE},
        {"enum", Token::KW_ENUM},         {"extern", Token::KW_EXTERN},
        {"float", Token::KW_FLOAT},       {"for", Token::KW_FOR},
        {"goto", Token::KW_GOTO},         {"if", Token::KW_IF},
        {"int", Token::KW_INT},           {"long", Token::KW_LONG},
        {"register", Token::KW_REGISTER}, {"return", Token::KW_RETURN},
        {"short", Token::KW_SHORT},       {"signed", Token::KW_SIGNED},
        {"sizeof", Token::KW_SIZEOF},     {"static", Token::KW_STATIC},
        {"struct", Token::KW_STRUCT},     {"switch", Token::KW_SWITCH},
        {"typedef", Token::KW_TYPEDEF},   {"union", Token::KW_UNION},
        {"unsigned", Token::KW_UNSIGNED}, {"void", Token::KW_VOID},
        {"volatile", Token::KW_VOLATILE}, {"while", Token::KW_WHILE},
    };

    if (keywords.find(text.toString()) != keywords.end())
        return keywords[text.toString()];
    else
        return Token::UNKNOWN;
}

Token::Type OperatorPunctuatorType(StringRef text) {
    static std::unordered_map<std::string, Token::Type>
        operators_and_punctuators = {
            {"--", Token::OP_DEC},      {"-", Token::OP_SUB},
            {"-=", Token::SUB_ASSIGN},  {"->", Token::OP_POINTTO},
            {"!", Token::BOOL_NOT},     {"!=", Token::REL_NE},
            {"%", Token::OP_MOD},       {"%=", Token::MOD_ASSIGN},
            {"&", Token::BIT_AND},      {"&&", Token::BOOL_AND},
            {"&=", Token::AND_ASSIGN},  {"(", Token::LP},
            {")", Token::RP},           {"*", Token::OP_MUL},
            {"*=", Token::MUL_ASSIGN},  {",", Token::OP_COMMA},
            {".", Token::OP_DOT},       {"...", Token::VAR_PARAM},
            {"/", Token::OP_DIV},       {"/=", Token::DIV_ASSIGN},
            {":", Token::OP_COLON},     {";", Token::STMT_END},
            {"?", Token::OP_QMARK},     {"[", Token::LSB},
            {"]", Token::RSB},          {"^", Token::BIT_XOR},
            {"^=", Token::XOR_ASSIGN},  {"{", Token::BLK_BEGIN},
            {"|", Token::BIT_OR},       {"||", Token::BOOL_OR},
            {"|=", Token::OR_ASSIGN},   {"}", Token::BLK_END},
            {"~", Token::BIT_NOT},      {"+", Token::OP_ADD},
            {"++", Token::OP_INC},      {"+=", Token::ADD_ASSIGN},
            {"<", Token::REL_LT},       {"<<", Token::BIT_SHL},
            {"<<=", Token::SHL_ASSIGN}, {"<=", Token::REL_LE},
            {"=", Token::ASSIGN},       {"==", Token::REL_EQ},
            {">", Token::REL_GT},       {">=", Token::REL_GE},
            {">>", Token::BIT_SHR},     {">>=", Token::SHR_ASSIGN},
        };
    if (operators_and_punctuators.find(text.toString()) !=
        operators_and_punctuators.end())
        return operators_and_punctuators[text.toString()];
    else
        return Token::UNKNOWN;
}

Token::Type PPDirectiveType(StringRef text) {
    static std::unordered_map<std::string, Token::Type>
        pp_directives = {
            {"#if", Token::PPD_IF},
            {"#ifdef", Token::PPD_IFDEF},
            {"#ifndef", Token::PPD_IFNDEF},
            {"#elif", Token::PPD_ELIF},
            {"#else", Token::PPD_ELSE},
            {"#endif", Token::PPD_ENDIF},
            {"#define", Token::PPD_DEFINE},
            {"#undef", Token::PPD_UNDEF},
            {"#include", Token::PPD_INCLUDE},
            {"#line", Token::PPD_LINE},
            {"#error", Token::PPD_ERROR},
            {"#pragma", Token::PPD_PRAGMA},
            {"#", Token::PPD_NULL},
        };
    if (pp_directives.find(text.toString()) !=
        pp_directives.end())
        return pp_directives[text.toString()];
    else
        return Token::UNKNOWN;
}

static inline double EvalFloat(StringRef text) {
    // TODO implement EvalFloat()
    int digits = 0;
    double factor = 1.0;
    bool afterDot = false;

    for (char c : text)
    {
        if (isdigit(c))
        {
            digits = digits * 10 + (c - '0');
            if (afterDot)
                factor *= 10.0;
        }
        else if (c == '.')
        {
            ASSERT(afterDot == false);
            afterDot = true;
        }
        else
        {
            break;
        }
    }
    return (double)digits / factor;
}

static inline int EvalInt(StringRef text) {
    // TODO implement EvalInt()
    int i = 0;
    for (char c : text)
    {
        ASSERT(isdigit(c));
        i = i * 10 + (c - '0');
    }
    return i;
}

static inline int EvalChar(StringRef text) {
    // TODO implement EvalChar()
    ASSERT((text.size() == 3 && text[0] == '\'' && text[2] == '\'') ||
           (text == "'\\n'"));
    if (text.size() == 3 && text[0] == '\'' && text[2] == '\'')
        return text[1];
    else
        return '\n';
}

static inline Token RecognizeToken(const char * start,
                                   const char * end,
                                   const TokenRecog::Info & info) {
    Token token;

    token.text = StringRef(start, end - start);
    token.type = Token::UNKNOWN;

    ASSERT(info.category != TokenRecog::UNKNOWN);
    switch (info.category)
    {
        case TokenRecog::ID:
            token.type = KeywordType(token.text);
            if (token.type == Token::UNKNOWN)
                token.type = Token::ID;
            break;
        case TokenRecog::STRING:
            token.type = Token::STRING;
            break;
        case TokenRecog::OP_OR_PUNC:
            token.type = OperatorPunctuatorType(token.text);
            break;
        case TokenRecog::FLOAT:
            token.type = Token::CONST_FLOAT;
            token.fval = EvalFloat(token.text);
            break;
        case TokenRecog::INT:
            token.type = Token::CONST_INT;
            token.ival = EvalInt(token.text);
            break;
        case TokenRecog::CHAR:
            token.type = Token::CONST_CHAR;
            token.cval = EvalChar(token.text);
            break;
        case TokenRecog::PP_DIRECTIVE:
            token.type = PPDirectiveType(token.text);
        default:
            break;
    }

    return token;
}

std::vector<Token> Tokenize(std::vector<std::string> vStrLines)
{
    std::vector<Token> tokens;

    for (auto strLine : vStrLines)
    {
        Token token;
        token.type = Token::LINE_START;
        tokens.push_back(token);

        const char * start;
        const char * end = strLine.data();
        do
        {
            TokenRecog::Info info;

            // Tokenize.
            start = TokenStart(end);
            if (*start == 0)
                break;
            end = TokenEnd(start, &info);
            if (end == start)
                throw std::invalid_argument("Unrecognized token.");

            // Recognize.
            Token token = RecognizeToken(start, end, info);
            if (token.type == Token::UNKNOWN)
                throw std::invalid_argument("Unrecognized token.");
            tokens.push_back(token);
        } while (true);
    }

    Token token;
    token.type = Token::FILE_END;
    tokens.push_back(token);

    return tokens;
}

#include "DfaMatcher.h"
#include <iostream>

std::vector<Token> Tokenize(std::string & text)
{
    static std::vector<std::string> general_patterns = {
        /* ID        */ "[_a-zA-Z][_a-zA-Z0-9]*",
        /* DIRECTIVE */ "#[_a-zA-Z][_a-zA-Z0-9]*",
        /* FLOATING  */ "[0-9]+\\.[0-9]*|\\.[0-9]+",
        /* INTEGER   */ "[0-9]+",
        /* SEQUENCE  */ "'([^']|\\\\')+'|\"([^\"]|\\\\\")*\"|<([^> ]|\\\\>)+>",
        /* OPERATOR  */ "~|}|\\|\\||\\|=|\\||{|^=|^|]|\\[|\\?|>>=|>>|>=|>|==|=|<=|<<=|<<|<|;|:|/=|/|\\.\\.\\.|\\.|->|-=|--|-|,|+=|++|+|*=|*|\\)|\\(|&=|&&|&|%=|%|##|#|!=|!",
        /* SPACE     */ "[ \t\r]+",
        /* NEW LINE  */ "\n",
    };
    std::vector<DfaMatchResult> mr = Match(general_patterns, StringRef(text.data(), text.length()));

    std::vector<Token> tokens;
    for (auto r : mr)
    {
        Token t;
        t.text = StringRef(text.data() + r.offset, r.length);
        tokens.push_back(t);
    }
    return tokens;
}
