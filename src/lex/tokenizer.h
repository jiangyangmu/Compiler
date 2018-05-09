#pragma once

#include "token.h"

class SourceSanner {
public:
    SourceSanner() = default;
    explicit SourceSanner(StringRef source);
    StringRef readLine();
    bool eof() const;

private:
    std::string source_;
    size_t i_;
};

class Tokenizer {
public:
    void compile(SourceSanner & scanner);
    TokenIterator getIterator();

private:
    std::vector<Token> tokens_;
};

#define EOL ('\n')
