#include "../Source/UnitTest/UnitTest.h"

#include "../Source/Preprocess/LineSlicer.h"
#include "../Source/Preprocess/Tokenizer.h"

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
