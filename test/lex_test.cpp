#include "testing/tester.h"

#include "lex/tokenizer.h"

class LexTest : public Tester {
public:
    virtual void setUp() {
    }
    virtual void shutDown() {
    }
    TokenIterator Tokens(StringRef source) {
        scanner_ = SourceSanner(source);
        tokenizer_.compile(scanner_);
        return tokenizer_.getIterator();
    }
    std::vector<Token::Type> Types(TokenIterator tokens) {
        std::vector<Token::Type> result;
        while (tokens.has())
            result.push_back(tokens.next().type);
        return result;
    }
    std::vector<Token::Type> Types(std::initializer_list<Token::Type> types) {
        return types;
    }
    std::vector<std::pair<Token::Type, StringRef>> TypesAndTexts(
        TokenIterator tokens) {
        std::vector<std::pair<Token::Type, StringRef>> result;
        while (tokens.has())
        {
            Token token = tokens.next();
            result.emplace_back(token.type, token.text);
        }
        return result;
    }
    std::vector<std::pair<Token::Type, StringRef>> TypesAndTexts(
        std::initializer_list<std::pair<Token::Type, StringRef>>
            types_and_texts) {
        return types_and_texts;
    }
    std::vector<std::pair<Token::Type, double>> TypesAndFValue(
        TokenIterator tokens) {
        std::vector<std::pair<Token::Type, double>> result;
        while (tokens.has())
        {
            Token token = tokens.next();
            result.emplace_back(token.type, token.fval);
        }
        return result;
    }
    std::vector<std::pair<Token::Type, double>> TypesAndFValue(
        std::initializer_list<std::pair<Token::Type, double>> types_and_fvals) {
        return types_and_fvals;
    }
    std::vector<std::pair<Token::Type, int>> TypesAndIValue(
        TokenIterator tokens) {
        std::vector<std::pair<Token::Type, int>> result;
        while (tokens.has())
        {
            Token token = tokens.next();
            result.emplace_back(token.type, token.ival);
        }
        return result;
    }
    std::vector<std::pair<Token::Type, int>> TypesAndIValue(
        std::initializer_list<std::pair<Token::Type, int>> types_and_fvals) {
        return types_and_fvals;
    }
    std::vector<std::pair<Token::Type, int>> TypesAndCValue(
        TokenIterator tokens) {
        std::vector<std::pair<Token::Type, int>> result;
        while (tokens.has())
        {
            Token token = tokens.next();
            result.emplace_back(token.type, token.cval);
        }
        return result;
    }
    std::vector<std::pair<Token::Type, int>> TypesAndCValue(
        std::initializer_list<std::pair<Token::Type, int>> types_and_fvals) {
        return types_and_fvals;
    }

private:
    SourceSanner scanner_;
    Tokenizer tokenizer_;
};

template <class T>
std::ostream & operator<<(std::ostream & o,
                          const std::pair<Token::Type, T> & p) {
    o << '(' << p.first << ',' << p.second << ')';
    return o;
}

TEST_F(LexTest, Keyword) {
    EXPECT_EQ_LIST(
        Types(Tokens(
            "auto double int struct break else long switch case enum register "
            "typedef char extern return union const float short unsigned "
            "continue for signed void default goto sizeof volatile do if "
            "static while")),
        Types({Token::KW_AUTO,     Token::KW_DOUBLE,   Token::KW_INT,
               Token::KW_STRUCT,   Token::KW_BREAK,    Token::KW_ELSE,
               Token::KW_LONG,     Token::KW_SWITCH,   Token::KW_CASE,
               Token::KW_ENUM,     Token::KW_REGISTER, Token::KW_TYPEDEF,
               Token::KW_CHAR,     Token::KW_EXTERN,   Token::KW_RETURN,
               Token::KW_UNION,    Token::KW_CONST,    Token::KW_FLOAT,
               Token::KW_SHORT,    Token::KW_UNSIGNED, Token::KW_CONTINUE,
               Token::KW_FOR,      Token::KW_SIGNED,   Token::KW_VOID,
               Token::KW_DEFAULT,  Token::KW_GOTO,     Token::KW_SIZEOF,
               Token::KW_VOLATILE, Token::KW_DO,       Token::KW_IF,
               Token::KW_STATIC,   Token::KW_WHILE}));
}

TEST_F(LexTest, Id) {
    EXPECT_EQ_LIST(TypesAndTexts(Tokens("_i aI A0 a_Very_L0ng_Nam3")),
                   TypesAndTexts({{Token::ID, "_i"},
                                  {Token::ID, "aI"},
                                  {Token::ID, "A0"},
                                  {Token::ID, "a_Very_L0ng_Nam3"}}));
}

TEST_F(LexTest, OperatorAndPunctuator) {
    EXPECT_EQ_LIST(
        Types(Tokens(
            "[ ] ( ) { } . -> ++ -- & * + - ~ ! / % << >> < > <= >= == != ^ | "
            "&& || ? : = *= /= %= += -= <<= >>= &= ^= |= , ; ...")),
        Types({Token::LSB,        Token::RSB,        Token::LP,
               Token::RP,         Token::BLK_BEGIN,  Token::BLK_END,
               Token::OP_DOT,     Token::OP_POINTTO, Token::OP_INC,
               Token::OP_DEC,     Token::BIT_AND,    Token::OP_MUL,
               Token::OP_ADD,     Token::OP_SUB,     Token::BIT_NOT,
               Token::BOOL_NOT,   Token::OP_DIV,     Token::OP_MOD,
               Token::BIT_SHL,    Token::BIT_SHR,    Token::REL_LT,
               Token::REL_GT,     Token::REL_LE,     Token::REL_GE,
               Token::REL_EQ,     Token::REL_NE,     Token::BIT_XOR,
               Token::BIT_OR,     Token::BOOL_AND,   Token::BOOL_OR,
               Token::OP_QMARK,   Token::OP_COLON,   Token::ASSIGN,
               Token::MUL_ASSIGN, Token::DIV_ASSIGN, Token::MOD_ASSIGN,
               Token::ADD_ASSIGN, Token::SUB_ASSIGN, Token::SHL_ASSIGN,
               Token::SHR_ASSIGN, Token::AND_ASSIGN, Token::XOR_ASSIGN,
               Token::OR_ASSIGN,  Token::OP_COMMA,   Token::STMT_END,
               Token::VAR_PARAM}));
}

TEST_F(LexTest, Float) {
    // TODO use hex value, not float constant.
    EXPECT_EQ_LIST(TypesAndFValue(Tokens("1.0")),
                   TypesAndFValue({{Token::CONST_FLOAT, 1.0}}));
}

TEST_F(LexTest, Int) {
    EXPECT_EQ_LIST(TypesAndIValue(Tokens("1")),
                   TypesAndIValue({{Token::CONST_INT, 1}}));
}

TEST_F(LexTest, Char) {
    EXPECT_EQ_LIST(TypesAndCValue(Tokens("'a'")),
                   TypesAndCValue({{Token::CONST_CHAR, 'a'}}));
}
