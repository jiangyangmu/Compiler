#pragma once

#include <vector>
#include <map>
#include <set>
#include <cassert>
#include <iostream>

/*

    Problem: generate LL(1) parser

    Input: rule files
        A -> BCD
        B -> b
        C -> c
        D -> d

    Output:
        Parser.h/.cpp
            class Visitor;
            class TokenIterator;
            void Parse(Visitor, TokenIterator);

*/

// ==== Entity: Left, Right ====

typedef std::string Token;
struct TokenIterator
{
    TokenIterator(std::vector<Token> t)
        : tokens(t)
        , i(0)
    {}

    const std::string & Peek()
    {
        return tokens.at(i);
    }
    const std::string & Next()
    {
        return tokens.at(i++);
    }

    std::vector<Token> tokens;
    size_t i;
};
struct TokenMatcher
{
    TokenMatcher(Token token)
        : testToken(token)
    {}

    bool Match(const Token & token)
    {
        return testToken == token;
    }

    Token testToken;
};

typedef std::string Left;

// Right type
// * Left reference
// * Token matcher
// * Structural
struct Right
{
    Left * leftRef;
    TokenMatcher * tokenMatcher;

    Right * next;
    Right * alt;
};
Right * FromLeftRef(Left left)
{
    Right * right = new Right;
    right->leftRef = new Left(left);
    right->tokenMatcher = nullptr;
    right->next = nullptr;
    right->alt = nullptr;
    return right;
}
Right * FromTokenMatcher(TokenMatcher tm)
{
    Right * right = new Right;
    right->leftRef = nullptr;
    right->tokenMatcher = new TokenMatcher(tm);
    right->next = nullptr;
    right->alt = nullptr;
    return right;
}
bool IsTerm(Right * right)
{
    return right->tokenMatcher != nullptr;
}

struct Grammer
{
    static void Init()
    {
        if (rules.empty())
        {
            auto b = FromLeftRef("B");
            auto c = FromLeftRef("C");
            auto d = FromLeftRef("D");
            b->next = c; c->next = d;

            rules["A"] = b;
            rules["B"] = FromTokenMatcher(TokenMatcher("b"));
            rules["C"] = FromTokenMatcher(TokenMatcher("c"));
            rules["D"] = FromTokenMatcher(TokenMatcher("d"));
        }
    }
    static Right * Expand(Left left)
    {
        Init();
        return rules.at(left);
    }

    static std::map<Left, Right *> rules;
};
std::map<Left, Right *> Grammer::rules;

// ==== Non-Term ====

bool First(Left left, TokenIterator & ti)
{
    Right * right = Grammer::Expand(left);
    if (IsTerm(right))
    {
        return right->tokenMatcher->Match(ti.Peek());
    }
    else
    {
        return First(*right->leftRef, ti);
    }
}
// First x (all alt + all with null left)
Right * Predict(Right * node, TokenIterator & ti)
{
    for (Right * n = node; n; n = n->alt)
    {
        if (First(*n->leftRef, ti))
            return n;
    }
    return nullptr;
}

// ==== API ====

struct Visitor
{
    void Before(Left left)
    {
        std::cout << "Before " << left.data() << std::endl;
    }
    void After(Left left)
    {
        std::cout << "After " << left.data() << std::endl;
    }
};

void Parse(Left left, Visitor & vi, TokenIterator & ti)
{
    vi.Before(left);
    
    Right * right = Grammer::Expand(left);
    if (IsTerm(right))
    {
        // Match terminal
        assert(right->tokenMatcher->Match(ti.Next()));
    }
    else
    {
        Right * next = right;
        while ((next = Predict(next, ti)) != nullptr)
        {
            Parse(*next->leftRef, vi, ti);
            next = next->next;
        }
    }
    vi.After(left);
}

// TODO: production construction API
// TODO: string -> production

void Test_ParserGen()
{
    Visitor vi;
    TokenIterator ti({"b", "c", "d"});
    Parse("A", vi, ti);
}
