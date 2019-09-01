#include "../Source/UnitTest/UnitTest.h"

#include "../Source/Preprocess/LineSlicer.h"
#include "../Source/Preprocess/Tokenizer.h"
#include "../Source/Preprocess/DfaMatcher.h"

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

TEST(DfaMatcher_Complete)
{
    try
    {
        std::vector<std::string> patterns = {
            "[_a-zA-Z][_a-zA-Z0-9]*",
            "#[_a-zA-Z][_a-zA-Z0-9]*",
            "[0-9]+\\.[0-9]*|\\.[0-9]+",
            "[0-9]+",
            "'([^']|\\\\')+'|\"([^\"]|\\\\\")*\"|<([^> ]|\\\\>)+>",
            "~|}|\\|\\||\\|=|\\||{|^=|^|]|\\[|\\?|>>=|>>|>=|>|==|=|<=|<<=|<<|<|;|:|/=|/|\\.\\.\\.|\\.|->|-=|--|-|,|+=|++|+|*=|*|\\)|\\(|&=|&&|&|%=|%|##|#|!=|!",
            "[ \t\r]+",
            "\n",
        };
        std::vector<std::string> types = {
            "ID",
            "DIRECTIVE",
            "FLOAT",
            "INT",
            "RANGE",
            "OP",
            "SPACE",
            "NEW_LINE",
        };
        std::string text =
            "hello_0_this_Is_a_Long_Name good "
            "#hello_0_this_Is_a_Long_Directive #haha "
            "0. 0.0 .0 "
            "0 1 22 4890 "
            "'a0=>?\"\\'' \"a0=>?'\\\"\" <a0=\\>?'\">"
            "~ } || |= | { ^= ^ ] [ ? >>= >> >= > == = <= <<= << < ; : /= / ... . -> -= -- - , += ++ + *= * ) ( &= && & %= % ## # != ! "
            " \t\r"
            "\n\n"
            ;
        std::vector<MatchResult> mr = Match(patterns, StringRef(text.data(), text.length()));

        for (auto r : mr)
        {
            if (r.which > 0)
                std::cout << "Matched: " << types[r.which - 1] << " " << text.substr(r.offset, r.length) << std::endl;
        }
    }
    catch (const std::invalid_argument& e)
    {
        std::cerr << "Caught exception: " << e.what() << std::endl;
    }
}

TEST(Tokenizer_DfaMatcher)
{
    try
    {
        std::string text =
            "hello_0_this_Is_a_Long_Name good "
            "#hello_0_this_Is_a_Long_Directive #haha "
            "0. 0.0 .0 "
            "0 1 22 4890 "
            "'a0=>?\"\\'' \"a0=>?'\\\"\" <a0=\\>?'\">"
            "~ } || |= | { ^= ^ ] [ ? >>= >> >= > == = <= <<= << < ; : /= / ... . -> -= -- - , += ++ + *= * ) ( &= && & %= % ## # != ! "
            " \t\r"
            "\n\n"
            ;
        std::vector<Token> tokens = Tokenize(text);

        for (auto t : Tokenize(text))
        {
            std::cout << "Token: " << t.text << std::endl;
        }
    }
    catch (const std::invalid_argument& e)
    {
        std::cerr << "Caught exception: " << e.what() << std::endl;
    }
}