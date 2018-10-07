#include "parse/parser_api.h"
#include "parse/GrammerDefinition.h"

std::ostream & operator<<(std::ostream & o, const TokenMatcher & t) {
    return o << t.toString();
}

#include "testing/tester.h"

TokenMatcher T(const char *text) {
    Token t;
    t.type = Token::ID;
    t.text = text;
    return TokenMatcher(t);
}

std::vector<TokenMatcher> TS(std::vector<TokenMatcher> tm) {
    return tm;
}

typedef ProductionBuilder PB;

TEST(ParserTest_BuildSymbol) {
    GM_BEGIN(G);
    GM_ADD(G, a);
    GM_ADD(G, b);

    a = PB::SYM(T("A"));
    b = PB::SYM(T("B"));

    GM_END(G);
    EXPECT_EQ(a.DebugString(), "'A'");
    EXPECT_EQ(b.DebugString(), "'B'");
}

TEST(ParserTest_BuildAnd) {
    GM_BEGIN(G);
    GM_ADD(G, a);
    GM_ADD(G, b);
    GM_ADD(G, c);
    GM_ADD(G, d);

    a = PB::SYM(T("A"));
    b = PB::SYM(T("B"));
    c = PB::AND(a, b);
    d = PB::AND(a, PB::AND(b, c));

    GM_END(G);
    EXPECT_EQ(c.DebugString(), "(and a b)");
    EXPECT_EQ(d.DebugString(), "(and a (and b c))");
}

TEST(ParserTest_BuildOr) {
    GM_BEGIN(PL);
    GM_ADD(PL, a);
    GM_ADD(PL, b);
    GM_ADD(PL, c);

    a = PB::SYM(T("A"));
    b = PB::SYM(T("B"));
    c = PB::OR(a, b);

    GM_END(PL);
    EXPECT_EQ(c.DebugString(), "(or a b)");
}

TEST(ParserTest_BuildAndOr) {
    GM_BEGIN(PL);
    GM_ADD(PL, a);
    GM_ADD(PL, b);
    GM_ADD(PL, c);
    GM_ADD(PL, d);

    a = PB::SYM(T("A"));
    b = PB::SYM(T("B"));
    c = PB::SYM(T("C"));
    d = PB::OR(PB::AND(a, b), PB::AND(c, b));

    GM_END(PL);
    EXPECT_EQ(d.DebugString(), "(or (and a b) (and c b))");
}

TEST(ParserTest_BuildOpt) {
    GM_BEGIN(PL);
    GM_ADD(PL, a);
    GM_ADD(PL, b);
    GM_ADD(PL, c);

    a = PB::SYM(T("A"));
    b = PB::SYM(T("B"));
    c = PB::AND(PB::OPT(a), b);

    GM_END(PL);
    EXPECT_EQ(c.DebugString(), "(and (opt a) b)");
}

TEST(ParserTest_BuildRep) {
    GM_BEGIN(PL);
    GM_ADD(PL, a);
    GM_ADD(PL, b);
    GM_ADD(PL, c);

    a = PB::SYM(T("a"));
    b = PB::SYM(T("b"));
    c = PB::AND(PB::REP(a), b);

    GM_END(PL);
    EXPECT_EQ(c.DebugString(), "(and (rep a) b)");
}

TEST(ParserTest_BuildComplex) {
    GM_BEGIN(PL);
    GM_ADD(PL, start);
    GM_ADD(PL, expr);
    GM_ADD(PL, term);
    GM_ADD(PL, fact);

    start = expr;
    expr = PB::AND(term, PB::REP(PB::AND(PB::SYM(T("*")), term)));
    term = PB::AND(fact, PB::REP(PB::AND(PB::SYM(T("+")), fact)));
    fact = PB::SYM(T("1"));

    GM_END(PL);
    EXPECT_EQ(start.DebugString(), "expr");
    EXPECT_EQ(expr.DebugString(), "(and term (rep (and '*' term)))");
    EXPECT_EQ(term.DebugString(), "(and fact (rep (and '+' fact)))");
    EXPECT_EQ(fact.DebugString(), "'1'");
}

TEST(ParserTest_FirstSymbol) {
    GM_BEGIN(G);
    GM_ADD(G, a);

    a = PB::SYM(T("a"));

    GM_END(G);
    EXPECT_EQ_SET(a->FIRST().matchers(), TS({T("a")}));
}

TEST(ParserTest_FirstAnd) {
    GM_BEGIN(G);
    GM_ADD(G, a);
    GM_ADD(G, b);
    GM_ADD(G, c);

    a = PB::SYM(T("a"));
    b = PB::SYM(T("b"));
    c = PB::AND(a, b);

    GM_END(G);
    EXPECT_EQ_SET(a->FIRST().matchers(), TS({T("a")}));
    EXPECT_EQ_SET(b->FIRST().matchers(), TS({T("b")}));
    EXPECT_EQ_SET(c->FIRST().matchers(), TS({T("a")}));
}

TEST(ParserTest_FirstOr) {
    GM_BEGIN(G);
    GM_ADD(G, a);
    GM_ADD(G, b);
    GM_ADD(G, c);

    a = PB::SYM(T("a"));
    b = PB::SYM(T("b"));
    c = PB::OR(a, b);

    GM_END(G);
    EXPECT_EQ_SET(a->FIRST().matchers(), TS({T("a")}));
    EXPECT_EQ_SET(b->FIRST().matchers(), TS({T("b")}));
    EXPECT_EQ_SET(c->FIRST().matchers(), TS({T("a"), T("b")}));
}

TEST(ParserTest_FirstAndOr) {
    GM_BEGIN(G);
    GM_ADD(G, a);
    GM_ADD(G, b);
    GM_ADD(G, c);
    GM_ADD(G, d);

    a = PB::SYM(T("a"));
    b = PB::SYM(T("b"));
    c = PB::SYM(T("c"));
    d = PB::OR(PB::AND(a, b), PB::AND(c, b));

    GM_END(G);
    EXPECT_EQ_SET(a->FIRST().matchers(), TS({T("a")}));
    EXPECT_EQ_SET(b->FIRST().matchers(), TS({T("b")}));
    EXPECT_EQ_SET(c->FIRST().matchers(), TS({T("c")}));
    EXPECT_EQ_SET(d->FIRST().matchers(), TS({T("a"), T("c")}));
}

TEST(ParserTest_FirstOpt) {
    GM_BEGIN(G);
    GM_ADD(G, a);
    GM_ADD(G, b);
    GM_ADD(G, c);

    a = PB::SYM(T("a"));
    b = PB::SYM(T("b"));
    c = PB::AND(PB::OPT(a), b);

    GM_END(G);
    EXPECT_EQ_SET(a->FIRST().matchers(), TS({T("a")}));
    EXPECT_EQ_SET(b->FIRST().matchers(), TS({T("b")}));
    EXPECT_EQ_SET(c->FIRST().matchers(), TS({T("a"), T("b")}));
}

TEST(ParserTest_FirstRep) {
    GM_BEGIN(G);
    GM_ADD(G, a);
    GM_ADD(G, b);
    GM_ADD(G, c);

    a = PB::SYM(T("a"));
    b = PB::SYM(T("b"));
    c = PB::AND(PB::REP(a), b);

    GM_END(G);
    EXPECT_EQ_SET(a->FIRST().matchers(), TS({T("a")}));
    EXPECT_EQ_SET(b->FIRST().matchers(), TS({T("b")}));
    EXPECT_EQ_SET(c->FIRST().matchers(), TS({T("a"), T("b")}));
}

TEST(ParserTest_FirstComplex) {
    GM_BEGIN(G);
    GM_ADD(G, start);
    GM_ADD(G, expr);
    GM_ADD(G, term);
    GM_ADD(G, fact);

    start = expr;
    expr = PB::AND(term, PB::REP(PB::AND(PB::SYM(T("*")), term)));
    term = PB::AND(fact, PB::REP(PB::AND(PB::SYM(T("+")), fact)));
    fact = PB::SYM(T("1"));

    GM_END(G);
    EXPECT_EQ_SET(start->FIRST().matchers(), TS({T("1")}));
    EXPECT_EQ_SET(expr->FIRST().matchers(), TS({T("1")}));
    EXPECT_EQ_SET(term->FIRST().matchers(), TS({T("1")}));
    EXPECT_EQ_SET(fact->FIRST().matchers(), TS({T("1")}));
}

TEST(ParserTest_FollowSymbol) {
    GM_BEGIN(PL);
    GM_ADD(PL, a);

    a = PB::SYM(T("a"));

    GM_END(PL);
    EXPECT_EQ_SET(a->FOLLOW().matchers(), TS({}));
}

TEST(ParserTest_FollowAnd) {
    GM_BEGIN(PL);
    GM_ADD(PL, a);
    GM_ADD(PL, b);
    GM_ADD(PL, c);

    a = PB::SYM(T("a"));
    b = PB::SYM(T("b"));
    c = PB::AND(a, b);

    GM_END(PL);
    EXPECT_EQ_SET(a->FOLLOW().matchers(), TS({T("b")}));
    EXPECT_EQ_SET(b->FOLLOW().matchers(), TS({}));
    EXPECT_EQ_SET(c->FOLLOW().matchers(), TS({}));
}

TEST(ParserTest_FollowOr) {
    GM_BEGIN(PL);
    GM_ADD(PL, a);
    GM_ADD(PL, b);
    GM_ADD(PL, c);

    a = PB::SYM(T("a"));
    b = PB::SYM(T("b"));
    c = PB::OR(a, b);

    GM_END(PL);
    EXPECT_EQ_SET(a->FOLLOW().matchers(), TS({}));
    EXPECT_EQ_SET(b->FOLLOW().matchers(), TS({}));
    EXPECT_EQ_SET(c->FOLLOW().matchers(), TS({}));
}

TEST(ParserTest_FollowAndOr) {
    GM_BEGIN(PL);
    GM_ADD(PL, a);
    GM_ADD(PL, b);
    GM_ADD(PL, c);
    GM_ADD(PL, d);
    GM_ADD(PL, e);
    GM_ADD(PL, f);

    a = PB::SYM(T("a"));
    b = PB::SYM(T("b"));
    c = PB::SYM(T("c"));
    d = PB::OR(PB::AND(a, b), c);
    e = PB::SYM(T("e"));
    f = PB::AND(d, e, c);

    GM_END(PL);
    EXPECT_EQ_SET(a->FOLLOW().matchers(), TS({T("b")}));
    EXPECT_EQ_SET(b->FOLLOW().matchers(), TS({T("e")}));
    EXPECT_EQ_SET(c->FOLLOW().matchers(), TS({T("e")}));
    EXPECT_EQ_SET(d->FOLLOW().matchers(), TS({T("e")}));
    EXPECT_EQ_SET(e->FOLLOW().matchers(), TS({T("c")}));
    EXPECT_EQ_SET(f->FOLLOW().matchers(), TS({}));
}

TEST(ParserTest_FollowOpt) {
    GM_BEGIN(PL);
    GM_ADD(PL, a);
    GM_ADD(PL, b);
    GM_ADD(PL, c);
    GM_ADD(PL, d);

    a = PB::SYM(T("a"));
    b = PB::SYM(T("b"));
    c = PB::SYM(T("c"));
    d = PB::AND(a, PB::OPT(b), c);

    GM_END(PL);
    EXPECT_EQ_SET(a->FOLLOW().matchers(), TS({T("b"), T("c")}));
    EXPECT_EQ_SET(b->FOLLOW().matchers(), TS({T("c")}));
    EXPECT_EQ_SET(c->FOLLOW().matchers(), TS({}));
    EXPECT_EQ_SET(d->FOLLOW().matchers(), TS({}));
}

TEST(ParserTest_FollowRep) {
    GM_BEGIN(PL);
    GM_ADD(PL, a);
    GM_ADD(PL, b);
    GM_ADD(PL, c);
    GM_ADD(PL, d);

    a = PB::SYM(T("a"));
    b = PB::SYM(T("b"));
    c = PB::SYM(T("c"));
    d = PB::AND(a, PB::REP(b), c);

    GM_END(PL);
    EXPECT_EQ_SET(a->FOLLOW().matchers(), TS({T("b"), T("c")}));
    EXPECT_EQ_SET(b->FOLLOW().matchers(), TS({T("c")}));
    EXPECT_EQ_SET(c->FOLLOW().matchers(), TS({}));
    EXPECT_EQ_SET(d->FOLLOW().matchers(), TS({}));
}

TEST(ParserTest_FollowComplex) {
    GM_BEGIN(PL);
    GM_ADD(PL, start);
    GM_ADD(PL, expr);
    GM_ADD(PL, term);
    GM_ADD(PL, fact);

    start = expr;
    expr = PB::AND(term, PB::REP(PB::AND(PB::SYM(T("*")), term)));
    term = PB::AND(fact, PB::REP(PB::AND(PB::SYM(T("+")), fact)));
    fact = PB::SYM(T("1"));

    GM_END(PL);
    EXPECT_EQ_SET(start->FOLLOW().matchers(), TS({}));
    EXPECT_EQ_SET(expr->FOLLOW().matchers(), TS({}));
    EXPECT_EQ_SET(term->FOLLOW().matchers(), TS({T("*")}));
    EXPECT_EQ_SET(fact->FOLLOW().matchers(), TS({T("*"), T("+")}));
}

class ParserTester : public Tester {
protected:
    ParserTester() : pG(nullptr) {}
    virtual void setUp()
    {
        if (pG == nullptr) {
            pG = new Grammer;
            *pG = CLanguageGrammer();
        }
    }
    void Test(std::string source, std::string ast) {
        auto tokens = Tokenize(StringRef(source.data()));
        TokenIterator ti(tokens);
        std::cout << pG->match(ti)->DebugString() << std::endl;
        std::cout << ast << std::endl;
    }
private:
    Grammer *pG;
};

TEST_F(ParserTester, CDecl) {
    Test("int a;", "");
}
