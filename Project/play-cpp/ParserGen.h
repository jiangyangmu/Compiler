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

struct TokenIterator
{
    TokenIterator(std::string str)
        : s(str)
        , i(0)
    {}

    char Peek()
    {
        return s.at(i);
    }
    char Next()
    {
        return s.at(i++);
    }

    std::string s;
    size_t i;
};
typedef char Left;

struct Right
{
    Left left;
    Right * next;
    Right * alt;
};
Right * Node(Left left)
{
    Right * right = new Right;
    right->left = left;
    right->next = nullptr;
    right->alt = nullptr;
    return right;
}

struct Grammer
{
    static Right * ExpandNonTerm(Left left)
    {
        static std::map<Left, Right *> nonTerms;
        if (nonTerms.empty())
        {
            auto b = Node('B'); auto c = Node('C'); auto d = Node('D');
            b->next = c; c->next = d;
            nonTerms['A'] = b;
        }
        return nonTerms.at(left);
    }
    static char ExpandTerm(Left left)
    {
        static std::map<Left, char> terms;
        if (terms.empty())
        {
            terms['B'] = 'b';
            terms['C'] = 'c';
            terms['D'] = 'd';
        }
        return terms.at(left);
    }
    static bool IsTerm(Left left)
    {
        return 'B' <= left && left <= 'D';
    }
};

// ==== Non-Term ====
bool First(Left left, TokenIterator & ti)
{
    if (Grammer::IsTerm(left))
    {
        char term = Grammer::ExpandTerm(left);
        return term == ti.Peek();
    }
    else
    {
        Right * right = Grammer::ExpandNonTerm(left);
        return First(right->left, ti);
    }
}
// First x (all alt + all with null left)
Right * Predict(Right * node, TokenIterator & ti)
{
    for (Right * n = node; n; n = n->alt)
    {
        if (First(n->left, ti))
            return n;
    }
    return nullptr;
}

struct Visitor
{
    void Before(Left left)
    {
        std::cout << "Before " << left << std::endl;
    }
    void After(Left left)
{
    std::cout << "After " << left << std::endl;
}
};

void Parse(Left left, Visitor & vi, TokenIterator & ti)
{
    vi.Before(left);
    if (Grammer::IsTerm(left))
    {
        Left term = Grammer::ExpandTerm(left);
        // Match terminal
        assert(term == tolower(ti.Next()));
    }
    else
    {
        Right * right = Grammer::ExpandNonTerm(left);
    
        Right * next = right;
        while ((next = Predict(next, ti)) != nullptr)
        {
            Parse(next->left, vi, ti);
            next = next->next;
        }
    }
    vi.After(left);
}

void Test_ParserGen()
{
    Visitor vi;
    TokenIterator ti("bcd");
    Parse('A', vi, ti);
}
