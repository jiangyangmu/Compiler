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

// ==== Entity: Left, Right, Grammer ====

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

// Right node type
// * Left reference
// * Token matcher
// * Structural
struct RightNode
{
    enum Type { LEFT_REF, TOKEN_MATCHER, STRUCTURAL };

    Left * leftRef;
    TokenMatcher * tokenMatcher;

    RightNode * next;
    RightNode * alt;
};
struct Right
{
    RightNode * in;
    RightNode ** next;
    RightNode ** alt;
};
RightNode * FromLeftRef(Left left)
{
    RightNode * node = new RightNode;
    node->leftRef = new Left(left);
    node->tokenMatcher = nullptr;
    node->next = nullptr;
    node->alt = nullptr;
    return node;
}
RightNode * FromTokenMatcher(TokenMatcher tm)
{
    RightNode * node = new RightNode;
    node->leftRef = nullptr;
    node->tokenMatcher = new TokenMatcher(tm);
    node->next = nullptr;
    node->alt = nullptr;
    return node;
}
RightNode * NewStructural()
{
    RightNode * node = new RightNode;
    node->leftRef = nullptr;
    node->tokenMatcher = nullptr;
    node->next = nullptr;
    node->alt = nullptr;
    return node;
}
Right Node(RightNode * node)
{
    return { node, &node->next, &node->alt };
}
Right Concat(Right r1, Right r2)
{
    *r1.next = r2.in;

    return { r1.in, r2.next , r1.alt };
}
Right Rep1(Right right)
{
    RightNode * st = NewStructural();
    st->alt = right.in;
    *right.next = st;

    return { right.in, &st->next, right.alt };
}
RightNode::Type GetType(RightNode * node)
{
    if (node->leftRef)
        return RightNode::LEFT_REF;
    else if (node->tokenMatcher)
        return RightNode::TOKEN_MATCHER;
    else
        return RightNode::STRUCTURAL;
}

class Grammer
{
public:
    virtual Right Expand(Left left) = 0;
};

// ==== Parse Helper ====

bool FIRST(Left left, Grammer & g, TokenIterator & ti)
{
    // assert no left recursion
    RightNode * right = g.Expand(left).in;
    
    bool result = false;
    switch (GetType(right))
    {
        case RightNode::LEFT_REF:       result = FIRST(*right->leftRef, g, ti); break;
        case RightNode::TOKEN_MATCHER:  result = right->tokenMatcher->Match(ti.Peek()); break;
        default:                        assert(false); break;
    }
    return result;
}
std::set<RightNode *> Closure(RightNode * right)
{
    std::set<RightNode *> clo;

    std::vector<RightNode *> q = { right };
    std::set<RightNode *> visited = { nullptr };
    while (!q.empty())
    {
        RightNode * r = q.back();
        q.pop_back();

        if (visited.find(r) != visited.end())
            continue;
        else
            visited.insert(r);

        if (GetType(r) != RightNode::STRUCTURAL)
            clo.insert(r);

        if (GetType(r) == RightNode::STRUCTURAL)
            q.push_back(r->next);
        q.push_back(r->alt);
    }

    return clo;
}
// First x (all alt + all with null left)
RightNode * Predict(RightNode * node, Grammer & g, TokenIterator & ti)
{
    RightNode * pred = nullptr;
    int count = 0;
    for (RightNode * n : Closure(node))
    {
        if (FIRST(*n->leftRef, g, ti))
        {
            pred = n;
            ++count;
        }
    }
    assert(count <= 1);
    return pred;
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

void Parse(Left left, Grammer & g, Visitor & vi, TokenIterator & ti)
{
    vi.Before(left);
    
    RightNode * right = g.Expand(left).in;
    RightNode::Type type = GetType(right);
    if (type == RightNode::TOKEN_MATCHER)
    {
        // Match terminal
        assert(right->tokenMatcher->Match(ti.Next()));
    }
    else if (type == RightNode::LEFT_REF)
    {
        RightNode * next = right;
        while ((next = Predict(next, g, ti)) != nullptr)
        {
            Parse(*next->leftRef, g, vi, ti);
            next = next->next;
        }
    }
    else
    {
        assert(false);
    }
    vi.After(left);
}

void Test_ABCD()
{
    class ABCDGrammer : public Grammer
    {
    public:
        Right Expand(Left left) override
        {
            if (rules.empty())
            {
                rules["A"] =
                    Concat(Concat(
                        Node(FromLeftRef("B")),
                        Rep1(Node(FromLeftRef("C")))),
                        Node(FromLeftRef("D")));
                rules["B"] = Node(FromTokenMatcher(TokenMatcher("b")));
                rules["C"] = Node(FromTokenMatcher(TokenMatcher("c")));
                rules["D"] = Node(FromTokenMatcher(TokenMatcher("d")));
            }
            return rules.at(left);
        }

        std::map<Left, Right> rules;
    } abcd;

    Visitor vi;
    TokenIterator ti({ "b", "c","c","c","c", "d" });
    Parse("A", abcd, vi, ti);
}

void Test_ParserGen()
{
    Test_ABCD();
}
