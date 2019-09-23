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
    {
        tokens.push_back("");
    }

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
    virtual bool Match(const Token & token) = 0;
};
struct ExactMatcher : public TokenMatcher
{
    ExactMatcher(Token token)
        : testToken(token)
    {}

    bool Match(const Token & token) override
    {
        return testToken == token;
    }

    Token testToken;
};
struct IdMatcher : public TokenMatcher
{
    bool Match(const Token & token) override
    {
        for (char c : token)
        {
            if (c != '_' && !isalpha(c))
                return false;
        }
        return true;
    }
};
struct NumberMatcher : public TokenMatcher
{
    bool Match(const Token & token) override
    {
        for (char c : token)
        {
            if (!isdigit(c))
                return false;
        }
        return true;
    }
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
RightNode * FromTokenMatcher(TokenMatcher * tm)
{
    RightNode * node = new RightNode;
    node->leftRef = nullptr;
    node->tokenMatcher = tm;
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
Right Alt(Right r1, Right r2)
{
    RightNode * n = NewStructural();
    *r1.next = n;
    *r1.alt = r2.in;
    *r2.next = n;

    return { r1.in, &n->next, r2.alt };
}
Right Rep0(Right r)
{
    RightNode * n = NewStructural();
    n->alt = r.in;
    *r.next = n;

    return { n, &n->next, r.alt };
}
Right Rep1(Right right)
{
    RightNode * n = NewStructural();
    n->alt = right.in;
    *right.next = n;

    return { right.in, &n->next, right.alt };
}
Right Opt(Right r)
{
    RightNode * n = NewStructural();
    *r.next = n;
    *r.alt = n;

    return { r.in, &n->next, &n->alt };
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
        std::cout << indent.data() << "Before " << left.data() << std::endl;
        indent += "  ";
    }
    void After(Left left)
    {
        indent.pop_back();
        indent.pop_back();
        std::cout << indent.data() << "After " << left.data() << std::endl;
    }
    std::string indent;
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

void Test_ParserGen_ABCD()
{
    /*
        A -> BCD
        B -> b
        C -> c
        D -> d
    */
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
                            Node(FromLeftRef("C"))),
                            Node(FromLeftRef("D")));
                    rules["B"] = Node(FromTokenMatcher(new ExactMatcher("b")));
                    rules["C"] = Node(FromTokenMatcher(new ExactMatcher("c")));
                    rules["D"] = Node(FromTokenMatcher(new ExactMatcher("d")));
                }
                return rules.at(left);
            }

            std::map<Left, Right> rules;
        } abcd;

        Visitor vi;
        TokenIterator ti({ "b", "c", "d" });
        Parse("A", abcd, vi, ti);
    }

    /*
        A -> B|C|D
        B -> b
        C -> c
        D -> d
    */
    {
        class ABCDGrammer : public Grammer
        {
        public:
            Right Expand(Left left) override
            {
                if (rules.empty())
                {
                    rules["A"] =
                        Alt(Alt(
                            Node(FromLeftRef("B")),
                            Node(FromLeftRef("C"))),
                            Node(FromLeftRef("D")));
                    rules["B"] = Node(FromTokenMatcher(new ExactMatcher("b")));
                    rules["C"] = Node(FromTokenMatcher(new ExactMatcher("c")));
                    rules["D"] = Node(FromTokenMatcher(new ExactMatcher("d")));
                }
                return rules.at(left);
            }

            std::map<Left, Right> rules;
        } abcd;

        {
            Visitor vi;
            TokenIterator ti({ "b" });
            Parse("A", abcd, vi, ti);
        }
        {
            Visitor vi;
            TokenIterator ti({ "c" });
            Parse("A", abcd, vi, ti);
        }
        {
            Visitor vi;
            TokenIterator ti({ "d" });
            Parse("A", abcd, vi, ti);
        }
    }

    /*
        A -> B?C*D+
        B -> b
        C -> c
        D -> d
    */
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
                            Opt(Node(FromLeftRef("B"))),
                            Rep0(Node(FromLeftRef("C")))),
                            Rep1(Node(FromLeftRef("D"))));
                    rules["B"] = Node(FromTokenMatcher(new ExactMatcher("b")));
                    rules["C"] = Node(FromTokenMatcher(new ExactMatcher("c")));
                    rules["D"] = Node(FromTokenMatcher(new ExactMatcher("d")));
                }
                return rules.at(left);
            }

            std::map<Left, Right> rules;
        } abcd;

        {
            Visitor vi;
            TokenIterator ti({ "b", "c", "d" });
            Parse("A", abcd, vi, ti);
        }
        {
            Visitor vi;
            TokenIterator ti({ "c", "d" });
            Parse("A", abcd, vi, ti);
        }
        {
            Visitor vi;
            TokenIterator ti({ "b", "d" });
            Parse("A", abcd, vi, ti);
        }
        {
            Visitor vi;
            TokenIterator ti({ "b", "c", "c", "d" });
            Parse("A", abcd, vi, ti);
        }
        {
            Visitor vi;
            TokenIterator ti({ "b", "c", "d", "d" });
            Parse("A", abcd, vi, ti);
        }
    }
}

std::vector<Token> Tokenize(std::string str)
{
    std::vector<Token> tokens;
    Token t;
    for (char c : str)
    {
        if (isspace(c))
        {
            if (!t.empty())
            {
                tokens.emplace_back();
                std::swap(tokens.back(), t);
            }
        }
        else
        {
            t.push_back(c);
        }
    }
    if (!t.empty())
    {
        tokens.emplace_back();
        std::swap(tokens.back(), t);
    }
    return tokens;
}

void Test_ParserGen_Decl()
{
    /*
        declarator  := ( pointer )? ( identifier | '(' declarator ')' ) ( '[' [constant_expr] ']' | '(' ')' )*
        pointer     := ( '*' )+
        identifier  := '\w+'
        constant_expr := '\d+'
    */
    {
        class DeclGrammer : public Grammer
        {
        public:
            Right Expand(Left left) override
            {
                if (rules.empty())
                {
                    rules["declarator"] =
                        Concat(Concat(
                            Node(FromLeftRef("pointer")),
                            Alt(
                                Node(FromLeftRef("identifier")),
                                Concat(Concat(
                                    Node(FromLeftRef("LP")),
                                    Node(FromLeftRef("declarator"))),
                                    Node(FromLeftRef("RP"))))),
                            Rep0(
                                Alt(
                                    Concat(Concat(
                                        Node(FromLeftRef("LSB")),
                                        Node(FromLeftRef("constant_expr"))),
                                        Node(FromLeftRef("RSB"))),
                                    Concat(
                                        Node(FromLeftRef("LP")),
                                        Node(FromLeftRef("RP"))))));
                    rules["pointer"] =
                        Rep1(
                            Node(FromLeftRef("STAR")));
                    rules["identifier"] =
                        Node(FromTokenMatcher(new IdMatcher()));
                    rules["constant_expr"] =
                        Node(FromTokenMatcher(new NumberMatcher()));
                    rules["LP"] =
                        Node(FromTokenMatcher(new ExactMatcher("(")));
                    rules["RP"] =
                        Node(FromTokenMatcher(new ExactMatcher(")")));
                    rules["LSB"] =
                        Node(FromTokenMatcher(new ExactMatcher("[")));
                    rules["RSB"] =
                        Node(FromTokenMatcher(new ExactMatcher("]")));
                    rules["STAR"] =
                        Node(FromTokenMatcher(new ExactMatcher("*")));
                }
                return rules.at(left);
            }

            std::map<Left, Right> rules;
        } decl;

        Visitor vi;
        TokenIterator ti(Tokenize("* ( * p ) [ 3 ] [ 10 ]"));
        Parse("declarator", decl, vi, ti);
    }

}

void Test_ParserGen()
{
    Test_ParserGen_ABCD();
    Test_ParserGen_Decl();
}
