#include "testing/tester.h"

#include "parse/parser_api.h"
typedef ProductionBuilder PB;

class ParserTester : public Tester {
public:
    virtual void setUp() {
    }
    virtual void shutDown() {
    }
};

TEST_F(ParserTester, BuildSymbol) {
    GM_BEGIN(G);
    GM_ADD(G, a);
    GM_ADD(G, b);

    a = PB::SYM('A');
    b = PB::SYM('B');

    GM_END(G);
    EXPECT_EQ(a.DebugString(), "'A'");
    EXPECT_EQ(b.DebugString(), "'B'");
}

TEST_F(ParserTester, BuildAnd) {
    GM_BEGIN(G);
    GM_ADD(G, a);
    GM_ADD(G, b);
    GM_ADD(G, c);
    GM_ADD(G, d);

    a = PB::SYM('A');
    b = PB::SYM('B');
    c = PB::AND(a, b);
    d = PB::AND(a, PB::AND(b, c));

    GM_END(G);
    EXPECT_EQ(c.DebugString(), "(and a b)");
    EXPECT_EQ(d.DebugString(), "(and a (and b c))");
}

TEST_F(ParserTester, BuildOr) {
    GM_BEGIN(PL);
    GM_ADD(PL, a);
    GM_ADD(PL, b);
    GM_ADD(PL, c);

    a = PB::SYM('A');
    b = PB::SYM('B');
    c = PB::OR(a, b);

    GM_END(PL);
    EXPECT_EQ(c.DebugString(), "(or a b)");
}

TEST_F(ParserTester, BuildAndOr) {
    GM_BEGIN(PL);
    GM_ADD(PL, a);
    GM_ADD(PL, b);
    GM_ADD(PL, c);
    GM_ADD(PL, d);

    a = PB::SYM('A');
    b = PB::SYM('B');
    c = PB::SYM('C');
    d = PB::OR(PB::AND(a, b), PB::AND(c, b));

    GM_END(PL);
    EXPECT_EQ(d.DebugString(), "(or (and a b) (and c b))");
}

TEST_F(ParserTester, BuildOpt) {
    GM_BEGIN(PL);
    GM_ADD(PL, a);
    GM_ADD(PL, b);
    GM_ADD(PL, c);

    a = PB::SYM('A');
    b = PB::SYM('B');
    c = PB::AND(PB::OPT(a), b);

    GM_END(PL);
    EXPECT_EQ(c.DebugString(), "(and (opt a) b)");
}

TEST_F(ParserTester, BuildRep) {
    GM_BEGIN(PL);
    GM_ADD(PL, a);
    GM_ADD(PL, b);
    GM_ADD(PL, c);

    a = PB::SYM('a');
    b = PB::SYM('b');
    c = PB::AND(PB::REP(a), b);

    GM_END(PL);
    EXPECT_EQ(c.DebugString(), "(and (rep a) b)");
}

TEST_F(ParserTester, BuildComplex) {
    GM_BEGIN(PL);
    GM_ADD(PL, start);
    GM_ADD(PL, expr);
    GM_ADD(PL, term);
    GM_ADD(PL, fact);

    start = expr;
    expr = PB::AND(term, PB::REP(PB::AND(PB::SYM('*'), term)));
    term = PB::AND(fact, PB::REP(PB::AND(PB::SYM('+'), fact)));
    fact = PB::SYM('1');

    GM_END(PL);
    EXPECT_EQ(start.DebugString(), "expr");
    EXPECT_EQ(expr.DebugString(), "(and term (rep (and '*' term)))");
    EXPECT_EQ(term.DebugString(), "(and fact (rep (and '+' fact)))");
    EXPECT_EQ(fact.DebugString(), "'1'");
}

TEST_F(ParserTester, FirstSymbol) {
    GM_BEGIN(G);
    GM_ADD(G, a);

    a = PB::SYM('a');

    GM_END(G);
    EXPECT_EQ_SET(a->FIRST(), TokenSet({'a'}));
}

TEST_F(ParserTester, FirstAnd) {
    GM_BEGIN(G);
    GM_ADD(G, a);
    GM_ADD(G, b);
    GM_ADD(G, c);

    a = PB::SYM('a');
    b = PB::SYM('b');
    c = PB::AND(a, b);

    GM_END(G);
    EXPECT_EQ_SET(a->FIRST(), TokenSet({'a'}));
    EXPECT_EQ_SET(b->FIRST(), TokenSet({'b'}));
    EXPECT_EQ_SET(c->FIRST(), TokenSet({'a'}));
}

TEST_F(ParserTester, FirstOr) {
    GM_BEGIN(G);
    GM_ADD(G, a);
    GM_ADD(G, b);
    GM_ADD(G, c);

    a = PB::SYM('a');
    b = PB::SYM('b');
    c = PB::OR(a, b);

    GM_END(G);
    EXPECT_EQ_SET(a->FIRST(), TokenSet({'a'}));
    EXPECT_EQ_SET(b->FIRST(), TokenSet({'b'}));
    EXPECT_EQ_SET(c->FIRST(), TokenSet({'a', 'b'}));
}

TEST_F(ParserTester, FirstAndOr) {
    GM_BEGIN(G);
    GM_ADD(G, a);
    GM_ADD(G, b);
    GM_ADD(G, c);
    GM_ADD(G, d);

    a = PB::SYM('a');
    b = PB::SYM('b');
    c = PB::SYM('c');
    d = PB::OR(PB::AND(a, b), PB::AND(c, b));

    GM_END(G);
    EXPECT_EQ_SET(a->FIRST(), TokenSet({'a'}));
    EXPECT_EQ_SET(b->FIRST(), TokenSet({'b'}));
    EXPECT_EQ_SET(c->FIRST(), TokenSet({'c'}));
    EXPECT_EQ_SET(d->FIRST(), TokenSet({'a', 'c'}));
}

TEST_F(ParserTester, FirstOpt) {
    GM_BEGIN(G);
    GM_ADD(G, a);
    GM_ADD(G, b);
    GM_ADD(G, c);

    a = PB::SYM('a');
    b = PB::SYM('b');
    c = PB::AND(PB::OPT(a), b);

    GM_END(G);
    EXPECT_EQ_SET(a->FIRST(), TokenSet({'a'}));
    EXPECT_EQ_SET(b->FIRST(), TokenSet({'b'}));
    EXPECT_EQ_SET(c->FIRST(), TokenSet({'a', 'b'}));
}

TEST_F(ParserTester, FirstRep) {
    GM_BEGIN(G);
    GM_ADD(G, a);
    GM_ADD(G, b);
    GM_ADD(G, c);

    a = PB::SYM('a');
    b = PB::SYM('b');
    c = PB::AND(PB::REP(a), b);

    GM_END(G);
    EXPECT_EQ_SET(a->FIRST(), TokenSet({'a'}));
    EXPECT_EQ_SET(b->FIRST(), TokenSet({'b'}));
    EXPECT_EQ_SET(c->FIRST(), TokenSet({'a', 'b'}));
}

TEST_F(ParserTester, FirstComplex) {
    GM_BEGIN(G);
    GM_ADD(G, start);
    GM_ADD(G, expr);
    GM_ADD(G, term);
    GM_ADD(G, fact);

    start = expr;
    expr = PB::AND(term, PB::REP(PB::AND(PB::SYM('*'), term)));
    term = PB::AND(fact, PB::REP(PB::AND(PB::SYM('+'), fact)));
    fact = PB::SYM('1');

    GM_END(G);
    EXPECT_EQ_SET(start->FIRST(), TokenSet({'1'}));
    EXPECT_EQ_SET(expr->FIRST(), TokenSet({'1'}));
    EXPECT_EQ_SET(term->FIRST(), TokenSet({'1'}));
    EXPECT_EQ_SET(fact->FIRST(), TokenSet({'1'}));
}

TEST_F(ParserTester, FollowSymbol) {
    GM_BEGIN(PL);
    GM_ADD(PL, a);

    a = PB::SYM('a');

    GM_END(PL);
    EXPECT_EQ_SET(a->FOLLOW(), TokenSet({}));
}

TEST_F(ParserTester, FollowAnd) {
    GM_BEGIN(PL);
    GM_ADD(PL, a);
    GM_ADD(PL, b);
    GM_ADD(PL, c);

    a = PB::SYM('a');
    b = PB::SYM('b');
    c = PB::AND(a, b);

    GM_END(PL);
    EXPECT_EQ_SET(a->FOLLOW(), TokenSet({'b'}));
    EXPECT_EQ_SET(b->FOLLOW(), TokenSet({}));
    EXPECT_EQ_SET(c->FOLLOW(), TokenSet({}));
}

TEST_F(ParserTester, FollowOr) {
    GM_BEGIN(PL);
    GM_ADD(PL, a);
    GM_ADD(PL, b);
    GM_ADD(PL, c);

    a = PB::SYM('a');
    b = PB::SYM('b');
    c = PB::OR(a, b);

    GM_END(PL);
    EXPECT_EQ_SET(a->FOLLOW(), TokenSet({}));
    EXPECT_EQ_SET(b->FOLLOW(), TokenSet({}));
    EXPECT_EQ_SET(c->FOLLOW(), TokenSet({}));
}

TEST_F(ParserTester, FollowAndOr) {
    GM_BEGIN(PL);
    GM_ADD(PL, a);
    GM_ADD(PL, b);
    GM_ADD(PL, c);
    GM_ADD(PL, d);
    GM_ADD(PL, e);
    GM_ADD(PL, f);

    a = PB::SYM('a');
    b = PB::SYM('b');
    c = PB::SYM('c');
    d = PB::OR(PB::AND(a, b), c);
    e = PB::SYM('e');
    f = PB::AND(d, e, c);

    GM_END(PL);
    EXPECT_EQ_SET(a->FOLLOW(), TokenSet({'b'}));
    EXPECT_EQ_SET(b->FOLLOW(), TokenSet({'e'}));
    EXPECT_EQ_SET(c->FOLLOW(), TokenSet({'e'}));
    EXPECT_EQ_SET(d->FOLLOW(), TokenSet({'e'}));
    EXPECT_EQ_SET(e->FOLLOW(), TokenSet({'c'}));
    EXPECT_EQ_SET(f->FOLLOW(), TokenSet({}));
}

TEST_F(ParserTester, FollowOpt) {
    GM_BEGIN(PL);
    GM_ADD(PL, a);
    GM_ADD(PL, b);
    GM_ADD(PL, c);
    GM_ADD(PL, d);

    a = PB::SYM('a');
    b = PB::SYM('b');
    c = PB::SYM('c');
    d = PB::AND(a, PB::OPT(b), c);

    GM_END(PL);
    EXPECT_EQ_SET(a->FOLLOW(), TokenSet({'b', 'c'}));
    EXPECT_EQ_SET(b->FOLLOW(), TokenSet({'c'}));
    EXPECT_EQ_SET(c->FOLLOW(), TokenSet({}));
    EXPECT_EQ_SET(d->FOLLOW(), TokenSet({}));
}

TEST_F(ParserTester, FollowRep) {
    GM_BEGIN(PL);
    GM_ADD(PL, a);
    GM_ADD(PL, b);
    GM_ADD(PL, c);
    GM_ADD(PL, d);

    a = PB::SYM('a');
    b = PB::SYM('b');
    c = PB::SYM('c');
    d = PB::AND(a, PB::REP(b), c);

    GM_END(PL);
    EXPECT_EQ_SET(a->FOLLOW(), TokenSet({'b', 'c'}));
    EXPECT_EQ_SET(b->FOLLOW(), TokenSet({'c'}));
    EXPECT_EQ_SET(c->FOLLOW(), TokenSet({}));
    EXPECT_EQ_SET(d->FOLLOW(), TokenSet({}));
}

TEST_F(ParserTester, FollowComplex) {
    GM_BEGIN(PL);
    GM_ADD(PL, start);
    GM_ADD(PL, expr);
    GM_ADD(PL, term);
    GM_ADD(PL, fact);

    start = expr;
    expr = PB::AND(term, PB::REP(PB::AND(PB::SYM('*'), term)));
    term = PB::AND(fact, PB::REP(PB::AND(PB::SYM('+'), fact)));
    fact = PB::SYM('1');

    GM_END(PL);
    EXPECT_EQ_SET(start->FOLLOW(), TokenSet({}));
    EXPECT_EQ_SET(expr->FOLLOW(), TokenSet({}));
    EXPECT_EQ_SET(term->FOLLOW(), TokenSet({'*'}));
    EXPECT_EQ_SET(fact->FOLLOW(), TokenSet({'*', '+'}));
}
