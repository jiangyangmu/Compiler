#include "../Source/UnitTest/UnitTest.h"

#include "../Source/Preprocess/LineSlicer.h"
#include "../Source/Preprocess/Tokenizer.h"
#include "../Source/Preprocess/Lexer.h"

TEST(LineSlicer_Simple)
{
    std::string in = (
        "line 1\n"
        "line 2\r\n"
        "line\\\n"
        "3\n"
        "line 5\n"
        "\n"
    );

    std::vector<std::string> out = {
        "line 1",
        "line 2",
        "line 3",
        "#line 5",
        "line 5",
        "",
    };

    auto lines = ASCIILineSlice(in);
    EXPECT_EQ(lines.size(), out.size());
    for (size_t i = 0; i < out.size(); ++i)
    {
        EXPECT_EQ(lines[i], out[i]);
    }
}

TEST(Tokenizer_Simple)
{
    std::vector<std::string> in = {
        "int a;",
        "1;",
    };

    std::vector<Token::Type> out = {
        Token::LINE_START, Token::KW_INT, Token::ID, Token::STMT_END,
        Token::LINE_START, Token::CONST_INT, Token::STMT_END,
        Token::FILE_END,
    };

    auto tokens = Tokenize(in);
    EXPECT_EQ(tokens.size(), out.size());
    for (size_t i = 0; i < out.size(); ++i)
    {
        EXPECT_EQ(tokens[i].type, out[i]);
    }
}

TEST(Tokenizer_PreprocDirective)
{
    std::vector<std::string> in = {
        "#if",
        "#ifdef",
        "#ifndef",
        "#elif",
        "#else",
        "#endif",
        "#define",
        "#undef",
        "#include",
        "#line",
        "#error",
        "#pragma",
        "#",
    };

    std::vector<Token::Type> out = {
        Token::LINE_START, Token::PPD_IF ,
        Token::LINE_START, Token::PPD_IFDEF,
        Token::LINE_START, Token::PPD_IFNDEF,
        Token::LINE_START, Token::PPD_ELIF,
        Token::LINE_START, Token::PPD_ELSE,
        Token::LINE_START, Token::PPD_ENDIF,
        Token::LINE_START, Token::PPD_DEFINE,
        Token::LINE_START, Token::PPD_UNDEF,
        Token::LINE_START, Token::PPD_INCLUDE,
        Token::LINE_START, Token::PPD_LINE,
        Token::LINE_START, Token::PPD_ERROR,
        Token::LINE_START, Token::PPD_PRAGMA,
        Token::LINE_START, Token::PPD_NULL,
        Token::FILE_END,
    };

    auto tokens = Tokenize(in);
    EXPECT_EQ(tokens.size(), out.size());
    for (size_t i = 0; i < out.size(); ++i)
    {
        EXPECT_EQ(tokens[i].type, out[i]);
    }
}

TEST(Tokenizer_UnknownToken)
{
    std::vector<std::string> in = {
        "#unknown",
    };

    try
    {
        auto tokens = Tokenize(in);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Caught exception: " << e.what() << std::endl;
        EXPECT_EQ(e.what(), std::string("Unrecognized token."));
    }
}

TEST(Lexer_Complete)
{
    DfaInput input;

    NfaStateFactoryScope scope(&input.nfaStateFactory);
    std::vector<NfaGraph> vc = {
        FromRegex("while"),
        FromRegex("volatile"),
        FromRegex("void"),
        FromRegex("unsigned"),
        FromRegex("union"),
        FromRegex("typedef"),
        FromRegex("switch"),
        FromRegex("struct"),
        FromRegex("static"),
        FromRegex("sizeof"),
        FromRegex("signed"),
        FromRegex("short"),
        FromRegex("return"),
        FromRegex("register"),
        FromRegex("long"),
        FromRegex("int"),
        FromRegex("if"),
        FromRegex("goto"),
        FromRegex("for"),
        FromRegex("float"),
        FromRegex("extern"),
        FromRegex("enum"),
        FromRegex("else"),
        FromRegex("double"),
        FromRegex("do"),
        FromRegex("default"),
        FromRegex("continue"),
        FromRegex("const"),
        FromRegex("char"),
        FromRegex("case"),
        FromRegex("break"),
        FromRegex("auto"),
        FromRegex("(a|b|c|d|e|f|g|h|i|j|k|l|m|n|o|p|q|r|s|t|u|v|w|x|y|z)+"),
    };
    input.nfaGraphs = vc;

    Dfa dfa = Compile(input);
    PrintDfa(dfa);

    std::vector<std::pair<std::string, size_t>> test_cases = {
        { "while", 1 },
        { "volatile", 2 },
        { "void", 3 },
        { "unsigned", 4 },
        { "union", 5 },
        { "typedef", 6 },
        { "switch", 7 },
        { "struct", 8 },
        { "static", 9 },
        { "sizeof", 10 },
        { "signed", 11 },
        { "short", 12 },
        { "return", 13 },
        { "register", 14 },
        { "long", 15 },
        { "int", 16 },
        { "if", 17 },
        { "goto", 18 },
        { "for", 19 },
        { "float", 20 },
        { "extern", 21 },
        { "enum", 22 },
        { "else", 23 },
        { "double", 24 },
        { "do", 25 },
        { "default", 26 },
        { "continue", 27 },
        { "const", 28 },
        { "char", 29 },
        { "case", 30 },
        { "break", 31 },
        { "auto", 32 },
        { "justanidname", 33 },
        { "ifelse", 17 },
    };
    for (auto kv : test_cases)
    {
        auto & text = kv.first;
        auto which = kv.second;
        std::cout << "pattern: C-Lex text: " << text << std::endl;
        EXPECT_EQ(dfa.Run(text).which, which);
    }
}
