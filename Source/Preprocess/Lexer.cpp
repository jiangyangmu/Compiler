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

#include "../Base/String.h"

#include "DfaMatcher.h"

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
        //OP_DOT, OP_POINTTO,
        //// condition operator
        //OP_QMARK, OP_COLON,
        //// comma operator
        //OP_COMMA,
        //// assignment operator
        //ASSIGN, ADD_ASSIGN, SUB_ASSIGN, MUL_ASSIGN, DIV_ASSIGN, MOD_ASSIGN,
        //SHL_ASSIGN, SHR_ASSIGN, AND_ASSIGN, OR_ASSIGN, XOR_ASSIGN,
        //OP_INC, OP_DEC,
        //// arithmetic operator
        //// *: dereference + multiplier
        //OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD,
        //// bool operator
        //BOOL_AND, BOOL_OR, BOOL_NOT,
        //// relation operator
        //REL_EQ, REL_NE, REL_GT, REL_GE, REL_LT, REL_LE,
        //// bit operator
        //BIT_AND, BIT_OR, BIT_XOR, BIT_NOT, BIT_SHL, BIT_SHR,

        //// punctuator: ( ) [ ] { } ; ... 
        //LP, RP, LSB, RSB, BLK_BEGIN, BLK_END, STMT_END, VAR_PARAM,
        OP_0, OP_1, OP_2, OP_3, OP_4, OP_5, OP_6, OP_7, OP_8, OP_9, OP_10, OP_11, OP_12, OP_13, OP_14, OP_15, OP_16, OP_17, OP_18, OP_19, OP_20, OP_21, OP_22, OP_23, OP_24, OP_25, OP_26, OP_27, OP_28, OP_29, OP_30, OP_COMMA, OP_32, OP_33, OP_34, OP_35, OP_36, RPAREN, LPAREN, OP_39, OP_40, OP_41, OP_42, OP_43, OP_TOKEN_PASTING, OP_STRINGIZING, OP_46, OP_47,

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

        SPACE, NEW_LINE, END,
    };

    Type type;
    StringRef text;

    Token() : type(UNKNOWN), text() {}
    Token(Type type_, StringRef text_) : type(type_), text(text_) {}
    Token(const Token & t) : type(t.type), text(t.text) {}
    Token(Token && t) : type(t.type), text(t.text) {}
    Token & operator = (const Token & t)
    {
        this->~Token();
        new (this) Token(t);
        return *this;
    }
    Token & operator = (Token && t)
    {
        this->~Token();
        new (this) Token(t);
        return *this;
    }
    ~Token() {}
};

#define LEX_ERROR(message) do { throw std::invalid_argument(std::string("Lex error: ") + message); } while (false)
#define LEX_EXPECT_TOKEN(actual, expect_type) do { \
    if ((actual).type != (expect_type)) \
        throw std::invalid_argument(std::string("Lex error: unexpect token ") + (actual).text.toString()); \
    } while (false)

class TokenBuffer
{
public:
    TokenBuffer()
    {
        Token nl = {Token::NEW_LINE, "\n"};
        buffer.push_back(nl);
    }
    void PushFront(Token token)
    {
        buffer.push_front(token);
    }
    void PushFront(std::vector<Token> & tokens)
    {
        buffer.insert(buffer.begin(), tokens.begin(), tokens.end());
    }
    Token::Type Peek() const
    {
        if (buffer.empty())
            LEX_ERROR("Unexpected end-of-file.");
        return buffer.front().type;
    }
    std::vector<Token> PopLine()
    {
        if (buffer.empty())
            LEX_ERROR("Unexpected end-of-file.");
        std::vector<Token> tokens;
        while (Peek() != Token::NEW_LINE)
        {
            tokens.push_back(buffer.front());
            buffer.pop_front();
        }
        return tokens;
    }
    Token Pop()
    {
        if (buffer.empty())
            LEX_ERROR("Unexpected end-of-file.");
        Token token = buffer.front();
        buffer.pop_front();
        return token;
    }
    //bool TrySkipIf(Token::Type type)
    //{
    //    if (!buffer.empty() && buffer.front().type == type)
    //    {
    //        buffer.pop_front();
    //        return true;
    //    }
    //    else
    //        return false;
    //}
    //bool TrySkipIfNot(Token::Type type)
    //{
    //    if (!buffer.empty() && buffer.front().type != type)
    //    {
    //        buffer.pop_front();
    //        return true;
    //    }
    //    else
    //        return false;
    //}
    //bool Empty() const
    //{
    //    return buffer.empty();
    //}
    bool More() const
    {
        return !buffer.empty();
    }
    size_t NumberOfTokens() const
    {
        return buffer.size();
    }
private:
    std::deque<Token> buffer;
};

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
    Token::OP_0, Token::OP_1, Token::OP_2, Token::OP_3, Token::OP_4, Token::OP_5, Token::OP_6, Token::OP_7, Token::OP_8, Token::OP_9, Token::OP_10, Token::OP_11, Token::OP_12, Token::OP_13, Token::OP_14, Token::OP_15, Token::OP_16, Token::OP_17, Token::OP_18, Token::OP_19, Token::OP_20, Token::OP_21, Token::OP_22, Token::OP_23, Token::OP_24, Token::OP_25, Token::OP_26, Token::OP_27, Token::OP_28, Token::OP_29, Token::OP_30, Token::OP_COMMA, Token::OP_32, Token::OP_33, Token::OP_34, Token::OP_35, Token::OP_36, Token::RPAREN, Token::LPAREN, Token::OP_39, Token::OP_40, Token::OP_41, Token::OP_42, Token::OP_43, Token::OP_TOKEN_PASTING, Token::OP_STRINGIZING, Token::OP_46, Token::OP_47,
    Token::SPACE,
    Token::NEW_LINE,
    Token::END,
};

bool IsPreprocDirective(Token::Type type)
{
    return Token::PPD_IF <= type && type <= Token::PPD_NULL;
}

Token ParseOneToken(Matcher & m, std::string & text)
{
    std::vector<MatchResult> mr = m.Match(StringRef(text.data(), text.length()));
    if (mr.size() != 1 || mr.front().which == 0)
        LEX_ERROR("Can't parse text to one token: " + text);
    auto & r = mr.front();
    Token t;
    t.text = StringRef(text.data() + r.offset, r.length);
    t.type = LexTypes.at(r.which - 1);
    return t;
}
std::vector<Token> ParseTokens(Matcher & m, std::string & text)
{
    // Text --dfa--> Raw Tokens (keyword, id, flt/int/char/str-const, op, punc, pp-directive, pp-path, pp-lparen, space, nl)
    std::vector<MatchResult> mr = m.Match(StringRef(text.data(), text.length()));

    std::vector<Token> tokens;
    for (auto r : mr)
    {
        Token t;
        t.text = StringRef(text.data() + r.offset, r.length);
        t.type = LexTypes.at(r.which - 1);

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

    // Raw Tokens --filter--> Tokens (keyword, id, flt/int/char/str-const, op, punc, pp-directive, pp-path, pp-lparen, nl)
    std::vector<Token> filtered_tokens;
    for (auto & token : tokens)
    {
        if (token.type != Token::SPACE)
            filtered_tokens.push_back(token);
    }

    return filtered_tokens;
}

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
        : tag(OBJECT), object{replaceTokens} {}
    PPMacroSubstitution(size_t paramCount, std::vector<std::variant<Token, int>> & replaceTokens)
        : tag(FUNCTION), function{paramCount, replaceTokens} {}
};
struct PPContext
{
    // macro object/func
    std::map<std::string, PPMacroSubstitution> macroSubs;
    // expr eval
    // file include

    std::unordered_set<std::string> disabledMacroSubs;
};
// PP Token Buffer { InsertFront,GetFirst,GetRange }
// PP Macro Table
//   obj: token-id -> token[]
//   fun: token-id -> (token|param-placeholder)[]

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
        auto it = context.macroSubs.find(tokens[0].text.toString());
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
// TokenRange ShrinkToToken(Token::Type, TokenRange)
// TokenRange ApplyPP(PPType, PPContext, TokenRange)
//      token-range -> emit, none
//      token-range -> macro-obj-def, none
//      token-range -> macro-fun-def, none
//      token-range -> token-range (cond inc, file inc)

// std::string GetFileContent(std::string)

std::vector<Token> PreprocessTokens(Matcher & matcher, const std::vector<Token> & in)
{
    std::vector<Token> out;

    PPContext context;

    struct MacroSubNode
    {
        std::shared_ptr<MacroSubNode> parent;
        std::string macroName;
    };

    struct AnnotatedToken
    {
        Token token;
        std::shared_ptr<MacroSubNode> macroSubInfo;
        
        AnnotatedToken() {}
        AnnotatedToken(Token t) : token(t) {}
        AnnotatedToken(Token t, std::shared_ptr<MacroSubNode> m) : token(t), macroSubInfo(m) {}
        operator Token () { return token; }
    };

    std::deque<AnnotatedToken> unprocessed(in.cbegin(), in.cend());

    while (!unprocessed.empty())
    {
        Token peekTokens[3] = {
            unprocessed.front(),
            unprocessed.size() > 1 ? unprocessed[1] : Token{Token::UNKNOWN, ""},
            unprocessed.size() > 2 ? unprocessed[2] : Token{Token::UNKNOWN, ""},
        };

        // Decide process type (token -> pp-type)
        // 1. Emit
        // 2. PP Define Macro Object
        // 3. PP Define Macro Function
        // 4. PP Macro Substitution
        // 5. PP Conditional Include
        // 6. PP File Include
        PPType ppt = DecidePPType(context, peekTokens);

        // Find all tokens in this process (token-range -> token-range) - modify right bound
        // 1. One
        // 2. Until new line
        // 3. Until new line
        // 4. One/Until arglist end
        // 5. Until #endif
        // 6. Until new line
        auto begin = unprocessed.cbegin();
        auto end = begin;
        if (ppt == PPType::EMIT || ppt == PPType::MACRO_OBJ_SUB)
        {
            ++end;
        }
        else if (ppt == PPType::MACRO_FUN_SUB)
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
        else if (ppt == PPType::COND_INCL)
        {
            ++end; // skip #if
            while (end->token.type != Token::PPD_ENDIF)
                ++end;
            ++end; // include #endif
        }
        else
        {
            assert(ppt == PPType::DEF_MACRO_FUN || ppt == PPType::DEF_MACRO_OBJ);
            ++end; // skip #define
            while (end->token.type != Token::NEW_LINE)
                ++end;
            ++end; // include new-line
        }
        
        std::vector<AnnotatedToken> tokensCopy(begin, end);

        //std::cout << "Expand macro..." << std::endl;
        //for (AnnotatedToken & at : tokensCopy)
        //{
        //    std::cout << "\t" << at.token.text.toString() << " [";
        //    auto msi = at.macroSubInfo;
        //    while (msi)
        //    {
        //        std::cout << msi->macroName << ",";
        //        msi = msi->parent;
        //    }
        //    std::cout << "]" << std::endl;
        //}
        
        unprocessed.erase(begin, end);

        // Apply actions (token-range, context -> token-range) - output, update state, read file, ...
        // 1. add to output
        // 2. update pp-macro-table
        // 3. update pp-macro-table
        // 4. replace pp-macro
        // 5. use pp-expr-eval, replace tokens
        // 6. get file content, convert to tokens, insert to in.
        {
            auto begin = tokensCopy.cbegin();
            auto end = tokensCopy.cend();

            if (ppt == PPType::EMIT)
            {
                std::transform(begin,
                               end,
                               std::back_inserter(out),
                               [] (const AnnotatedToken & token) -> const Token & { return token.token; });
            }
            else if (ppt == PPType::DEF_MACRO_OBJ)
            {
                ++begin; // skip #define
                --end; // ignore new-line

                LEX_EXPECT_TOKEN(begin->token, Token::ID);
                std::string id = begin->token.text.toString();
                ++begin;
            
                if (context.macroSubs.find(id) != context.macroSubs.end())
                    LEX_ERROR("Duplicate macro definition: " + id);
                std::vector<Token> replaceTokens;
                std::transform(begin,
                               end,
                               std::back_inserter(replaceTokens),
                               [](const AnnotatedToken & token) -> const Token &{ return token.token; });
                context.macroSubs.emplace(id, replaceTokens);
            }
            else if (ppt == PPType::DEF_MACRO_FUN)
            {
                ++begin; // skip #define
                --end; // ignore new-line

                LEX_EXPECT_TOKEN(begin->token, Token::ID);
                std::string id = begin->token.text.toString();
                ++begin;

                LEX_EXPECT_TOKEN(begin->token, Token::PP_LPAREN);
                ++begin;

                // parse param-list
                std::vector<std::string> paramIds;
                while (begin->token.type != Token::RPAREN)
                {
                    for (std::string & paramId : paramIds)
                    {
                        if (paramId == begin->token.text.toString())
                            LEX_ERROR("Illegal macro function definition: duplicate parameter name '" + paramId + "'.");
                    }

                    LEX_EXPECT_TOKEN(begin->token, Token::ID);
                    paramIds.emplace_back(begin->token.text.toString());
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
                    auto paramIdIt = std::find(paramIds.begin(), paramIds.end(), begin->token.text.toString());
                    if (begin->token.type == Token::ID && paramIdIt != paramIds.end())
                        vTokenOrParam.emplace_back(std::distance(paramIds.begin(), paramIdIt));
                    else
                        vTokenOrParam.emplace_back(begin->token);
                    ++begin;
                }

                if (context.macroSubs.find(id) != context.macroSubs.end())
                    LEX_ERROR("Duplicate macro definition: " + id);
                context.macroSubs.emplace(id, PPMacroSubstitution(paramIds.size(), vTokenOrParam));
            }
            else if (ppt == PPType::MACRO_OBJ_SUB)
            {
                // Expand macro object

                std::string macroName = begin->token.text.toString();

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
                {
                    std::transform(begin,
                                   end,
                                   std::back_inserter(out),
                                   [](const AnnotatedToken & token) -> const Token & { return token.token; });
                    continue;
                }

                auto & objSub = context.macroSubs.find(macroName)->second.object;

                auto annotate = std::make_shared<MacroSubNode>();
                annotate->parent = begin->macroSubInfo;
                annotate->macroName = macroName;

                std::transform(objSub.replaceTokens.begin(),
                               objSub.replaceTokens.end(),
                               std::front_inserter(unprocessed),
                               [annotate](const Token & token) -> AnnotatedToken { return { token, annotate }; });
            }
            else if (ppt == PPType::MACRO_FUN_SUB)
            {
                // Expand macro function

                std::string macroName = begin->token.text.toString();

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
                {
                    std::transform(begin,
                                   end,
                                   std::back_inserter(out),
                                   [](const AnnotatedToken & token) -> const Token &{ return token.token; });
                    continue;
                }
                
                auto & funcSub = context.macroSubs.find(macroName)->second.function;

                auto annotate = std::make_shared<MacroSubNode>();
                annotate->parent = begin->macroSubInfo;
                annotate->macroName = macroName;

                ++begin;

                LEX_EXPECT_TOKEN(begin->token, Token::LPAREN);
                ++begin;

                // collect argment tokens
                //  arglist := expr (',' expr)*
                //  expr := ([^,]+|\([^)]*\))
                std::vector<std::vector<AnnotatedToken>> argReplaceTokens;
                std::vector<AnnotatedToken> replaceTokens;
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

                std::transform(funcSub.replaceTokens.begin(),
                               funcSub.replaceTokens.end(),
                               std::back_inserter(rt),
                               [](const std::variant<Token, int> & v) -> std::variant<AnnotatedToken, int> {
                                   return v.index() == 0
                                       ? std::variant<AnnotatedToken, int>(std::get<0>(v))
                                       : std::variant<AnnotatedToken, int>(std::get<1>(v));
                               });

                // stringizing operator (#)
                for(auto curr = rt.begin(); curr != rt.end(); ++curr)
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
                        t.text = StringRef(leakyText->data(), leakyText->size());
                        t.type = Token::STRING;
                        rt2.emplace_back(t);
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
                        std::string * leakyMergedText = new std::string(std::get<0>(rt[l]).token.text.toString() + std::get<0>(rt[r]).token.text.toString());
                        Token mergedToken = ParseOneToken(matcher, *leakyMergedText);
                        rt[r] = mergedToken;
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

                std::transform(rt2.rbegin(),
                               rt2.rend(),
                               std::front_inserter(unprocessed),
                               [annotate](const std::variant<AnnotatedToken, int> & v) -> AnnotatedToken { return { std::get<AnnotatedToken>(v).token, annotate }; });
            }
            else if (ppt == PPType::COND_INCL)
            {
            }
            else if (ppt == PPType::FILE_INCL)
            {
            }
            }
    }

    return out;
}


#ifdef UNIT_TEST
#include "../UnitTest/UnitTest.h"

class LexerTest : public Tester
{
protected:
    void BeforeAllTestCases() override
    {
        lexMatcher.Compile(LexPatterns);
    }
    Matcher & GetMatcher()
    {
        return lexMatcher;
    }
private:
    static Matcher lexMatcher;
};
Matcher LexerTest::lexMatcher;


// string -> Token[]
TEST_F(LexerTest, ParseTokens)
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
        { Token::OP_0, "~" },
        { Token::OP_1, "}" },
        { Token::OP_2, "||" },
        { Token::OP_3, "|=" },
        { Token::OP_4, "|" },
        { Token::OP_5, "{" },
        { Token::OP_6, "^=" },
        { Token::OP_7, "^" },
        { Token::OP_8, "]" },
        { Token::OP_9, "[" },
        { Token::OP_10, "?" },
        { Token::OP_11, ">>=" },
        { Token::OP_12, ">>" },
        { Token::OP_13, ">=" },
        { Token::OP_14, ">" },
        { Token::OP_15, "==" },
        { Token::OP_16, "=" },
        { Token::OP_17, "<=" },
        { Token::OP_18, "<<=" },
        { Token::OP_19, "<<" },
        { Token::OP_20, "<" },
        { Token::OP_21, ";" },
        { Token::OP_22, ":" },
        { Token::OP_23, "/=" },
        { Token::OP_24, "/" },
        { Token::OP_25, "..." },
        { Token::OP_26, "." },
        { Token::OP_27, "->" },
        { Token::OP_28, "-=" },
        { Token::OP_29, "--" },
        { Token::OP_30, "-" },
        { Token::OP_COMMA, "," },
        { Token::OP_32, "+=" },
        { Token::OP_33, "++" },
        { Token::OP_34, "+" },
        { Token::OP_35, "*=" },
        { Token::OP_36, "*" },
        { Token::RPAREN, ")" },
        { Token::LPAREN, "(" },
        { Token::OP_39, "&=" },
        { Token::OP_40, "&&" },
        { Token::OP_41, "&" },
        { Token::OP_42, "%=" },
        { Token::OP_43, "%" },
        { Token::OP_TOKEN_PASTING, "##" },
        { Token::OP_STRINGIZING, "#" },
        { Token::OP_46, "!=" },
        { Token::OP_47, "!" },
        { Token::NEW_LINE, "\n" },
    };
    try
    {
        std::vector<Token> tokens = ParseTokens(GetMatcher(), input);
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
TEST_F(LexerTest, PreprocessTokens)
{
    //try
    {
        std::string input =
            "#define I 5                \n"
            "#define ADD(a, b) (a + b)  \n"
            "#define STR(x) #x          \n"
            "#define CONCAT(x, y) x##y  \n"
            "#define FOO(x) BAR(x)BAR(x)\n"
            "#define BAR(x) FOO(x)      \n"
            "I                          \n"
            "ADD(2, 3)                  \n"
            "ADD(I, I)                  \n"
            "STR(apple)                 \n"
            "CONCAT(ye, ah)             \n"
            "FOO(a)                     \n"
            "BAR(a)                     \n"
            ;
        std::string output =
            "5                          \n"
            "(2 + 3)                    \n"
            "(5 + 5)                    \n"
            "\"apple\"                  \n"
            "yeah                       \n"
            "FOO(a) FOO(a)              \n"
            "BAR(a) BAR(a)              \n"
            ;

        std::vector<Token> expectTokens = ParseTokens(GetMatcher(), output);

        std::vector<Token> inputTokens = ParseTokens(GetMatcher(), input);
        std::vector<Token> actualTokens = PreprocessTokens(GetMatcher(), inputTokens);

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
    //catch (const std::exception & e)
    //{
    //    std::cerr << "Caught exception: " << e.what() << std::endl;
    //    EXPECT_NOT_REACH;
    //}
}

#endif
