#pragma once

#include <cassert>
#include <set>
#include <string>

typedef char Token;
typedef std::set<Token> TokenSet;

class TokenIterator {
public:
    TokenIterator(std::string tokens)
        : tokens_(tokens)
        , i_(0) {
    }
    void reset() {
        i_ = 0;
    }
    bool has() const {
        return i_ < tokens_.size();
    }
    Token peek() const {
        assert(i_ < tokens_.size());
        return tokens_[i_];
    }
    Token next() {
        assert(i_ < tokens_.size());
        return tokens_[i_++];
    }

private:
    std::string tokens_;
    size_t i_;
};
