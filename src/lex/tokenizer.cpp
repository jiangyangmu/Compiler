#include "tokenizer.h"

#include <cctype>
#include <unordered_map>
#include <xutility>

SourceSanner::SourceSanner(StringRef source)
    : source_(std::move(source.toString()))
    , i_(0) {
    if (source_.empty() || source_.back() != EOL)
        source_.push_back(EOL);
}

StringRef SourceSanner::readLine() {
    assert(i_ < source_.size());

    size_t start = i_;
    size_t end = i_;
    while (source_[end] != EOL)
        ++end;
    ++end;

    i_ = end;

    return StringRef(source_.data() + start, end - start);
}

bool SourceSanner::eof() const {
    return i_ == source_.size();
}

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
    while (*p != EOL && isspace(*p))
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
        if (*end == EOL)
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

    do
    {
        if (*end == EOL)
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
    } while (*end != '"');
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

static inline double EvalFloat(StringRef text) {
    // TODO implement EvalFloat()
    assert(text == "1.0");
    return 1.0;
}

static inline int EvalInt(StringRef text) {
    // TODO implement EvalInt()
    assert(text == "1");
    return 1;
}

static inline int EvalChar(StringRef text) {
    // TODO implement EvalChar()
    assert(text == "'a'");
    return 'a';
}

static inline Token RecognizeToken(const char * start,
                                   const char * end,
                                   const TokenRecog::Info & info) {
    Token token;

    token.text = StringRef(start, end - start);
    token.type = Token::UNKNOWN;

    assert(info.category != TokenRecog::UNKNOWN);
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
        default:
            break;
    }

    return token;
}

TokenIterator::TokenIterator(std::vector<Token> & tokens)
    : tokens_(tokens)
    , i_(0) {
}

bool TokenIterator::has() const {
    return i_ < tokens_.size();
}

Token TokenIterator::next() {
    assert(i_ < tokens_.size());
    return tokens_[i_++];
}

Token TokenIterator::peek() const {
    assert(i_ < tokens_.size());
    return tokens_[i_];
}

Token TokenIterator::peekN(int n) const {
    assert(i_ + n < tokens_.size());
    return tokens_[i_ + n];
}

TokenMatcher::TokenMatcher() {
    token_.type = Token::UNKNOWN;
}

TokenMatcher::TokenMatcher(Token token)
    : token_(token) {
}

bool TokenMatcher::match(const Token & token) const {
    return token_.type == token.type;
}

std::string TokenMatcher::toString() const {
    return token_.text.toString();
}

bool TokenMatcher::operator==(const TokenMatcher & other) const {
    return token_.type == other.token_.type;
}

void TokenMatcherSet::addMatcher(TokenMatcher m) {
    for (auto & matcher : matchers_)
    {
        if (m == matcher)
            return;
    }
    matchers_.emplace_back(m);
}

void TokenMatcherSet::addMatchers(const TokenMatcherSet & ms) {
    for (auto & matcher : ms.matchers_)
    {
        addMatcher(matcher);
    }
}

bool TokenMatcherSet::match(const Token & token) const {
    for (auto & matcher : matchers_)
    {
        if (matcher.match(token))
            return true;
    }
    return false;
}

size_t TokenMatcherSet::size() const {
    return matchers_.size();
}

bool TokenMatcherSet::empty() const {
    return matchers_.empty();
}

const std::vector<TokenMatcher> & TokenMatcherSet::matchers() const {
    return matchers_;
}

void Tokenizer::compile(SourceSanner & scanner) {
    tokens_.clear();
    while (!scanner.eof())
    {
        StringRef line = scanner.readLine();
        assert(!line.empty() && line.back() == EOL);

        const char * start;
        const char * end = line.data();
        do
        {
            TokenRecog::Info info;

            // Tokenize.
            start = TokenStart(end);
            if (*start == EOL)
                break;
            end = TokenEnd(start, &info);
            assert(end > start); // ERROR: invalid character sequence if fail.

            // Recognize.
            Token token = RecognizeToken(start, end, info);
            tokens_.push_back(token);
        } while (true);
    }
}

TokenIterator Tokenizer::getIterator() {
    return TokenIterator(tokens_);
}

Token TokenFromString(std::string s) {
    const char * start;
    const char * end;
    TokenRecog::Info info;

    start = TokenStart(s.data());
    end = TokenEnd(start, &info);
    assert(end > start); // ERROR: invalid character sequence if fail.

    Token token = RecognizeToken(start, end, info);
    return token;
}
