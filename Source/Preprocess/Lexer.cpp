#include <vector>
#include <string>

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
        OP_0, OP_1, OP_2, OP_3, OP_4, OP_5, OP_6, OP_7, OP_8, OP_9, OP_10, OP_11, OP_12, OP_13, OP_14, OP_15, OP_16, OP_17, OP_18, OP_19, OP_20, OP_21, OP_22, OP_23, OP_24, OP_25, OP_26, OP_27, OP_28, OP_29, OP_30, OP_31, OP_32, OP_33, OP_34, OP_35, OP_36, OP_37, OP_38, OP_39, OP_40, OP_41, OP_42, OP_43, OP_44, OP_45, OP_46, OP_47,

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
    "<([^> ]|\\\\>)+>",
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
    Token::OP_0, Token::OP_1, Token::OP_2, Token::OP_3, Token::OP_4, Token::OP_5, Token::OP_6, Token::OP_7, Token::OP_8, Token::OP_9, Token::OP_10, Token::OP_11, Token::OP_12, Token::OP_13, Token::OP_14, Token::OP_15, Token::OP_16, Token::OP_17, Token::OP_18, Token::OP_19, Token::OP_20, Token::OP_21, Token::OP_22, Token::OP_23, Token::OP_24, Token::OP_25, Token::OP_26, Token::OP_27, Token::OP_28, Token::OP_29, Token::OP_30, Token::OP_31, Token::OP_32, Token::OP_33, Token::OP_34, Token::OP_35, Token::OP_36, Token::OP_37, Token::OP_38, Token::OP_39, Token::OP_40, Token::OP_41, Token::OP_42, Token::OP_43, Token::OP_44, Token::OP_45, Token::OP_46, Token::OP_47,
    Token::SPACE,
    Token::NEW_LINE,
    Token::END,
};

std::vector<Token> ParseTokens(std::string & text)
{
    // 1. text --dfa--> raw token (keyword, id, flt/int/char/str-const, op, punc, pp-directive, pp-path, pp-lparen, space, nl, eof)
    std::vector<MatchResult> mr = Match(LexPatterns, StringRef(text.data(), text.length()));

    std::vector<Token> tokens;
    for (auto r : mr)
    {
        Token t;
        t.text = StringRef(text.data() + r.offset, r.length);
        t.type = LexTypes.at(r.which - 1);
        tokens.push_back(t);
    }
    tokens.push_back({Token::END});

    // 2. pp

    return tokens;
}

#ifdef UNIT_TEST
#include "../UnitTest/UnitTest.h"

TEST(Lexer_Simple)
{
    std::string input =
        "while volatile void unsigned union typedef switch struct static sizeof signed short return register long int if goto for float extern enum else double do default continue const char case break auto "
        "_this_iS_A_L0ng_Id "
        "#undef #pragma #line #include #ifndef #ifdef #if #error #endif #else #elif #define "
        "0.0 123 'a' \"hello\" <file.h> "
        "~ } || |= | { ^= ^ ] [ ? >>= >> >= > == = <= <<= << < ; : /= / ... . -> -= -- - , += ++ + *= * ) ( &= && & %= % ## # != ! "
        ;
    std::vector<Token> output = {
        { Token::KW_WHILE, "while" },{ Token::SPACE, " " },
        { Token::KW_VOLATILE, "volatile" },{ Token::SPACE, " " },
        { Token::KW_VOID, "void" },{ Token::SPACE, " " },
        { Token::KW_UNSIGNED, "unsigned" },{ Token::SPACE, " " },
        { Token::KW_UNION, "union" },{ Token::SPACE, " " },
        { Token::KW_TYPEDEF, "typedef" },{ Token::SPACE, " " },
        { Token::KW_SWITCH, "switch" },{ Token::SPACE, " " },
        { Token::KW_STRUCT, "struct" },{ Token::SPACE, " " },
        { Token::KW_STATIC, "static" },{ Token::SPACE, " " },
        { Token::KW_SIZEOF, "sizeof" },{ Token::SPACE, " " },
        { Token::KW_SIGNED, "signed" },{ Token::SPACE, " " },
        { Token::KW_SHORT, "short" },{ Token::SPACE, " " },
        { Token::KW_RETURN, "return" },{ Token::SPACE, " " },
        { Token::KW_REGISTER, "register" },{ Token::SPACE, " " },
        { Token::KW_LONG, "long" },{ Token::SPACE, " " },
        { Token::KW_INT, "int" },{ Token::SPACE, " " },
        { Token::KW_IF, "if" },{ Token::SPACE, " " },
        { Token::KW_GOTO, "goto" },{ Token::SPACE, " " },
        { Token::KW_FOR, "for" },{ Token::SPACE, " " },
        { Token::KW_FLOAT, "float" },{ Token::SPACE, " " },
        { Token::KW_EXTERN, "extern" },{ Token::SPACE, " " },
        { Token::KW_ENUM, "enum" },{ Token::SPACE, " " },
        { Token::KW_ELSE, "else" },{ Token::SPACE, " " },
        { Token::KW_DOUBLE, "double" },{ Token::SPACE, " " },
        { Token::KW_DO, "do" },{ Token::SPACE, " " },
        { Token::KW_DEFAULT, "default" },{ Token::SPACE, " " },
        { Token::KW_CONTINUE, "continue" },{ Token::SPACE, " " },
        { Token::KW_CONST, "const" },{ Token::SPACE, " " },
        { Token::KW_CHAR, "char" },{ Token::SPACE, " " },
        { Token::KW_CASE, "case" },{ Token::SPACE, " " },
        { Token::KW_BREAK, "break" },{ Token::SPACE, " " },
        { Token::KW_AUTO, "auto" },{ Token::SPACE, " " },
        { Token::ID, "_this_iS_A_L0ng_Id" },{ Token::SPACE, " " },
        { Token::PPD_UNDEF, "#undef" },{ Token::SPACE, " " },
        { Token::PPD_PRAGMA, "#pragma" },{ Token::SPACE, " " },
        { Token::PPD_LINE, "#line" },{ Token::SPACE, " " },
        { Token::PPD_INCLUDE, "#include" },{ Token::SPACE, " " },
        { Token::PPD_IFNDEF, "#ifndef" },{ Token::SPACE, " " },
        { Token::PPD_IFDEF, "#ifdef" },{ Token::SPACE, " " },
        { Token::PPD_IF, "#if" },{ Token::SPACE, " " },
        { Token::PPD_ERROR, "#error" },{ Token::SPACE, " " },
        { Token::PPD_ENDIF, "#endif" },{ Token::SPACE, " " },
        { Token::PPD_ELSE, "#else" },{ Token::SPACE, " " },
        { Token::PPD_ELIF, "#elif" },{ Token::SPACE, " " },
        { Token::PPD_DEFINE, "#define" },{ Token::SPACE, " " },
        { Token::CONST_FLOAT, "0.0" },{ Token::SPACE, " " },
        { Token::CONST_INT, "123" },{ Token::SPACE, " " },
        { Token::CONST_CHAR, "'a'" },{ Token::SPACE, " " },
        { Token::STRING, "\"hello\"" },{ Token::SPACE, " " },
        { Token::PP_ENV_PATH, "<file.h>" },{ Token::SPACE, " " },
        { Token::OP_0, "~" }, { Token::SPACE, " " },
        { Token::OP_1, "}" }, { Token::SPACE, " " },
        { Token::OP_2, "||" }, { Token::SPACE, " " },
        { Token::OP_3, "|=" }, { Token::SPACE, " " },
        { Token::OP_4, "|" }, { Token::SPACE, " " },
        { Token::OP_5, "{" }, { Token::SPACE, " " },
        { Token::OP_6, "^=" }, { Token::SPACE, " " },
        { Token::OP_7, "^" }, { Token::SPACE, " " },
        { Token::OP_8, "]" }, { Token::SPACE, " " },
        { Token::OP_9, "[" }, { Token::SPACE, " " },
        { Token::OP_10, "?" }, { Token::SPACE, " " },
        { Token::OP_11, ">>=" }, { Token::SPACE, " " },
        { Token::OP_12, ">>" }, { Token::SPACE, " " },
        { Token::OP_13, ">=" }, { Token::SPACE, " " },
        { Token::OP_14, ">" }, { Token::SPACE, " " },
        { Token::OP_15, "==" }, { Token::SPACE, " " },
        { Token::OP_16, "=" }, { Token::SPACE, " " },
        { Token::OP_17, "<=" }, { Token::SPACE, " " },
        { Token::OP_18, "<<=" }, { Token::SPACE, " " },
        { Token::OP_19, "<<" }, { Token::SPACE, " " },
        { Token::OP_20, "<" }, { Token::SPACE, " " },
        { Token::OP_21, ";" }, { Token::SPACE, " " },
        { Token::OP_22, ":" }, { Token::SPACE, " " },
        { Token::OP_23, "/=" }, { Token::SPACE, " " },
        { Token::OP_24, "/" }, { Token::SPACE, " " },
        { Token::OP_25, "..." }, { Token::SPACE, " " },
        { Token::OP_26, "." }, { Token::SPACE, " " },
        { Token::OP_27, "->" }, { Token::SPACE, " " },
        { Token::OP_28, "-=" }, { Token::SPACE, " " },
        { Token::OP_29, "--" }, { Token::SPACE, " " },
        { Token::OP_30, "-" }, { Token::SPACE, " " },
        { Token::OP_31, "," }, { Token::SPACE, " " },
        { Token::OP_32, "+=" }, { Token::SPACE, " " },
        { Token::OP_33, "++" }, { Token::SPACE, " " },
        { Token::OP_34, "+" }, { Token::SPACE, " " },
        { Token::OP_35, "*=" }, { Token::SPACE, " " },
        { Token::OP_36, "*" }, { Token::SPACE, " " },
        { Token::OP_37, ")" }, { Token::SPACE, " " },
        { Token::OP_38, "(" }, { Token::SPACE, " " },
        { Token::OP_39, "&=" }, { Token::SPACE, " " },
        { Token::OP_40, "&&" }, { Token::SPACE, " " },
        { Token::OP_41, "&" }, { Token::SPACE, " " },
        { Token::OP_42, "%=" }, { Token::SPACE, " " },
        { Token::OP_43, "%" }, { Token::SPACE, " " },
        { Token::OP_44, "##" }, { Token::SPACE, " " },
        { Token::OP_45, "#" }, { Token::SPACE, " " },
        { Token::OP_46, "!=" }, { Token::SPACE, " " },
        { Token::OP_47, "!" }, { Token::SPACE, " " },
        { Token::END, "" },
    };
    std::vector<Token> tokens = ParseTokens(input);
    EXPECT_EQ(tokens.size(), output.size());
    for (size_t i = 0; i < tokens.size(); ++i)
    {
        EXPECT_EQ(tokens[i].type, output[i].type);
        EXPECT_EQ(tokens[i].text, output[i].text);
    }
}

#endif
