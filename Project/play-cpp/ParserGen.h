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

// ==== Token Iterator & Matcher ====

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
struct MultiExactMatcher : public TokenMatcher
{
    MultiExactMatcher(std::vector<Token> tokens)
        : testTokens(tokens)
    {}

    bool Match(const Token & token) override
    {
        for (const Token & testToken : testTokens)
        {
            if (testToken == token)
                return true;
        }
        return false;
    }

    std::vector<Token> testTokens;
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

// ==== Grammer ====

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
        default:
            if (right->next)
                result = FIRST(*right->next->leftRef, g, ti);
            if (!result && right->alt)
                result = FIRST(*right->alt->leftRef, g, ti);
            break;
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
RightNode * Predict(std::set<RightNode *> nodes, Grammer & g, TokenIterator & ti)
{
    RightNode * pred = nullptr;
    int count = 0;
    for (RightNode * n : nodes)
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
    void Before(Left left, const Token * token = nullptr)
    {
        std::cout << indent.data() << "Before " << left.data() << " " << (token ? token->data() : "") << std::endl;
        indent += "  ";
    }
    void After(Left left, const Token * token = nullptr)
    {
        indent.pop_back();
        indent.pop_back();
        std::cout << indent.data() << "After " << left.data() << " " << (token ? token->data() : "") << std::endl;
    }
    std::string indent;
};

void Parse(Left left, Grammer & g, Visitor & vi, TokenIterator & ti)
{
    // Left -> TokenMatcher
    // Left -> (LeftRef|Structural)*
    
    RightNode * right = g.Expand(left).in;
    RightNode::Type type = GetType(right);
    if (type == RightNode::TOKEN_MATCHER)
    {
        Token token = ti.Peek();
        vi.Before(left, &token);

        // Match terminal
        assert(right->tokenMatcher->Match(ti.Next()));
    
        vi.After(left, &token);
    }
    else
    {
        vi.Before(left);

        RightNode * next = right;
        while ((next = Predict(Closure(next), g, ti)) != nullptr)
        {
            Parse(*next->leftRef, g, vi, ti);
            next = next->next;
        }

        vi.After(left);
    }
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

    /*
        Right recursion

        A -> BA?
        B -> b
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
                        Concat(
                            Node(FromLeftRef("B")),
                            Opt(Node(FromLeftRef("A"))));
                    rules["B"] = Node(FromTokenMatcher(new ExactMatcher("b")));
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
            TokenIterator ti({ "b", "b", "b" });
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

void Test_ParserGen_Expr()
{
    /*
        TODO: cast_expr, sizeof(type_name)

        expr                    := comma_expr
        comma_expr              := assign_expr  (comma_op           assign_expr)*
        assign_expr             := cond_expr    (assign_op          cond_expr)*
        cond_expr               := or_expr      (cond_op            expr cond_op2 cond_expr)?
        or_expr                 := and_expr     (or_op              and_expr)*
        and_expr                := bit_or_expr  (and_op             bit_or_expr)*
        bit_or_expr             := bit_xor_expr (bit_or_op          bit_xor_expr)*
        bit_xor_expr            := bit_and_expr (bit_xor_op         bit_and_expr)*
        bit_and_expr            := eq_expr      (bit_and_op         bit_and_expr)*
        eq_expr                 := rel_expr     (eq_op              rel_expr)*
        rel_expr                := shift_expr   (rel_op             shift_expr)*
        shift_expr              := add_expr     (shift_op           add_expr)*
        add_expr                := mul_expr     (add_op             mul_expr)*
        mul_expr                := prefix_expr  (mul_op             prefix_expr)*
        prefix_expr             :=              (prefix_op)*        postfix_expr
        postfix_expr            := primary_expr (idx_op | arg_op | postfix_op | postfix_op2 identifier)*
        primary_expr            := identifier | '(' expr ')'

        comma_op                := ,
        assign_op               := = *= /= %= += -= <<= >>= &= ^= |=
        cond_op                 := ?
        cond_op2                := :
        or_op                   := ||
        and_op                  := &&
        bit_or_op               := |
        bit_xor_op              := ^
        bit_and_op              := &
        eq_op                   := == !=
        rel_op                  := < <= > >=
        shift_op                := << >>
        add_op                  := + -
        mul_op                  := * / %
        postfix_op              := ++ --
        postfix_op2             := . ->
        arg_begin               := (
        arg_end                 := )
        arg_op                  := arg_begin (assign_expr (comma_op assign_expr)*)? arg_end
        idx_begin               := [
        idx_end                 := ]
        idx_op                  := idx_begin expr idx_end
        prefix_op               := & * + - ~ ! ++ -- sizeof
        identifier              := '\w+'
    */
    class ExprGrammer : public Grammer
    {
    public:
        Right Expand(Left left) override
        {
            if (rules.empty())
            {
                rules["expr"] =         Node(FromLeftRef("comma_expr"));
                rules["comma_expr"] =   Concat(Node(FromLeftRef("assign_expr")),Rep0(Concat(Node(FromLeftRef("comma_op")),Node(FromLeftRef("assign_expr")))));
                rules["assign_expr"] =  Concat(Node(FromLeftRef("cond_expr")),Rep0(Concat(Node(FromLeftRef("assign_op")),Node(FromLeftRef("cond_expr")))));
                rules["cond_expr"] =    Concat(Node(FromLeftRef("or_expr")),Opt(Concat(Concat(Concat(Node(FromLeftRef("cond_op")),Node(FromLeftRef("expr"))),Node(FromLeftRef("cond_op2"))),Node(FromLeftRef("cond_expr")))));
                rules["or_expr"] =      Concat(Node(FromLeftRef("and_expr")),Rep0(Concat(Node(FromLeftRef("or_op")),Node(FromLeftRef("and_expr")))));
                rules["and_expr"] =     Concat(Node(FromLeftRef("bit_or_expr")),Rep0(Concat(Node(FromLeftRef("and_op")),Node(FromLeftRef("bit_or_expr")))));
                rules["bit_or_expr"] =  Concat(Node(FromLeftRef("bit_xor_expr")),Rep0(Concat(Node(FromLeftRef("bit_or_op")),Node(FromLeftRef("bit_xor_expr")))));
                rules["bit_xor_expr"] = Concat(Node(FromLeftRef("bit_and_expr")),Rep0(Concat(Node(FromLeftRef("bit_xor_op")),Node(FromLeftRef("bit_and_expr")))));
                rules["bit_and_expr"] = Concat(Node(FromLeftRef("eq_expr")),Rep0(Concat(Node(FromLeftRef("bit_and_op")),Node(FromLeftRef("bit_and_expr")))));
                rules["eq_expr"] =      Concat(Node(FromLeftRef("rel_expr")),Rep0(Concat(Node(FromLeftRef("eq_op")),Node(FromLeftRef("rel_expr")))));
                rules["rel_expr"] =     Concat(Node(FromLeftRef("shift_expr")),Rep0(Concat(Node(FromLeftRef("rel_op")),Node(FromLeftRef("shift_expr")))));
                rules["shift_expr"] =   Concat(Node(FromLeftRef("add_expr")),Rep0(Concat(Node(FromLeftRef("shift_op")),Node(FromLeftRef("add_expr")))));
                rules["add_expr"] =     Concat(Node(FromLeftRef("mul_expr")),Rep0(Concat(Node(FromLeftRef("add_op")),Node(FromLeftRef("mul_expr")))));
                rules["mul_expr"] =     Concat(Node(FromLeftRef("prefix_expr")),Rep0(Concat(Node(FromLeftRef("mul_op")),Node(FromLeftRef("prefix_expr")))));
                rules["prefix_expr"] =  Concat(Rep0(Node(FromLeftRef("prefix_op"))),Node(FromLeftRef("postfix_expr")));
                rules["postfix_expr"] = Concat(Node(FromLeftRef("primary_expr")),Rep0(Concat(Concat(Concat(Node(FromLeftRef("idx_op")),Node(FromLeftRef("arg_op"))),Node(FromLeftRef("postfix_op"))),Concat(Node(FromLeftRef("postfix_op2")),Node(FromLeftRef("identifier"))))));
                rules["primary_expr"] = Alt(Node(FromLeftRef("identifier")),Concat(Concat(Node(FromLeftRef("arg_begin")),Node(FromLeftRef("expr"))),Node(FromLeftRef("arg_end"))));

                rules["comma_op"] =     Node(FromTokenMatcher(new MultiExactMatcher(Tokenize(","))));
                rules["assign_op"] =    Node(FromTokenMatcher(new MultiExactMatcher(Tokenize("= *= /= %= += -= <<= >>= &= ^= |="))));
                rules["cond_op"] =      Node(FromTokenMatcher(new MultiExactMatcher(Tokenize("?"))));
                rules["cond_op2"] =     Node(FromTokenMatcher(new MultiExactMatcher(Tokenize(":"))));
                rules["or_op"] =        Node(FromTokenMatcher(new MultiExactMatcher(Tokenize("||"))));
                rules["and_op"] =       Node(FromTokenMatcher(new MultiExactMatcher(Tokenize("&&"))));
                rules["bit_or_op"] =    Node(FromTokenMatcher(new MultiExactMatcher(Tokenize("|"))));
                rules["bit_xor_op"] =   Node(FromTokenMatcher(new MultiExactMatcher(Tokenize("^"))));
                rules["bit_and_op"] =   Node(FromTokenMatcher(new MultiExactMatcher(Tokenize("&"))));
                rules["eq_op"] =        Node(FromTokenMatcher(new MultiExactMatcher(Tokenize("== !="))));
                rules["rel_op"] =       Node(FromTokenMatcher(new MultiExactMatcher(Tokenize("< <= > >="))));
                rules["shift_op"] =     Node(FromTokenMatcher(new MultiExactMatcher(Tokenize("<< >>"))));
                rules["add_op"] =       Node(FromTokenMatcher(new MultiExactMatcher(Tokenize("+ -"))));
                rules["mul_op"] =       Node(FromTokenMatcher(new MultiExactMatcher(Tokenize("* / %"))));
                rules["postfix_op"] =   Node(FromTokenMatcher(new MultiExactMatcher(Tokenize("++ --"))));
                rules["postfix_op2"] =  Node(FromTokenMatcher(new MultiExactMatcher(Tokenize(". ->"))));
                rules["arg_begin"] =    Node(FromTokenMatcher(new MultiExactMatcher(Tokenize("("))));
                rules["arg_end"] =      Node(FromTokenMatcher(new MultiExactMatcher(Tokenize(")"))));
                rules["idx_begin"] =    Node(FromTokenMatcher(new MultiExactMatcher(Tokenize("["))));
                rules["idx_end"] =      Node(FromTokenMatcher(new MultiExactMatcher(Tokenize("]"))));
                rules["prefix_op"] =    Node(FromTokenMatcher(new MultiExactMatcher(Tokenize("& * + - ~ ! ++ -- sizeof"))));
                rules["arg_op"] =       Concat(Concat(Node(FromLeftRef("arg_begin")),Opt(Concat(Node(FromLeftRef("assign_expr")),Rep0(Concat(Node(FromLeftRef("comma_op")),Node(FromLeftRef("assign_expr"))))))),Node(FromLeftRef("arg_end")));
                rules["idx_op"] =       Concat(Concat(Node(FromLeftRef("idx_begin")),Node(FromLeftRef("expr"))),Node(FromLeftRef("idx_end")));
                rules["identifier"] =   Node(FromTokenMatcher(new IdMatcher()));
            }
            return rules.at(left);
        }

        std::map<Left, Right> rules;
    } expr;

    {
        Visitor vi;
        TokenIterator ti(Tokenize("a , b"));
        Parse("expr", expr, vi, ti);
    }
    {
        Visitor vi;
        TokenIterator ti(Tokenize("++ a + ++ b"));
        Parse("expr", expr, vi, ti);
    }
    {
        Visitor vi;
        TokenIterator ti(Tokenize("a ? b : c ? d : e"));
        Parse("expr", expr, vi, ti);
    }
}

void Test_ParserGen()
{
    Test_ParserGen_ABCD();
    Test_ParserGen_Decl();
    Test_ParserGen_Expr();
}
