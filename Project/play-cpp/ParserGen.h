#pragma once

#include <vector>
#include <map>
#include <set>
#include <cassert>
#include <iostream>

/*

    Problem 1: generate LL(1) parser
    Problem 2: generate LR(0) parser

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

// ==== RE ====

typedef std::string Left;

// Right node type
// * Left reference
// * Token matcher
// * Structural
struct RightNode
{
    enum Type { LEFT_REF, TOKEN_MATCHER, STRUCTURAL, DONE };

    // one of
    Left * leftRef;
    TokenMatcher * tokenMatcher;
    bool isDone;

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
    node->isDone = false;
    node->next = nullptr;
    node->alt = nullptr;
    return node;
}
RightNode * FromTokenMatcher(TokenMatcher * tm)
{
    RightNode * node = new RightNode;
    node->leftRef = nullptr;
    node->tokenMatcher = tm;
    node->isDone = false;
    node->next = nullptr;
    node->alt = nullptr;
    return node;
}
RightNode * NewStructural()
{
    RightNode * node = new RightNode;
    node->leftRef = nullptr;
    node->tokenMatcher = nullptr;
    node->isDone = false;
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
Right Done(Right r)
{
    RightNode * n = NewStructural();
    n->isDone = true;
    *r.next = n;

    return { r.in, &n->next, r.alt };
}
RightNode::Type GetType(RightNode * node)
{
    if (node->leftRef)
        return RightNode::LEFT_REF;
    else if (node->tokenMatcher)
        return RightNode::TOKEN_MATCHER;
    else
        return node->isDone ? RightNode::DONE : RightNode::STRUCTURAL;
}
bool IsDoneNode(RightNode * node)
{
    return node && node->isDone;
}

// ==== NFA ====

struct NfaState
{
    // NT|T
    Right value;
    NfaState * valueOut;
    NfaState * epsilonOut;
};
struct Nfa
{
    NfaState * in;
    NfaState ** valueOut;
    NfaState ** epsilonOut;
};

// ==== DFA ====

// ==== Grammar ====

class Grammar
{
public:
    virtual Right Expand(Left left) = 0;
};

// ==== LL Parse ====

struct Visitor
{
    void Before(Left  * left, const Token * token = nullptr)
    {
        std::cout << indent.data() << "Before";
        if (left)
            std::cout << " " << left->data();
        if (token)
            std::cout << " " << token->data();
        std::cout << std::endl;
        indent += "  ";
    }
    void After(Left * left, const Token * token = nullptr)
    {
        indent.pop_back();
        indent.pop_back();
        std::cout << indent.data() << "After";
        if (left)
            std::cout << " " << left->data();
        if (token)
            std::cout << " " << token->data();
        std::cout << std::endl;
    }
    std::string indent;
};

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

// Peek one token, find next node to match. Or done node. Or nullptr.
RightNode * LL1Predict(RightNode * node, Grammar & g, const Token & token)
{
    std::set<RightNode *> canMatchNodes;
    RightNode * doneNode = nullptr;

    for (RightNode * n : Closure(node))
    {
        switch (GetType(n))
        {
            case RightNode::LEFT_REF:
                if (LL1Predict(g.Expand(*n->leftRef).in, g, token))
                    canMatchNodes.insert(n);
                break;
            case RightNode::TOKEN_MATCHER:
                if (n->tokenMatcher->Match(token))
                    canMatchNodes.insert(n);
                break;
            case RightNode::DONE:
                doneNode = n;
                break;
            default:
                assert(false);
                break;
        }
    }
    assert(canMatchNodes.size() <= 1);
    return canMatchNodes.empty() ? doneNode : *canMatchNodes.begin();
}
void LL1Match(RightNode * right, Grammar & g, Visitor & vi, TokenIterator & ti)
{
    assert(right);

    RightNode * node = right;
    while (true)
    {
        node = LL1Predict(node, g, ti.Peek());
        assert(node);
        if (IsDoneNode(node))
            break;
        
        Token token;
        switch (GetType(node))
        {
            case RightNode::LEFT_REF:
                vi.Before(node->leftRef, nullptr);
                LL1Match(g.Expand(*node->leftRef).in, g, vi, ti);
                vi.After(node->leftRef, nullptr);
                break;
            case RightNode::TOKEN_MATCHER:
                token = ti.Next();
                vi.Before(nullptr, &token);
                assert(node->tokenMatcher->Match(token));
                vi.After(nullptr, &token);
                break;
            default:
                assert(false);
                break;
        }

        node = node->next;
    }
}

// ==== LR Parse ====

// TODO
// A. extensible RE module
// B. RE to NFA converter
// C. NFA to DFA converter
// D. simulate DFA

void LR0Parse(Left left, Grammar & g, Visitor & vi, TokenIterator & ti)
{
    // 1. List right segments
    // 2. Shift, try find right segment (compare token right-to-left), repeat until find.
}

// ==== API ====

void Parse(Left left, Grammar & g, Visitor & vi, TokenIterator & ti)
{
    vi.Before(&left, nullptr);
    LL1Match(g.Expand(left).in, g, vi, ti);
    vi.After(&left, nullptr);
}

// ==== Test ====

// Test helpers
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

void Test_ParserGen_ABCD()
{
    /*
        A -> BCD
        B -> b
        C -> c
        D -> d
    */
    {
        class ABCDGrammar : public Grammar
        {
        public:
            Right Expand(Left left) override
            {
                if (rules.empty())
                {
                    rules["A"] = Done(Concat(Concat(Node(FromLeftRef("B")),Node(FromLeftRef("C"))),Node(FromLeftRef("D"))));
                    rules["B"] = Done(Node(FromTokenMatcher(new ExactMatcher("b"))));
                    rules["C"] = Done(Node(FromTokenMatcher(new ExactMatcher("c"))));
                    rules["D"] = Done(Node(FromTokenMatcher(new ExactMatcher("d"))));
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
        class ABCDGrammar : public Grammar
        {
        public:
            Right Expand(Left left) override
            {
                if (rules.empty())
                {
                    rules["A"] = Done(Alt(Alt(Node(FromLeftRef("B")),Node(FromLeftRef("C"))),Node(FromLeftRef("D"))));
                    rules["B"] = Done(Node(FromTokenMatcher(new ExactMatcher("b"))));
                    rules["C"] = Done(Node(FromTokenMatcher(new ExactMatcher("c"))));
                    rules["D"] = Done(Node(FromTokenMatcher(new ExactMatcher("d"))));
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
        class ABCDGrammar : public Grammar
        {
        public:
            Right Expand(Left left) override
            {
                if (rules.empty())
                {
                    rules["A"] = Done(Concat(Concat(Opt(Node(FromLeftRef("B"))),Rep0(Node(FromLeftRef("C")))),Rep1(Node(FromLeftRef("D")))));
                    rules["B"] = Done(Node(FromTokenMatcher(new ExactMatcher("b"))));
                    rules["C"] = Done(Node(FromTokenMatcher(new ExactMatcher("c"))));
                    rules["D"] = Done(Node(FromTokenMatcher(new ExactMatcher("d"))));
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
        class ABCDGrammar : public Grammar
        {
        public:
            Right Expand(Left left) override
            {
                if (rules.empty())
                {
                    rules["A"] = Done(Concat(Node(FromLeftRef("B")),Opt(Node(FromLeftRef("A")))));
                    rules["B"] = Done(Node(FromTokenMatcher(new ExactMatcher("b"))));
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

void Test_ParserGen_Decl()
{
    /*
        declarator  := ( pointer )? ( identifier | '(' declarator ')' ) ( '[' [constant_expr] ']' | '(' ')' )*
        pointer     := ( '*' )+
        identifier  := '\w+'
        constant_expr := '\d+'
    */
    class DeclGrammar : public Grammar
    {
    public:
        Right Expand(Left left) override
        {
            if (rules.empty())
            {
                rules["declarator"] =
                    Done(Concat(Concat(
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
                                    Node(FromLeftRef("RP")))))));
                rules["pointer"] =
                    Done(Rep1(
                        Node(FromLeftRef("STAR"))));
                rules["identifier"] =
                    Done(Node(FromTokenMatcher(new IdMatcher())));
                rules["constant_expr"] =
                    Done(Node(FromTokenMatcher(new NumberMatcher())));
                rules["LP"] =
                    Done(Node(FromTokenMatcher(new ExactMatcher("("))));
                rules["RP"] =
                    Done(Node(FromTokenMatcher(new ExactMatcher(")"))));
                rules["LSB"] =
                    Done(Node(FromTokenMatcher(new ExactMatcher("["))));
                rules["RSB"] =
                    Done(Node(FromTokenMatcher(new ExactMatcher("]"))));
                rules["STAR"] =
                    Done(Node(FromTokenMatcher(new ExactMatcher("*"))));
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
    class ExprGrammar : public Grammar
    {
    public:
        Right Expand(Left left) override
        {
            if (rules.empty())
            {
                rules["expr"] =         Done(Node(FromLeftRef("comma_expr")));
                rules["comma_expr"] =   Done(Concat(Node(FromLeftRef("assign_expr")),Rep0(Concat(Node(FromLeftRef("comma_op")),Node(FromLeftRef("assign_expr"))))));
                rules["assign_expr"] =  Done(Concat(Node(FromLeftRef("cond_expr")),Rep0(Concat(Node(FromLeftRef("assign_op")),Node(FromLeftRef("cond_expr"))))));
                rules["cond_expr"] =    Done(Concat(Node(FromLeftRef("or_expr")),Opt(Concat(Concat(Concat(Node(FromLeftRef("cond_op")),Node(FromLeftRef("expr"))),Node(FromLeftRef("cond_op2"))),Node(FromLeftRef("cond_expr"))))));
                rules["or_expr"] =      Done(Concat(Node(FromLeftRef("and_expr")),Rep0(Concat(Node(FromLeftRef("or_op")),Node(FromLeftRef("and_expr"))))));
                rules["and_expr"] =     Done(Concat(Node(FromLeftRef("bit_or_expr")),Rep0(Concat(Node(FromLeftRef("and_op")),Node(FromLeftRef("bit_or_expr"))))));
                rules["bit_or_expr"] =  Done(Concat(Node(FromLeftRef("bit_xor_expr")),Rep0(Concat(Node(FromLeftRef("bit_or_op")),Node(FromLeftRef("bit_xor_expr"))))));
                rules["bit_xor_expr"] = Done(Concat(Node(FromLeftRef("bit_and_expr")),Rep0(Concat(Node(FromLeftRef("bit_xor_op")),Node(FromLeftRef("bit_and_expr"))))));
                rules["bit_and_expr"] = Done(Concat(Node(FromLeftRef("eq_expr")),Rep0(Concat(Node(FromLeftRef("bit_and_op")),Node(FromLeftRef("bit_and_expr"))))));
                rules["eq_expr"] =      Done(Concat(Node(FromLeftRef("rel_expr")),Rep0(Concat(Node(FromLeftRef("eq_op")),Node(FromLeftRef("rel_expr"))))));
                rules["rel_expr"] =     Done(Concat(Node(FromLeftRef("shift_expr")),Rep0(Concat(Node(FromLeftRef("rel_op")),Node(FromLeftRef("shift_expr"))))));
                rules["shift_expr"] =   Done(Concat(Node(FromLeftRef("add_expr")),Rep0(Concat(Node(FromLeftRef("shift_op")),Node(FromLeftRef("add_expr"))))));
                rules["add_expr"] =     Done(Concat(Node(FromLeftRef("mul_expr")),Rep0(Concat(Node(FromLeftRef("add_op")),Node(FromLeftRef("mul_expr"))))));
                rules["mul_expr"] =     Done(Concat(Node(FromLeftRef("prefix_expr")),Rep0(Concat(Node(FromLeftRef("mul_op")),Node(FromLeftRef("prefix_expr"))))));
                rules["prefix_expr"] =  Done(Concat(Rep0(Node(FromLeftRef("prefix_op"))),Node(FromLeftRef("postfix_expr"))));
                rules["postfix_expr"] = Done(Concat(Node(FromLeftRef("primary_expr")),Rep0(Concat(Concat(Concat(Node(FromLeftRef("idx_op")),Node(FromLeftRef("arg_op"))),Node(FromLeftRef("postfix_op"))),Concat(Node(FromLeftRef("postfix_op2")),Node(FromLeftRef("identifier")))))));
                rules["primary_expr"] = Done(Alt(Node(FromLeftRef("identifier")),Concat(Concat(Node(FromLeftRef("arg_begin")),Node(FromLeftRef("expr"))),Node(FromLeftRef("arg_end")))));
                rules["comma_op"] =     Done(Node(FromTokenMatcher(new MultiExactMatcher(Tokenize(",")))));
                rules["assign_op"] =    Done(Node(FromTokenMatcher(new MultiExactMatcher(Tokenize("= *= /= %= += -= <<= >>= &= ^= |=")))));
                rules["cond_op"] =      Done(Node(FromTokenMatcher(new MultiExactMatcher(Tokenize("?")))));
                rules["cond_op2"] =     Done(Node(FromTokenMatcher(new MultiExactMatcher(Tokenize(":")))));
                rules["or_op"] =        Done(Node(FromTokenMatcher(new MultiExactMatcher(Tokenize("||")))));
                rules["and_op"] =       Done(Node(FromTokenMatcher(new MultiExactMatcher(Tokenize("&&")))));
                rules["bit_or_op"] =    Done(Node(FromTokenMatcher(new MultiExactMatcher(Tokenize("|")))));
                rules["bit_xor_op"] =   Done(Node(FromTokenMatcher(new MultiExactMatcher(Tokenize("^")))));
                rules["bit_and_op"] =   Done(Node(FromTokenMatcher(new MultiExactMatcher(Tokenize("&")))));
                rules["eq_op"] =        Done(Node(FromTokenMatcher(new MultiExactMatcher(Tokenize("== !=")))));
                rules["rel_op"] =       Done(Node(FromTokenMatcher(new MultiExactMatcher(Tokenize("< <= > >=")))));
                rules["shift_op"] =     Done(Node(FromTokenMatcher(new MultiExactMatcher(Tokenize("<< >>")))));
                rules["add_op"] =       Done(Node(FromTokenMatcher(new MultiExactMatcher(Tokenize("+ -")))));
                rules["mul_op"] =       Done(Node(FromTokenMatcher(new MultiExactMatcher(Tokenize("* / %")))));
                rules["postfix_op"] =   Done(Node(FromTokenMatcher(new MultiExactMatcher(Tokenize("++ --")))));
                rules["postfix_op2"] =  Done(Node(FromTokenMatcher(new MultiExactMatcher(Tokenize(". ->")))));
                rules["arg_begin"] =    Done(Node(FromTokenMatcher(new MultiExactMatcher(Tokenize("(")))));
                rules["arg_end"] =      Done(Node(FromTokenMatcher(new MultiExactMatcher(Tokenize(")")))));
                rules["idx_begin"] =    Done(Node(FromTokenMatcher(new MultiExactMatcher(Tokenize("[")))));
                rules["idx_end"] =      Done(Node(FromTokenMatcher(new MultiExactMatcher(Tokenize("]")))));
                rules["prefix_op"] =    Done(Node(FromTokenMatcher(new MultiExactMatcher(Tokenize("& * + - ~ ! ++ -- sizeof")))));
                rules["arg_op"] =       Done(Concat(Concat(Node(FromLeftRef("arg_begin")),Opt(Concat(Node(FromLeftRef("assign_expr")),Rep0(Concat(Node(FromLeftRef("comma_op")),Node(FromLeftRef("assign_expr"))))))),Node(FromLeftRef("arg_end"))));
                rules["idx_op"] =       Done(Concat(Concat(Node(FromLeftRef("idx_begin")),Node(FromLeftRef("expr"))),Node(FromLeftRef("idx_end"))));
                rules["identifier"] =   Done(Node(FromTokenMatcher(new IdMatcher())));
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

void Test_ParserGen_Stmt()
{
    /*
        stmt                    := labeled_stmt | compound_stmt | selection_stmt | iteration_stmt | jump_stmt
        labeled_stmt            := ('default' | 'case' expr) ':' stmt
        compound_stmt           := '{' stmt* '}'
        selection_stmt          := 'if' '(' expr ')' stmt ('else' stmt)?
                                || 'switch' '(' expr ')' stmt
        iteration_stmt          := 'while' '(' expr ')' stmt
                                || 'do' stmt 'while' '(' expr ')' ';'
                                || 'for' '(' expr? ';' expr? ';' expr? ')' stmt
        jump_stmt               := ('continue' | 'break' | 'return' expr?) ';'
        expr                    := integer
        integer                 := '\d+'
    */
    class StmtGrammar : public Grammar
    {
    public:
        Right Expand(Left left) override
        {
            if (rules.empty())
            {
                rules["stmt"] = Done(
                    Alt(Alt(Alt(Alt(
                        Node(FromLeftRef("labeled_stmt")),
                        Node(FromLeftRef("compound_stmt"))),
                        Node(FromLeftRef("selection_stmt"))),
                        Node(FromLeftRef("iteration_stmt"))),
                        Node(FromLeftRef("jump_stmt"))));
                rules["labeled_stmt"] = Done(
                    Concat(
                        Concat(
                            Alt(
                                Node(FromTokenMatcher(new ExactMatcher("default"))),
                                Concat(
                                    Node(FromTokenMatcher(new ExactMatcher("case"))),
                                    Node(FromTokenMatcher(new NumberMatcher()))
                                )
                            ),
                            Node(FromTokenMatcher(new ExactMatcher(":")))
                        ),
                        Node(FromLeftRef("stmt"))
                    )
                );
                rules["compound_stmt"] = Done(
                    Concat(Concat(
                        Node(FromTokenMatcher(new ExactMatcher("{"))),
                        Rep0(Node(FromLeftRef("stmt")))),
                        Node(FromTokenMatcher(new ExactMatcher("}"))))
                );

                rules["selection_stmt"] = Done(
                    Concat(Concat(Concat(Concat(Concat(
                        Node(FromTokenMatcher(new ExactMatcher("if"))),
                        Node(FromTokenMatcher(new ExactMatcher("(")))),
                        Node(FromLeftRef("expr"))),
                        Node(FromTokenMatcher(new ExactMatcher(")")))),
                        Node(FromLeftRef("stmt"))),
                        Opt(
                        Concat(
                            Node(FromTokenMatcher(new ExactMatcher("else"))),
                            Node(FromLeftRef("stmt"))
                        )
                        )
                    )
                );
                rules["iteration_stmt"] = Done(
                    Alt(Alt(
                        Concat(Concat(Concat(Concat(
                            Node(FromTokenMatcher(new ExactMatcher("while"))),
                            Node(FromTokenMatcher(new ExactMatcher("(")))),
                            Node(FromLeftRef("expr"))),
                            Node(FromTokenMatcher(new ExactMatcher(")")))),
                            Node(FromLeftRef("stmt"))),
                        Concat(Concat(Concat(Concat(Concat(Concat(
                            Node(FromTokenMatcher(new ExactMatcher("do"))),
                            Node(FromLeftRef("stmt"))),
                            Node(FromTokenMatcher(new ExactMatcher("while")))),
                            Node(FromTokenMatcher(new ExactMatcher("(")))),
                            Node(FromLeftRef("expr"))),
                            Node(FromTokenMatcher(new ExactMatcher(")")))),
                            Node(FromTokenMatcher(new ExactMatcher(";"))))
                    ),
                        Concat(Concat(Concat(Concat(Concat(Concat(Concat(Concat(
                            Node(FromTokenMatcher(new ExactMatcher("for"))),
                            Node(FromTokenMatcher(new ExactMatcher("(")))),
                            Opt(Node(FromLeftRef("expr")))),
                            Node(FromTokenMatcher(new ExactMatcher(";")))),
                            Opt(Node(FromLeftRef("expr")))),
                            Node(FromTokenMatcher(new ExactMatcher(";")))),
                            Opt(Node(FromLeftRef("expr")))),
                            Node(FromTokenMatcher(new ExactMatcher(")")))),
                            Node(FromLeftRef("stmt")))
                    )
                );
                rules["jump_stmt"] = Done(
                    Concat(
                        Alt(Alt(
                            Node(FromTokenMatcher(new ExactMatcher("continue"))),
                            Node(FromTokenMatcher(new ExactMatcher("break")))),
                            Concat(
                                Node(FromTokenMatcher(new ExactMatcher("return"))),
                                Opt(Node(FromLeftRef("expr")))
                            )
                        ),
                        Node(FromTokenMatcher(new ExactMatcher(";")))
                    )
                );
                rules["expr"] = Done(Node(FromTokenMatcher(new NumberMatcher())));
            }
            return rules.at(left);
        }

        std::map<Left, Right> rules;
    } stmt;

    {
        Visitor vi;
        TokenIterator ti(Tokenize("{ do { if ( 1 ) { break ; } else { for ( ; 0 ; ) { } continue ; } } while ( 0 ) ; return 1 ; }"));
        Parse("stmt", stmt, vi, ti);
    }
}

void Test_ParserGen()
{
    Test_ParserGen_ABCD();
    Test_ParserGen_Decl();
    Test_ParserGen_Expr();
    Test_ParserGen_Stmt();
}
