#include "../test/tester.h"

#include "bnf.h"

class BNFTester : public Tester
{
   public:
    virtual void setUp() {}
    virtual void shutDown() {}
};

TEST_F(BNFTester, First)
{
    // node
    {
        BNF_RESET();
        N a;
        a = 'a';
        BNF_REGISTER_1(a);
        EXPECT_EQ_SET(a.first, TokenSet({'a'}));
    }

    // and
    {
        BNF_RESET();
        N a, b, c;
        a = 'a';
        b = 'b';
        c = a & b;
        BNF_REGISTER_3(a, b, c);
        EXPECT_EQ_SET(a.first, TokenSet({'a'}));
        EXPECT_EQ_SET(b.first, TokenSet({'b'}));
        EXPECT_EQ_SET(c.first, TokenSet({'a'}));
    }

    // or
    {
        BNF_RESET();
        N a, b, c;
        a = 'a';
        b = 'b';
        c = a | b;
        BNF_REGISTER_3(a, b, c);
        EXPECT_EQ_SET(a.first, TokenSet({'a'}));
        EXPECT_EQ_SET(b.first, TokenSet({'b'}));
        EXPECT_EQ_SET(c.first, TokenSet({'a', 'b'}));
    }

    // and + or
    {
        BNF_RESET();
        N a, b, c, d;
        a = 'a';
        b = 'b';
        c = 'c';
        d = a & b | c & b;
        BNF_REGISTER_4(a, b, c, d);
        EXPECT_EQ_SET(a.first, TokenSet({'a'}));
        EXPECT_EQ_SET(b.first, TokenSet({'b'}));
        EXPECT_EQ_SET(c.first, TokenSet({'c'}));
        EXPECT_EQ_SET(d.first, TokenSet({'a', 'c'}));
    }

    // epsilon
    {
        BNF_RESET();
        N a, b, c, d;
        a = 'a';
        b = 'b';
        c = EPSILON;
        d = c & a | b;
        BNF_REGISTER_4(a, b, c, d);
        EXPECT_EQ_SET(a.first, TokenSet({'a'}));
        EXPECT_EQ_SET(b.first, TokenSet({'b'}));
        EXPECT_EQ_SET(c.first, TokenSet({'a'}));
        EXPECT_EQ_SET(d.first, TokenSet({'a', 'b'}));
    }

    // non-left recursion
    {
        BNF_RESET();
        N a, b, c, d;
        a = 'a';
        b = 'b';
        c = EPSILON | b & c;
        d = a & c;
        BNF_REGISTER_4(a, b, c, d);
        EXPECT_EQ_SET(a.first, TokenSet({'a'}));
        EXPECT_EQ_SET(b.first, TokenSet({'b'}));
        EXPECT_EQ_SET(c.first, TokenSet({'b'}));
        EXPECT_EQ_SET(d.first, TokenSet({'a'}));
    }

    // complex
    {
        BNF_RESET();
        N fact, add, mul, term, term_tail, expr, expr_tail, start;
        fact = '1';
        add = '+';
        mul = '*';
        term = fact & term_tail;
        term_tail = EPSILON | add & fact & term_tail;
        expr = term & expr_tail;
        expr_tail = EPSILON | mul & term & expr_tail;
        start = expr;
        BNF_REGISTER_8(fact, add, mul, term, term_tail, expr, expr_tail, start);
        EXPECT_EQ_SET(fact.first, TokenSet({'1'}));
        EXPECT_EQ_SET(add.first, TokenSet({'+'}));
        EXPECT_EQ_SET(mul.first, TokenSet({'*'}));
        EXPECT_EQ_SET(term.first, TokenSet({'1'}));
        EXPECT_EQ_SET(term_tail.first, TokenSet({'+', '*'}));
        EXPECT_EQ_SET(expr.first, TokenSet({'1'}));
        EXPECT_EQ_SET(expr_tail.first, TokenSet({'*'}));
        EXPECT_EQ_SET(start.first, TokenSet({'1'}));
    }
}

TEST_F(BNFTester, Follow)
{
    // node
    {
        BNF_RESET();
        N a;
        a = 'a';
        BNF_REGISTER_1(a);
        EXPECT_EQ_SET(a.follow, TokenSet({}));
    }

    // and
    {
        BNF_RESET();
        N a, b, c;
        a = 'a';
        b = 'b';
        c = a & b;
        BNF_REGISTER_3(a, b, c);
        EXPECT_EQ_SET(a.follow, TokenSet({'b'}));
        EXPECT_EQ_SET(b.follow, TokenSet({}));
        EXPECT_EQ_SET(c.follow, TokenSet({}));
    }

    // or
    {
        BNF_RESET();
        N a, b, c;
        a = 'a';
        b = 'b';
        c = a | b;
        BNF_REGISTER_3(a, b, c);
        EXPECT_EQ_SET(a.follow, TokenSet({}));
        EXPECT_EQ_SET(b.follow, TokenSet({}));
        EXPECT_EQ_SET(c.follow, TokenSet({}));
    }

    // and + or
    {
        BNF_RESET();
        N a, b, c, d, e, f;
        a = 'a';
        b = 'b';
        c = 'c';
        d = a & b | c;
        e = 'e';
        f = d & e;
        BNF_REGISTER_6(a, b, c, d, e, f);
        EXPECT_EQ_SET(a.follow, TokenSet({'b'}));
        EXPECT_EQ_SET(b.follow, TokenSet({'e'}));
        EXPECT_EQ_SET(c.follow, TokenSet({'e'}));
        EXPECT_EQ_SET(d.follow, TokenSet({'e'}));
        EXPECT_EQ_SET(e.follow, TokenSet({}));
        EXPECT_EQ_SET(f.follow, TokenSet({}));
    }

    // epsilon
    {
        BNF_RESET();
        N a, b, c, d;
        a = 'a';
        b = 'b';
        c = EPSILON;
        d = a & c & b;
        BNF_REGISTER_4(a, b, c, d);
        EXPECT_EQ_SET(a.follow, TokenSet({'b'}));
        EXPECT_EQ_SET(b.follow, TokenSet({}));
        EXPECT_EQ_SET(c.follow, TokenSet({'b'}));
        EXPECT_EQ_SET(d.follow, TokenSet({}));
    }

    // non-left recursion
    {
        BNF_RESET();
        N a, b, c, d;
        a = 'a';
        b = 'b';
        c = EPSILON | b & c;
        d = a & c;
        BNF_REGISTER_4(a, b, c, d);
        EXPECT_EQ_SET(a.follow, TokenSet({'b'}));
        EXPECT_EQ_SET(b.follow, TokenSet({'b'}));
        EXPECT_EQ_SET(c.follow, TokenSet({}));
        EXPECT_EQ_SET(d.follow, TokenSet({}));
    }

    // follow
    {
        BNF_RESET();
        N fact, add, mul, term, term_tail, expr, expr_tail, start;
        fact = '1';
        add = '+';
        mul = '*';
        term = fact & term_tail;
        term_tail = EPSILON | add & fact & term_tail;
        expr = term & expr_tail;
        expr_tail = EPSILON | mul & term & expr_tail;
        start = expr;
        BNF_REGISTER_8(fact, add, mul, term, term_tail, expr, expr_tail, start);
        EXPECT_EQ_SET(fact.follow, TokenSet({'+', '*'}));
        EXPECT_EQ_SET(add.follow, TokenSet({'1'}));
        EXPECT_EQ_SET(mul.follow, TokenSet({'1'}));
        EXPECT_EQ_SET(term.follow, TokenSet({'*'}));
        EXPECT_EQ_SET(term_tail.follow, TokenSet({'*'}));
        EXPECT_EQ_SET(expr.follow, TokenSet({}));
        EXPECT_EQ_SET(expr_tail.follow, TokenSet({}));
        EXPECT_EQ_SET(start.follow, TokenSet({}));
    }
}

TEST_F(BNFTester, Flatten)
{
    ;
}

TEST_F(BNFTester, Parse)
{
    ;
}