#include <vector>
#include <string>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>
#include <variant>
#include <algorithm>
#include <memory>
#include <iostream>

#include "Lexer.h"
#include "../Base/File.h"


// ==== C Lex Definition ====

static std::vector<std::string> LexPatterns = {
    "while",
    "volatile",
    "void",
    "unsigned",
    "union",
    "typedef",
    "switch",
    "struct",
    "static",
    "sizeof",
    "signed",
    "short",
    "return",
    "register",
    "long",
    "int",
    "if",
    "goto",
    "for",
    "float",
    "extern",
    "enum",
    "else",
    "double",
    "do",
    "default",
    "continue",
    "const",
    "char",
    "case",
    "break",
    "auto",
    "[_a-zA-Z][_a-zA-Z0-9]*",
    "#undef",
    "#pragma",
    "#line",
    "#include",
    "#ifndef",
    "#ifdef",
    "#if",
    "#error",
    "#endif",
    "#else",
    "#elif",
    "#define",
    "[0-9]+\\.[0-9]*|\\.[0-9]+", // float
    "[0-9]+", // int
    "'([^']|\\\\')+'", // char
    "\"([^\"]|\\\\\")*\"", // string or pp-local-path
    "<([^> ]|\\\\>)+>", // pp-env-path
    "~", "}", "\\|\\|", "\\|=", "\\|", "{", "^=", "^", "]", "\\[", "?", ">>=", ">>", ">=", ">", "==", "=", "<=", "<<=", "<<", "<", ";", ":", "/=", "/", "\\.\\.\\.", "\\.", "->", "-=", "--", "-", ",", "+=", "+\\+", "+", "*=", "*", "\\)", "\\(", "&=", "&&", "&", "%=", "%", "##", "#", "!=", "!",
    /* SPACE     */ "[ \t\r]+",
    /* NEW LINE  */ "\n",
};
static std::vector<Token::Type> LexTypes = {
    Token::KW_WHILE,
    Token::KW_VOLATILE,
    Token::KW_VOID,
    Token::KW_UNSIGNED,
    Token::KW_UNION,
    Token::KW_TYPEDEF,
    Token::KW_SWITCH,
    Token::KW_STRUCT,
    Token::KW_STATIC,
    Token::KW_SIZEOF,
    Token::KW_SIGNED,
    Token::KW_SHORT,
    Token::KW_RETURN,
    Token::KW_REGISTER,
    Token::KW_LONG,
    Token::KW_INT,
    Token::KW_IF,
    Token::KW_GOTO,
    Token::KW_FOR,
    Token::KW_FLOAT,
    Token::KW_EXTERN,
    Token::KW_ENUM,
    Token::KW_ELSE,
    Token::KW_DOUBLE,
    Token::KW_DO,
    Token::KW_DEFAULT,
    Token::KW_CONTINUE,
    Token::KW_CONST,
    Token::KW_CHAR,
    Token::KW_CASE,
    Token::KW_BREAK,
    Token::KW_AUTO,
    Token::ID,
    Token::PPD_UNDEF,
    Token::PPD_PRAGMA,
    Token::PPD_LINE,
    Token::PPD_INCLUDE,
    Token::PPD_IFNDEF,
    Token::PPD_IFDEF,
    Token::PPD_IF,
    Token::PPD_ERROR,
    Token::PPD_ENDIF,
    Token::PPD_ELSE,
    Token::PPD_ELIF,
    Token::PPD_DEFINE,
    Token::CONST_FLOAT,
    Token::CONST_INT,
    Token::CONST_CHAR,
    Token::STRING,
    Token::PP_ENV_PATH,
    Token::BIT_NOT, Token::BLK_END, Token::BOOL_OR, Token::OR_ASSIGN, Token::BIT_OR, Token::BLK_BEGIN, Token::XOR_ASSIGN, Token::BIT_XOR, Token::RSB, Token::LSB, Token::OP_QMARK, Token::SHR_ASSIGN, Token::BIT_SHR, Token::REL_GE, Token::REL_GT, Token::REL_EQ, Token::ASSIGN, Token::REL_LE, Token::SHL_ASSIGN, Token::BIT_SHL, Token::REL_LT, Token::STMT_END, Token::OP_COLON, Token::DIV_ASSIGN, Token::OP_DIV, Token::VAR_PARAM, Token::OP_DOT, Token::OP_POINTTO, Token::SUB_ASSIGN, Token::OP_DEC, Token::OP_SUB, Token::OP_COMMA, Token::ADD_ASSIGN, Token::OP_INC, Token::OP_ADD, Token::MUL_ASSIGN, Token::OP_MUL, Token::RPAREN, Token::LPAREN, Token::AND_ASSIGN, Token::BOOL_AND, Token::BIT_AND, Token::MOD_ASSIGN, Token::OP_MOD, Token::OP_TOKEN_PASTING, Token::OP_STRINGIZING, Token::REL_NE, Token::BOOL_NOT,
    Token::SPACE,
    Token::NEW_LINE,
    Token::END,
};

// ==== Error Handling ====

#define LEX_ERROR(message) do { throw std::invalid_argument(std::string("Lex error: ") + message); } while (false)
#define LEX_EXPECT_TOKEN(actual, expect_type) do { \
    if ((actual).type != (expect_type)) \
        throw std::invalid_argument(std::string("Lex error: unexpect token ") + (actual).text); \
    } while (false)
#define LEX_UNEXPECTED_TOKEN(token) do { \
        throw std::invalid_argument(std::string("Lex error: unexpect token ") + (token).text); \
    } while (false)

// ==== Comments ====

// Remove /* ... */, keep new line.
std::string RemoveComments(std::string input)
{
    std::string output;

    output.reserve(input.size());

    bool inComment = false;
    const char * in = input.data();
    const char * inEnd = input.data() + input.length();
    while (in < inEnd)
    {
        // toggle in comment state
        if (in + 1 < inEnd)
        {
            if (!inComment && *in == '/' && *(in + 1) == '*')
            {
                inComment = true;
                in += 2;
                continue;
            }
            else if (inComment && *in == '*' && *(in + 1) == '/')
            {
                inComment = false;
                in += 2;
                continue;
            }
            else if (!inComment && *in == '/' && *(in + 1) == '/')
            {
                ASSERT("C++ comment not allowed" && false);
            }
        }

        if (inComment)
        {
            // keep new line in comment
            if (*in == '\n')
            {
                output += *in++;
            }
            else
            {
                ++in;
            }
        }
        else
        {
            output += *in++;
        }
    }

    ASSERT(!inComment);

    return output;
}

// ==== Parse & Eval Token ====

Token ToToken(MatchResult mr, std::string & text)
{
    return {
        LexTypes.at(mr.which - 1), // type
        std::string(text.data() + mr.offset, mr.length) // text
    };
}
Token ParseOneToken(MatchEngine & me, std::string & text)
{
    MatchResult mr = MatchPrefix(me,
                                 StringView(text.data(), text.length()));
    if (mr.length != text.length())
        LEX_ERROR("Can't parse text into one token: " + text);
    return ToToken(mr, text);
}
std::vector<Token> ParseAllTokens(MatchEngine & me, std::string & text)
{
    // Text --dfa--> Raw Tokens (keyword, id, flt/int/char/str-const, op, punc, pp-directive, pp-path, pp-lparen, space, nl)
    std::vector<MatchResult> mrs = MatchAll(me, StringView(text.data(), text.length()));

    std::vector<Token> tokens;
    for (auto mr : mrs)
    {
        Token t = ToToken(mr, text);

        if (t.type == Token::STRING) // Fix PP_LOCAL_PATH
        {
            if (tokens.size() >= 2 &&
                tokens[tokens.size() - 1].type == Token::SPACE &&
                tokens[tokens.size() - 2].type == Token::PPD_INCLUDE)
                t.type = Token::PP_LOCAL_PATH;
        }
        else if (t.type == Token::LPAREN) // Fix PP_LPAREN
        {
            if (tokens.size() >= 3 &&
                tokens[tokens.size() - 1].type == Token::ID &&
                tokens[tokens.size() - 2].type == Token::SPACE &&
                tokens[tokens.size() - 3].type == Token::PPD_DEFINE)
                t.type = Token::PP_LPAREN;
        }

        tokens.push_back(t);
    }

    return tokens;
}

int StringToInt(const char * begin, const char * end)
{
    bool neg = *begin == '-' ? (++begin, true) : false;

    int value = 0;
    while (begin != end)
    {
        value = value * 10 - (*begin - '0');
        ++begin;
    }

    return neg ? value : -value;
}
char StringToChar(const char * begin, const char * end)
{
    if (begin[0] == '\\')
    {
        assert(begin[1] == 'n');
        return '\n';
    }
    else
    {
        assert(begin + 1 == end);
        return *begin;
    }
}
float StringToFloat(const char * begin, const char * end)
{
    int digits = 0;
    double factor = *begin == '-' ? (++begin, -1.0) : 1.0;
    bool afterDot = false;

    while (begin != end)
    {
        char c = *begin++;

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

    return static_cast<float>(digits / factor);
}
void EvalToken(Token & token)
{
    switch (token.type)
    {
        case Token::CONST_INT:
            token.ival = StringToInt(token.text.data(), token.text.data() + token.text.length());
            break;
        case Token::CONST_CHAR:
            token.cval = StringToChar(token.text.data() + 1, token.text.data() + token.text.length() - 1);
            break;
        case Token::CONST_FLOAT:
            token.fval = StringToFloat(token.text.data(), token.text.data() + token.text.length());
            break;
        default:
            break;
    }
}

std::vector<Token> & EvalTokens(std::vector<Token> & in)
{
    for (Token & token : in)
    {
        EvalToken(token);
    }
    return in;
}

// ==== Token Reader ====

// T -> std::vector<Token>

template <typename T>
class TokenReader
{
public:
    virtual std::vector<Token>      Read(T) = 0;
};

class FileTokenReader : public TokenReader<std::string>
{
public:
    FileTokenReader(MatchEngine a0)
        : matchEngine(a0)
    {}

    std::vector<Token>              Read(std::string fileName) override
    {
        return ParseAllTokens(matchEngine,
                              RemoveComments(GetFileContent(fileName.data())));
    }

private:
    MatchEngine matchEngine;
};

// ==== Helper: Token Replacer ====

struct MacroSubNode
{
    std::shared_ptr<MacroSubNode> parent;
    std::string macroName;
};
struct AnnotatedToken
{
    Token token;
    std::shared_ptr<MacroSubNode> macroSubInfo;

    operator Token () { return token; }
};

typedef std::vector<AnnotatedToken> TokenVector;
typedef std::list<AnnotatedToken>   TokenList;
typedef TokenList::iterator         TokenListIterator;
typedef TokenList::const_iterator   TokenListConstIterator;

AnnotatedToken                  Annotate(const Token & token, std::shared_ptr<MacroSubNode> m)
{
    return { token, m };
}
TokenVector                     AnnotateAll(const std::vector<Token> & tokens, std::shared_ptr<MacroSubNode> m)
{
    TokenVector atokens;
    for (const Token & token : tokens)
        atokens.emplace_back(Annotate(token, m));
    return atokens;
}
Token                           DeAnnotate(const AnnotatedToken & token)
{
    return token.token;
}
std::vector<Token>              DeAnnotateAll(TokenVector & tokens)
{
    return std::vector<Token>(tokens.begin(), tokens.end());
}

struct TokenRange
{
    bool                    Empty() const { return size == 0; }
    const AnnotatedToken &  First() const { assert(!Empty()); return *begin; }
    const AnnotatedToken &  Last() const { assert(!Empty()); auto it = end; return *(--it); }
    size_t                  Size() const { return size; }
    operator                bool() const { return size > 0; }

    TokenListConstIterator  Begin() const { return begin; }
    TokenListConstIterator  End() const { return end; }

    TokenListConstIterator begin;
    TokenListConstIterator end;
    size_t size;
};
struct TokenBuffer
{
    bool More() const { return !listToken.empty(); }
    void Remove(TokenRange range) { listToken.erase(range.begin, range.end); }

    TokenList listToken;
};
struct TokenOut
{
    enum { INVALID, VECTOR, TOKEN_BUFFER } tag;

    TokenOut()
        : tag(INVALID)
        , vecTokens(nullptr)
        , bufTokens(nullptr)
    {}
    TokenOut(TokenVector * a0)
        : tag(VECTOR)
        , vecTokens(a0)
        , bufTokens(nullptr)
    {}
    TokenOut(TokenList * a1,
             TokenListIterator a2)
        : tag(TOKEN_BUFFER)
        , vecTokens(nullptr)
        , bufTokens(a1)
        , bufInsertPos(a2)
    {}
    virtual ~TokenOut() = default;

    virtual void Insert(AnnotatedToken token)
    {
        if (tag == VECTOR)
        {
            assert(vecTokens);
            vecTokens->push_back(token);
        }
        else
        {
            assert(bufTokens);
            bufTokens->insert(bufInsertPos, token);
        }
    }

    TokenVector * vecTokens;
    TokenList * bufTokens;
    TokenListIterator bufInsertPos;
};

TokenRange EmptyRange()
{
    static TokenList empty;
    return { empty.begin(), empty.end(), 0 };
}
TokenRange FirstN(TokenRange range, size_t size)
{
    assert(size <= range.size);
    TokenListConstIterator begin, end;
    begin = end = range.begin;
    std::advance(end, size);
    return { begin, end, size };
}
TokenRange FromTokenBuffer(TokenBuffer & buffer)
{
    return { buffer.listToken.begin(), buffer.listToken.end(), buffer.listToken.size() };
}
TokenRange FromRange(TokenListConstIterator begin, TokenListConstIterator end)
{
    return { begin, end, static_cast<size_t>(std::distance(begin, end)) };
}
// [range.begin, first-token-with-type]
TokenRange SubrangeUntilIncludeType(TokenRange range, Token::Type type)
{
    TokenListConstIterator end = range.begin;
    while (end != range.end)
    {
        if (end->token.type == type)
            return FromRange(range.begin, ++end);
        ++end;
    }
    return EmptyRange();
}
TokenRange SubrangeBeforeTypes(TokenRange range, std::vector<Token::Type> types)
{
    TokenListConstIterator end = range.begin;
    while (end != range.end)
    {
        if (std::find(types.begin(), types.end(), end->token.type) != types.end())
            return FromRange(range.begin, end);
        ++end;
    }
    return EmptyRange();
}
TokenBuffer FromVector(TokenVector & tokens)
{
    TokenBuffer buffer;
    buffer.listToken.insert(buffer.listToken.end(),
                            tokens.begin(),
                            tokens.end());
    return buffer;
}
TokenOut Inserter(TokenVector & out)
{
    return { &out };
}
TokenOut Inserter(TokenBuffer & buffer, TokenListConstIterator itConst)
{
    TokenListIterator it = buffer.listToken.begin();
    std::advance(it, std::distance(buffer.listToken.cbegin(), itConst));
    return { &buffer.listToken, it };
}

// ==== Helper: Macro Token Replacer ====

enum class PPType
{
    UNKNOWN,
    EMIT,
    DEF_MACRO_OBJ,
    DEF_MACRO_FUN,
    MACRO_OBJ_SUB,
    MACRO_FUN_SUB,
    COND_INCL,
    FILE_INCL,
};
struct PPMacroSubstitution
{
    enum { OBJECT, FUNCTION } tag;
    struct
    {
        std::vector<Token> replaceTokens;
    } object;
    struct
    {
        size_t paramCount;
        std::vector<std::variant<Token, int>> replaceTokens;
    } function;

    PPMacroSubstitution(std::vector<Token> & replaceTokens)
        : tag(OBJECT), object{ replaceTokens } {}
    PPMacroSubstitution(size_t paramCount, std::vector<std::variant<Token, int>> & replaceTokens)
        : tag(FUNCTION), function{ paramCount, replaceTokens } {}
};
struct PPContext
{
    // macro object/func
    std::map<std::string, PPMacroSubstitution> macroSubs;

    bool IsMacroDefined(std::string macroName)
    {
        return macroSubs.find(macroName) != macroSubs.end();
    }
};
PPType DecidePPType(PPContext & context, Token * tokens)
{
    if (tokens[0].type == Token::PPD_DEFINE)
    {
        if (tokens[1].type == Token::ID && tokens[2].type != Token::UNKNOWN)
        {
            return tokens[2].type == Token::PP_LPAREN
                ? PPType::DEF_MACRO_FUN
                : PPType::DEF_MACRO_OBJ;
        }
    }

    if (tokens[0].type == Token::ID)
    {
        auto it = context.macroSubs.find(tokens[0].text);
        if (it != context.macroSubs.end())
        {
            return it->second.tag == PPMacroSubstitution::FUNCTION
                ? PPType::MACRO_FUN_SUB
                : PPType::MACRO_OBJ_SUB;
        }
    }

    if (tokens[0].type == Token::PPD_IF ||
        tokens[0].type == Token::PPD_IFDEF ||
        tokens[0].type == Token::PPD_IFNDEF)
    {
        return PPType::COND_INCL;
    }

    if (tokens[0].type == Token::PPD_INCLUDE)
        return PPType::FILE_INCL;

    return tokens[0].type != Token::UNKNOWN
        ? PPType::EMIT
        : PPType::UNKNOWN;
}

// ==== Token Replacer ====

class TokenReplacer
{
public:
    virtual TokenRange              Accept(TokenRange) = 0;
    virtual void                    Replace(TokenRange, TokenOut &) = 0;
};
class EmitTokenReplacer : public TokenReplacer
{
public:
    EmitTokenReplacer(TokenOut & out)
        : emitter(out)
    {}

    TokenRange                      Accept(TokenRange range) override
    {
        return FirstN(range, 1);
    }
    void                            Replace(TokenRange range, TokenOut & _) override
    {
        assert(range.Size() == 1);
        emitter.Insert(range.First());
    }

private:
    TokenOut & emitter;
};
class MacroContextTokenReplacer : public TokenReplacer
{
    // Handle macro definition (#define...)
public:
    TokenRange                      Accept(TokenRange range) override
    {
        TokenListConstIterator it = range.Begin();
        if (it->token.type == Token::PPD_DEFINE)
        {
            ++it;
            LEX_EXPECT_TOKEN(it->token, Token::ID);
            ++it;
            macroType = it->token.type == Token::PP_LPAREN
                ? PPType::DEF_MACRO_FUN
                : PPType::DEF_MACRO_OBJ;

            return SubrangeUntilIncludeType(range, Token::NEW_LINE);
        }
        else
        {
            return EmptyRange();
        }
    }
    void                            Replace(TokenRange range, TokenOut & _) override
    {
        TokenListConstIterator begin = range.Begin();
        TokenListConstIterator end = range.End();

        if (macroType == PPType::DEF_MACRO_OBJ)
        {
            ++begin; // skip #define
            --end; // ignore new-line

            LEX_EXPECT_TOKEN(begin->token, Token::ID);
            std::string id = begin->token.text;
            ++begin;

            if (macroContext.macroSubs.find(id) != macroContext.macroSubs.end())
                LEX_ERROR("Duplicate macro definition: " + id);
            TokenVector vecTokens(begin, end);
            macroContext.macroSubs.emplace(id, DeAnnotateAll(vecTokens));
        }
        else
        {
            ++begin; // skip #define
            --end; // ignore new-line

            LEX_EXPECT_TOKEN(begin->token, Token::ID);
            std::string id = begin->token.text;
            ++begin;

            LEX_EXPECT_TOKEN(begin->token, Token::PP_LPAREN);
            ++begin;

            // parse param-list
            std::vector<std::string> paramIds;
            while (begin->token.type != Token::RPAREN)
            {
                for (std::string & paramId : paramIds)
                {
                    if (paramId == begin->token.text)
                        LEX_ERROR("Illegal macro function definition: duplicate parameter name '" + paramId + "'.");
                }

                LEX_EXPECT_TOKEN(begin->token, Token::ID);
                paramIds.emplace_back(begin->token.text);
                ++begin;

                if (begin->token.type != Token::RPAREN)
                {
                    LEX_EXPECT_TOKEN(begin->token, Token::OP_COMMA);
                    ++begin;
                }
            }
            ++begin; // skip ')'
            if (paramIds.empty())
                LEX_ERROR("Illegal macro function definition: must has at least one parameter.");

            // put param-placeholder in body
            std::vector<std::variant<Token, int>> vTokenOrParam;
            while (begin != end)
            {
                auto paramIdIt = std::find(paramIds.begin(), paramIds.end(), begin->token.text);
                if (begin->token.type == Token::ID && paramIdIt != paramIds.end())
                    vTokenOrParam.emplace_back(static_cast<int>(std::distance(paramIds.begin(), paramIdIt)));
                else
                    vTokenOrParam.emplace_back(begin->token);
                ++begin;
            }

            if (macroContext.macroSubs.find(id) != macroContext.macroSubs.end())
                LEX_ERROR("Duplicate macro definition: " + id);
            macroContext.macroSubs.emplace(id, PPMacroSubstitution(paramIds.size(), vTokenOrParam));
        }
    }

    PPContext &                     GetMacroContext()
    {
        return macroContext;
    }

private:
    PPContext macroContext;
    PPType macroType;
};
class MacroSubTokenReplacer : public TokenReplacer
{
    // Apply macro object substitution
    // Apply macro function substitution
public:
    MacroSubTokenReplacer(PPContext & a0,
                          MatchEngine a1)
        : macroContext(a0)
        , matchEngine(a1)
    {}

    TokenRange                      Accept(TokenRange range) override
    {
        if (range.First().token.type != Token::ID)
            return EmptyRange();

        // Avoid recursive expansion.
        {
            TokenListConstIterator begin = range.Begin();
            TokenListConstIterator end = range.End();

            std::string macroName = begin->token.text;

            bool hasRecursion = false;
            std::shared_ptr<MacroSubNode> disableChain = begin->macroSubInfo;
            while (disableChain)
            {
                if (disableChain->macroName == macroName)
                {
                    hasRecursion = true;
                    break;
                }
                else
                    disableChain = disableChain->parent;
            }
            if (hasRecursion)
                return EmptyRange();
        }

        auto it = macroContext.macroSubs.find(range.First().token.text);
        if (it == macroContext.macroSubs.end())
            return EmptyRange();

        macroType =
            it->second.tag == PPMacroSubstitution::FUNCTION
            ? PPType::MACRO_FUN_SUB
            : PPType::MACRO_OBJ_SUB;

        TokenListConstIterator end = range.Begin();
        if (macroType == PPType::MACRO_OBJ_SUB)
        {
            ++end;
        }
        else
        {
            ++end; // function name

            LEX_EXPECT_TOKEN(end->token, Token::LPAREN);
            ++end;

            for (int paren = 1; paren > 0;)
            {
                if (end->token.type == Token::LPAREN)
                    ++paren;
                else if (end->token.type == Token::RPAREN)
                    --paren;
                ++end;
            }
        }

        return FromRange(range.Begin(), end);
    }
    void                            Replace(TokenRange range, TokenOut & out) override
    {
        TokenListConstIterator begin = range.Begin();
        TokenListConstIterator end = range.End();

        std::string macroName = begin->token.text;
        ++begin;

        auto annotation = std::make_shared<MacroSubNode>();
        annotation->parent = begin->macroSubInfo;
        annotation->macroName = macroName;

        if (macroType == PPType::MACRO_OBJ_SUB)
        {
            // Expand macro object

            auto & objSub = macroContext.macroSubs.find(macroName)->second.object;

            for (auto & token : objSub.replaceTokens)
            {
                out.Insert(Annotate(token, annotation));
            }
        }
        else if (macroType == PPType::MACRO_FUN_SUB)
        {
            // Expand macro function
            auto & funcSub = macroContext.macroSubs.find(macroName)->second.function;

            LEX_EXPECT_TOKEN(begin->token, Token::LPAREN);
            ++begin;

            // collect argment tokens
            //  arglist := expr (',' expr)*
            //  expr := ([^,]+|\([^)]*\))
            std::vector<TokenVector> argReplaceTokens;
            TokenVector replaceTokens;
            while (true)
            {
                int paren = 0;
                while (paren > 0 ||
                    (begin->token.type != Token::RPAREN && begin->token.type != Token::OP_COMMA))
                {
                    if (begin->token.type == Token::LPAREN) ++paren;
                    else if (begin->token.type == Token::RPAREN) --paren;
                    replaceTokens.push_back(*begin);
                    ++begin;
                    assert(begin != end);
                }

                if (replaceTokens.empty())
                    LEX_ERROR("Illegal macro function call: unexpected comma.");
                argReplaceTokens.push_back(replaceTokens);
                replaceTokens.clear();

                if (begin->token.type == Token::RPAREN)
                    break;
                else
                {
                    assert(begin->token.type == Token::OP_COMMA);
                    ++begin;
                    assert(begin != end);
                }
            }

            LEX_EXPECT_TOKEN(begin->token, Token::RPAREN);
            ++begin;

            if (argReplaceTokens.size() != funcSub.paramCount)
                LEX_ERROR("Illegal macro function call: mismatched argument count.");

            // apply macro function
            //  1. handle #
            //  2. handle argument substitution
            //  3. handle ##
            //  * avoid recursive expanison
            std::vector<std::variant<AnnotatedToken, int>> rt;
            std::vector<std::variant<AnnotatedToken, int>> rt2;

            for (auto & v : funcSub.replaceTokens)
            {
                std::variant<AnnotatedToken, int> vv;
                if (v.index() == 0)
                    vv = Annotate(std::get<0>(v), annotation);
                else
                    vv = std::get<1>(v);
                rt.emplace_back(vv);
            }

            // stringizing operator (#)
            for (auto curr = rt.begin(); curr != rt.end(); ++curr)
            {
                if (curr->index() == 0 && std::get<0>(*curr).token.type == Token::OP_STRINGIZING)
                {
                    ++curr; // skip #
                    assert(curr != rt.end());
                    assert(curr->index() == 1);

                    int argIndex = std::get<1>(*curr);

                    std::string * leakyText = new std::string("\"");

                    std::for_each(argReplaceTokens.at(argIndex).begin(),
                                  argReplaceTokens.at(argIndex).end(),
                                  [leakyText](AnnotatedToken & token) { leakyText->append(token.token.text.data(), token.token.text.size()); leakyText->push_back(' '); });
                    leakyText->pop_back();
                    leakyText->append("\"");

                    Token t;
                    t.text = *leakyText;
                    t.type = Token::STRING;
                    rt2.emplace_back(Annotate(t, annotation));
                }
                else
                {
                    rt2.emplace_back(*curr);
                }
            }
            rt.clear();
            std::swap(rt, rt2);
            // argument substitution
            for (auto curr = rt.begin(); curr != rt.end(); ++curr)
            {
                if (curr->index() == 1)
                {
                    int argIndex = std::get<1>(*curr);

                    rt2.insert(rt2.end(),
                               argReplaceTokens.at(argIndex).begin(),
                               argReplaceTokens.at(argIndex).end());
                }
                else
                {
                    rt2.emplace_back(*curr);
                }
            }
            rt.clear();
            std::swap(rt, rt2);
            // token-pasting operator (##)
            assert(rt.front().index() == 1 || std::get<0>(rt.front()).token.type != Token::OP_TOKEN_PASTING);
            assert(rt.back().index() == 1 || std::get<0>(rt.back()).token.type != Token::OP_TOKEN_PASTING);
            size_t l = 0;
            for (size_t m = 1, r = 2;
                 r < rt.size();
                 ++l, ++m, ++r)
            {
                if (rt[m].index() == 0 && std::get<0>(rt[m]).token.type == Token::OP_TOKEN_PASTING)
                {
                    // merge, re-pase, insert
                    std::string * leakyMergedText = new std::string(std::get<0>(rt[l]).token.text + std::get<0>(rt[r]).token.text);
                    Token mergedToken = ParseOneToken(matchEngine, *leakyMergedText);
                    rt[r] = Annotate(mergedToken, {});
                    // remove 3, insert 1 -> add 2
                    ++l, ++m, ++r;
                }
                else
                {
                    rt2.emplace_back(rt[l]);
                }
            }
            while (l < rt.size())
            {
                rt2.emplace_back(rt[l]);
                ++l;
            }

            for (auto it = rt2.begin(); it != rt2.end(); ++it)
            {
                AnnotatedToken token = { std::get<AnnotatedToken>(*it).token, annotation };
                out.Insert(token);
            }
        }
    }

private:
    PPContext & macroContext;
    PPType macroType;
    MatchEngine matchEngine;
};
class ConditionalIncludeTokenReplacer : public TokenReplacer
{
    // Handle #if...#elif...#endif, #ifdef...#endif, #ifndef...#endif
public:
    ConditionalIncludeTokenReplacer(PPContext & a0)
        : macroContext(a0)
    {}

    TokenRange                      Accept(TokenRange range) override
    {
        TokenRange acceptedRange;
        switch (range.First().token.type)
        {
            case Token::PPD_IF:
            case Token::PPD_IFDEF:
            case Token::PPD_IFNDEF:
                acceptedRange = SubrangeUntilIncludeType(range, Token::PPD_ENDIF);
                ++acceptedRange.end;
                LEX_EXPECT_TOKEN(acceptedRange.Last().token, Token::NEW_LINE);
                break;
            default:
                acceptedRange = EmptyRange();
                break;
        }
        return acceptedRange;
    }
    void                            Replace(TokenRange range, TokenOut & out) override
    {
        TokenRange expr;
        TokenRange body;
        TokenRange rest;

        body = EmptyRange();
        rest = range;

        while (!rest.Empty() && rest.First().token.type != Token::PPD_ENDIF)
        {
            expr = SubrangeUntilIncludeType(rest, Token::NEW_LINE);
            body = SubrangeBeforeTypes(
                FromRange(expr.End(), rest.End()),
                { Token::PPD_ELIF,Token::PPD_ELSE, Token::PPD_ENDIF, }
            );
            rest = FromRange(body.End(), rest.End());

            if (EvalPPExpr(expr))
                break;
            else
                body = EmptyRange();
        }

        for (auto it = body.Begin(); it != body.End(); ++it)
        {
            out.Insert(*it);
        }
    }

private:
    bool EvalPPExpr(TokenRange range)
    {
        /*
            pp-expr     := '#if' '!'? 'defined' ID NL
                        || '#elif' '!'? 'defined' ID NL
                        || '#else' NL
                        || '#ifdef' ID NL
                        || '#ifndef' ID NL
        */
        bool isTrue = false;

        bool neg = false;
        auto it = range.Begin();
        switch (it->token.type)
        {
            case Token::PPD_IF:
            case Token::PPD_ELIF:
                ++it;
                if (it->token.type == Token::BOOL_NOT)
                    neg = true, ++it;
                if (it->token.type != Token::ID || it->token.text != "defined")
                    LEX_ERROR("Expect 'defined'.");
                ++it;
                LEX_EXPECT_TOKEN(it->token, Token::ID);
                isTrue = macroContext.IsMacroDefined(it->token.text);
                if (neg) isTrue = !isTrue;
                ++it;
                break;
            case Token::PPD_ELSE:
                isTrue = true;
                ++it;
                break;
            case Token::PPD_IFNDEF:
                neg = true;
            case Token::PPD_IFDEF:
                ++it;
                LEX_EXPECT_TOKEN(it->token, Token::ID);
                isTrue = macroContext.IsMacroDefined(it->token.text);
                if (neg) isTrue = !isTrue;
                ++it;
                break;
            default:
                LEX_UNEXPECTED_TOKEN(it->token);
                break;
        }

        LEX_EXPECT_TOKEN(it->token, Token::NEW_LINE);

        return isTrue;
    }

    PPContext & macroContext;
};
class FileIncludeTokenReplacer : public TokenReplacer
{
    // Handle #include...
public:
    FileIncludeTokenReplacer(TokenReader<std::string> * a0, std::string a1)
        : trFile(a0)
        , baseDir(a1)
    {}

    TokenRange                      Accept(TokenRange range) override
    {
        auto end = range.Begin();
        if (end->token.type == Token::PPD_INCLUDE)
        {
            ++end;
            LEX_EXPECT_TOKEN(end->token, Token::PP_LOCAL_PATH); // PP_ENV_PATH
            ++end;
            LEX_EXPECT_TOKEN(end->token, Token::NEW_LINE);
            ++end;
            return FromRange(range.Begin(), end);
        }
        else
        {
            return EmptyRange();
        }
    }
    void                            Replace(TokenRange range, TokenOut & out) override
    {
        auto it = range.Begin();
        ++it;
        std::string fileName = it->token.text.substr(1, it->token.text.size() - 2);
        for (Token & token : trFile->Read(baseDir + "\\" + fileName))
        {
            out.Insert(Annotate(token, {}));
        }
    }

private:
    TokenReader<std::string> * trFile;
    std::string baseDir;
};
class ChainedTokenReplacer : public TokenReplacer
{
public:
    ChainedTokenReplacer(std::vector<TokenReplacer *> a0)
        : tokenReplacerList(a0)
    {}

    TokenRange                      Accept(TokenRange range) override
    {
        size_t      acceptedIndex = 0;
        TokenRange  acceptedRange;
        for (TokenReplacer * tr : tokenReplacerList)
        {
            acceptedRange = tr->Accept(range);
            if (acceptedRange) break;
            else ++acceptedIndex;
        }

        acceptedTokenReplacerIndex = acceptedIndex;

        return acceptedRange;
    }
    void                            Replace(TokenRange range, TokenOut & inserter) override
    {
        assert(acceptedTokenReplacerIndex < tokenReplacerList.size());
        tokenReplacerList[acceptedTokenReplacerIndex]->Replace(range, inserter);
    }

private:
    std::vector<TokenReplacer *> tokenReplacerList;
    size_t acceptedTokenReplacerIndex;
};

// ==== Token Filter ====

// Token -> bool

class TokenFilter
{
public:
    virtual bool                    Filter(const Token &) = 0;
};
class TypeTokenFilter : public TokenFilter
{
public:
    TypeTokenFilter(Token::Type type)
        : filteredType(type)
    {}

    bool                            Filter(const Token & token)
    {
        return token.type == filteredType;
    }

private:
    Token::Type filteredType;
};

class FilteredTokenReader : public TokenReader<std::string>
{
public:
    FilteredTokenReader(TokenReader<std::string> * a0,
                      TokenFilter * a1)
        : tokenReader(a0)
        , tokenFilter(a1)
    {}

    std::vector<Token>              Read(std::string source) override
    {
        std::vector<Token> tokens1 = tokenReader->Read(source);
        std::vector<Token> tokens2;

        std::copy_if(tokens1.begin(),
                     tokens1.end(),

                     std::back_inserter(tokens2),

                     [this](Token & token) ->bool
                     {
                         return !tokenFilter->Filter(token);
                     });

        return tokens2;
    }

private:
    TokenReader<std::string> * tokenReader;
    TokenFilter * tokenFilter;
};
struct FilteredTokenOut : public TokenOut
{
    TokenFilter * filter;
    TokenOut & out;

    FilteredTokenOut(TokenOut & a0, TokenFilter * a1)
        : out(a0)
        , filter(a1)
    {}

    void Insert(AnnotatedToken token) override
    {
        if (!filter->Filter(token))
            out.Insert(token);
    }
};

// ==== Print ====

struct PrintTokenOut : public TokenOut
{
    std::string * printDest;
    TokenOut & out;

    PrintTokenOut(TokenOut & a0, std::string * a1)
        : out(a0)
        , printDest(a1)
    {
        if (printDest)
            printDest->reserve(4096);
    }

    void Insert(AnnotatedToken token) override
    {
        if (printDest)
        {
            *printDest += token.token.text;
            printDest->push_back(' ');
        }
        out.Insert(token);
    }
};

// ==== Batch ====

template <typename T>
std::vector<Token> ReadAllTokens(T input, TokenReader<T> & reader)
{
    return reader.Read(input);
}
std::vector<Token> FilterAllTokens(const std::vector<Token> & input, TokenFilter & filter)
{
    std::vector<Token> output;

    std::copy_if(input.begin(),
                 input.end(),
                 std::back_inserter(output),
                 [&filter] (const Token & token) -> bool { return !filter.Filter(token); });
    
    return output;
}
std::vector<Token> ReplaceAllTokens(const std::vector<Token> & input, TokenReplacer & replacer)
{
    std::vector<Token> output;

    TokenBuffer in = FromVector(AnnotateAll(input, {}));
    while (in.More())
    {
        TokenRange all = FromTokenBuffer(in);
        TokenRange accept = replacer.Accept(all);
        if (accept.Empty())
        {
            output.emplace_back(DeAnnotate(all.First()));
            in.Remove(FirstN(all, 1));
        }
        else
        {
            TokenVector tmp;
            // Ideally should use Inserter(in, accept.End()), but accept.End() moves with Inserter.
            replacer.Replace(accept, Inserter(tmp));
            in.Remove(accept);
            in.listToken.insert(in.listToken.begin(), tmp.begin(), tmp.end());
        }
    }

    return output;
}

// ==== API ====

std::vector<Token> LexProcess(std::string sourceFile, std::string * sourceAfterPreproc)
{
    TokenVector output;

    MatchEngine                         me = Compile(LexPatterns);

    TokenFilter *                       tfSpace = new TypeTokenFilter({ Token::SPACE });
    TokenFilter *                       tfNewLine = new TypeTokenFilter({ Token::NEW_LINE });

    TokenReader<std::string> *          trFile = new FilteredTokenReader(new FileTokenReader(me), tfSpace);

    TokenOut                            e0 = Inserter(output);
    FilteredTokenOut                    e1(e0, tfNewLine);
    PrintTokenOut                       emitter(e1, sourceAfterPreproc);

    EmitTokenReplacer *                 repEmit = new EmitTokenReplacer(emitter);
    MacroContextTokenReplacer *         repMacroContext = new MacroContextTokenReplacer;
    MacroSubTokenReplacer *             repMacroSub = new MacroSubTokenReplacer(repMacroContext->GetMacroContext(), me);
    ConditionalIncludeTokenReplacer *   repCondIncl = new ConditionalIncludeTokenReplacer(repMacroContext->GetMacroContext());
    FileIncludeTokenReplacer *          repFileIncl = new FileIncludeTokenReplacer(trFile, GetCanonicalFileDirectory(sourceFile));

    ChainedTokenReplacer                repPreproc({ repMacroContext, repMacroSub, repCondIncl, repFileIncl, repEmit });

    TokenVector tokens = AnnotateAll(trFile->Read(sourceFile), {});
    TokenBuffer in = FromVector(tokens);
    while (in.More())
    {
        TokenRange all = FromTokenBuffer(in);
        TokenRange accept = repPreproc.Accept(all);
        if (accept.Empty())
            LEX_ERROR("Unexpected token.");
        TokenVector tmp;
        // Ideally should use Inserter(in, accept.End()), but accept.End() moves with Inserter.
        repPreproc.Replace(accept, Inserter(tmp));
        in.Remove(accept);
        in.listToken.insert(in.listToken.begin(), tmp.begin(), tmp.end());
    }

    return EvalTokens(DeAnnotateAll(output));
}

TokenIterator::TokenIterator(std::vector<Token> & tokens)
    : tokens_(tokens)
    , i_(0) {
}

bool TokenIterator::has() const {
    return i_ < tokens_.size();
}

Token TokenIterator::next() {
    ASSERT(i_ < tokens_.size());
    return tokens_[i_++];
}

Token TokenIterator::peek() const {
    ASSERT(i_ < tokens_.size());
    return tokens_[i_];
}

Token TokenIterator::peekN(int n) const {
    ASSERT(i_ + n < tokens_.size());
    return tokens_[i_ + n];
}

#ifdef UNIT_TEST
#include "../UnitTest/UnitTest.h"

class LexerTest : public Tester
{
protected:
    void BeforeAllTestCases() override
    {
        lexMatcher = Compile(LexPatterns);
    }
    MatchEngine & GetMatcher()
    {
        return lexMatcher;
    }
private:
    static MatchEngine lexMatcher;
};
MatchEngine LexerTest::lexMatcher;

// string -> Token[]
TEST_F(LexerTest, ParseToken)
{
    std::string input =
        "while volatile void unsigned union typedef switch struct static sizeof signed short return register long int if goto for float extern enum else double do default continue const char case break auto "
        "_this_iS_A_L0ng_Id "
        "#undef #pragma #line "
        "#include \"src.c\" "
        "#ifndef #ifdef #if #error #endif #else #elif "
        "#define MACRO_FUNC( "
        "0.0 123 'a' \"hello\" <file.h> "
        "~ } || |= | { ^= ^ ] [ ? >>= >> >= > == = <= <<= << < ; : /= / ... . -> -= -- - , += ++ + *= * ) ( &= && & %= % ## # != ! \n"
        ;
    std::vector<Token> output = {
        { Token::KW_WHILE, "while" },
    { Token::KW_VOLATILE, "volatile" },
    { Token::KW_VOID, "void" },
    { Token::KW_UNSIGNED, "unsigned" },
    { Token::KW_UNION, "union" },
    { Token::KW_TYPEDEF, "typedef" },
    { Token::KW_SWITCH, "switch" },
    { Token::KW_STRUCT, "struct" },
    { Token::KW_STATIC, "static" },
    { Token::KW_SIZEOF, "sizeof" },
    { Token::KW_SIGNED, "signed" },
    { Token::KW_SHORT, "short" },
    { Token::KW_RETURN, "return" },
    { Token::KW_REGISTER, "register" },
    { Token::KW_LONG, "long" },
    { Token::KW_INT, "int" },
    { Token::KW_IF, "if" },
    { Token::KW_GOTO, "goto" },
    { Token::KW_FOR, "for" },
    { Token::KW_FLOAT, "float" },
    { Token::KW_EXTERN, "extern" },
    { Token::KW_ENUM, "enum" },
    { Token::KW_ELSE, "else" },
    { Token::KW_DOUBLE, "double" },
    { Token::KW_DO, "do" },
    { Token::KW_DEFAULT, "default" },
    { Token::KW_CONTINUE, "continue" },
    { Token::KW_CONST, "const" },
    { Token::KW_CHAR, "char" },
    { Token::KW_CASE, "case" },
    { Token::KW_BREAK, "break" },
    { Token::KW_AUTO, "auto" },
    { Token::ID, "_this_iS_A_L0ng_Id" },
    { Token::PPD_UNDEF, "#undef" },
    { Token::PPD_PRAGMA, "#pragma" },
    { Token::PPD_LINE, "#line" },
    { Token::PPD_INCLUDE, "#include" },
    { Token::PP_LOCAL_PATH, "\"src.c\"" },
    { Token::PPD_IFNDEF, "#ifndef" },
    { Token::PPD_IFDEF, "#ifdef" },
    { Token::PPD_IF, "#if" },
    { Token::PPD_ERROR, "#error" },
    { Token::PPD_ENDIF, "#endif" },
    { Token::PPD_ELSE, "#else" },
    { Token::PPD_ELIF, "#elif" },
    { Token::PPD_DEFINE, "#define" },
    { Token::ID, "MACRO_FUNC" },{ Token::PP_LPAREN, "(" },
    { Token::CONST_FLOAT, "0.0" },
    { Token::CONST_INT, "123" },
    { Token::CONST_CHAR, "'a'" },
    { Token::STRING, "\"hello\"" },
    { Token::PP_ENV_PATH, "<file.h>" },
    { Token::BIT_NOT, "~" },
    { Token::BLK_END, "}" },
    { Token::BOOL_OR, "||" },
    { Token::OR_ASSIGN, "|=" },
    { Token::BIT_OR, "|" },
    { Token::BLK_BEGIN, "{" },
    { Token::XOR_ASSIGN, "^=" },
    { Token::BIT_XOR, "^" },
    { Token::RSB, "]" },
    { Token::LSB, "[" },
    { Token::OP_QMARK, "?" },
    { Token::SHR_ASSIGN, ">>=" },
    { Token::BIT_SHR, ">>" },
    { Token::REL_GE, ">=" },
    { Token::REL_GT, ">" },
    { Token::REL_EQ, "==" },
    { Token::ASSIGN, "=" },
    { Token::REL_LE, "<=" },
    { Token::SHL_ASSIGN, "<<=" },
    { Token::BIT_SHL, "<<" },
    { Token::REL_LT, "<" },
    { Token::STMT_END, ";" },
    { Token::OP_COLON, ":" },
    { Token::DIV_ASSIGN, "/=" },
    { Token::OP_DIV, "/" },
    { Token::VAR_PARAM, "..." },
    { Token::OP_DOT, "." },
    { Token::OP_POINTTO, "->" },
    { Token::SUB_ASSIGN, "-=" },
    { Token::OP_DEC, "--" },
    { Token::OP_SUB, "-" },
    { Token::OP_COMMA, "," },
    { Token::ADD_ASSIGN, "+=" },
    { Token::OP_INC, "++" },
    { Token::OP_ADD, "+" },
    { Token::MUL_ASSIGN, "*=" },
    { Token::OP_MUL, "*" },
    { Token::RPAREN, ")" },
    { Token::LPAREN, "(" },
    { Token::AND_ASSIGN, "&=" },
    { Token::BOOL_AND, "&&" },
    { Token::BIT_AND, "&" },
    { Token::MOD_ASSIGN, "%=" },
    { Token::OP_MOD, "%" },
    { Token::OP_TOKEN_PASTING, "##" },
    { Token::OP_STRINGIZING, "#" },
    { Token::REL_NE, "!=" },
    { Token::BOOL_NOT, "!" },
    { Token::NEW_LINE, "\n" },
    };
    try
    {
        std::vector<Token> tokens = FilterAllTokens(ParseAllTokens(GetMatcher(), input), TypeTokenFilter(Token::SPACE));
        EXPECT_EQ(tokens.size(), output.size());
        for (size_t i = 0; i < output.size(); ++i)
        {
            EXPECT_EQ(tokens[i].type, output[i].type);
            EXPECT_EQ(tokens[i].text, output[i].text);
            if (tokens[i].type != output[i].type && tokens[i].text == output[i].text)
                std::cerr << "Text: \"" << tokens[i].text << "\"" << std::endl;
            if (tokens[i].type == output[i].type && tokens[i].text != output[i].text)
                std::cerr << "Type: " << tokens[i].type << std::endl;
        }
    }
    catch (const std::exception & e)
    {
        std::cerr << "Caught exception: " << e.what() << std::endl;
        EXPECT_NOT_REACH;
    }
}

// Token[] -> Token[]
TEST_F(LexerTest, EvalToken)
{
    std::string input =
        "0 -1 1 2147483647 -2147483648 "
        "0.0 1.0 2.0 -1.0 -2.0 "
        "'c' '\\n' "
        ;
    std::vector<std::pair<Token::Type, std::variant<int, float, char>>> output = {
        { Token::CONST_INT, 0 },
    { Token::CONST_INT, -1 },
    { Token::CONST_INT, 1 },
    { Token::CONST_INT, 2147483647 },
    { Token::CONST_INT, -2147483647 - 1 },
    { Token::CONST_FLOAT, 0.0f },
    { Token::CONST_FLOAT, 1.0f },
    { Token::CONST_FLOAT, 2.0f },
    { Token::CONST_FLOAT, -1.0f },
    { Token::CONST_FLOAT, -2.0f },
    { Token::CONST_CHAR, 'c' },
    { Token::CONST_CHAR, '\n' },
    };
    try
    {
        std::vector<Token> tokens = FilterAllTokens(ParseAllTokens(GetMatcher(), input), TypeTokenFilter(Token::SPACE));
        EvalTokens(tokens);

        EXPECT_EQ(tokens.size(), output.size());
        for (size_t i = 0; i < output.size(); ++i)
        {
            EXPECT_EQ(tokens[i].type, output[i].first);
            if (tokens[i].type == output[i].first)
            {
                switch (tokens[i].type)
                {
                    case Token::CONST_INT:      EXPECT_EQ(tokens[i].ival, std::get<int>(output[i].second)); break;
                    case Token::CONST_CHAR:     EXPECT_EQ(tokens[i].cval, std::get<char>(output[i].second)); break;
                    case Token::CONST_FLOAT:    EXPECT_EQ(tokens[i].fval, std::get<float>(output[i].second)); break;
                    default: break;
                }
            }
        }
    }
    catch (const std::exception & e)
    {
        std::cerr << "Caught exception: " << e.what() << std::endl;
        EXPECT_NOT_REACH;
    }
}

// Token[] -> Token[]
TEST_F(LexerTest, MacroSubstitution)
{
    try
    {
        std::string input =
            "#define I 5                 \n"
            "#define ADD(a, b) (a + b)   \n"
            "#define STR(x) #x           \n"
            "#define CONCAT(x, y) x##y   \n"
            "#define FOO(x) BAR(x)BAR(x) \n"
            "#define BAR(x) FOO(x)       \n"
            "I                           \n"
            "ADD(2, 3)                   \n"
            "ADD(I, I)                   \n"
            "STR(apple)                  \n"
            "CONCAT(ye, ah)              \n"
            "FOO(a)                      \n"
            "BAR(a)                      \n"
            ;
        std::string output =
            "5                           \n"
            "(2 + 3)                     \n"
            "(5 + 5)                     \n"
            "\"apple\"                   \n"
            "yeah                        \n"
            "FOO(a) FOO(a)               \n"
            "BAR(a) BAR(a)               \n"
            ;

        std::vector<Token>          expectTokens = FilterAllTokens(ParseAllTokens(GetMatcher(), output),
                                                                   TypeTokenFilter(Token::SPACE));

        MacroContextTokenReplacer   repMacroContext ;
        MacroSubTokenReplacer       repMacroSub(repMacroContext.GetMacroContext(), GetMatcher());
        ChainedTokenReplacer        rep({ &repMacroContext, &repMacroSub });
        std::vector<Token>          actualTokens = ReplaceAllTokens(FilterAllTokens(ParseAllTokens(GetMatcher(), input),
                                                                                    TypeTokenFilter(Token::SPACE)),
                                                                    rep);
        
        EXPECT_EQ(actualTokens.size(), expectTokens.size());
        for (size_t i = 0; i < expectTokens.size(); ++i)
        {
            EXPECT_EQ(actualTokens[i].type, expectTokens[i].type);
            EXPECT_EQ(actualTokens[i].text, expectTokens[i].text);
            if (actualTokens[i].type != expectTokens[i].type && actualTokens[i].text == expectTokens[i].text)
                std::cerr << "Text: \"" << actualTokens[i].text << "\"" << std::endl;
            if (actualTokens[i].type == expectTokens[i].type && actualTokens[i].text != expectTokens[i].text)
                std::cerr << "Type: " << actualTokens[i].type << std::endl;
        }
    }
    catch (const std::exception & e)
    {
        std::cerr << "Caught exception: " << e.what() << std::endl;
        EXPECT_NOT_REACH;
    }
}

#endif
