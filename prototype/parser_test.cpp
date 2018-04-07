#include "../testing/tester.h"

#include "parser.h"
typedef ProductionBuilder PB;

class ParserTester : public Tester
{
   public:
    virtual void setUp() {}
    virtual void shutDown() {}
};

TEST_F(ParserTester, Build)
{
    // symbol
    {
        GM_BEGIN(PL);
        GM_ADD(PL, a);

        a = PB::SYM('a');

        GM_END(PL);
        EXPECT_EQ(a.DebugString(), "'a'");
    }

    // and
    {
        GM_BEGIN(PL);
        GM_ADD(PL, a);
        GM_ADD(PL, b);
        GM_ADD(PL, c);
        // GM_ADD(PL, d);

        a = PB::SYM('a');
        b = PB::SYM('b');
        c = PB::AND(a, b);
        // d = PB::AND(a, PB::AND(b, c));

        GM_END(PL);
        EXPECT_EQ(a.DebugString(), "'a'");
        EXPECT_EQ(b.DebugString(), "'b'");
        EXPECT_EQ(c.DebugString(), "(and SYM SYM)");
        // EXPECT_EQ(d.DebugString(), "(and a (and 'b' c))");
    }
}

TEST_F(ParserTester, First)
{
    /*
    // symbol
    {
        GM_BEGIN(PL);
        GM_ADD(PL, a);

        a = PB::SYM('a');

        GM_END(PL);
        EXPECT_EQ_SET(a.FIRST(), TokenSet({'a'}));
    }

    // and
    {
        GM_BEGIN(PL);
        GM_ADD(PL, a);
        GM_ADD(PL, b);
        GM_ADD(PL, c);

        a = PB::SYM('a');
        b = PB::SYM('b');
        c = PB::AND(a, b);

        GM_END(PL);
        EXPECT_EQ_SET(a.FIRST(), TokenSet({'a'}));
        EXPECT_EQ_SET(b.FIRST(), TokenSet({'b'}));
        EXPECT_EQ_SET(c.FIRST(), TokenSet({'a'}));
    }

    
    // or
    {
        GM_BEGIN(PL);
        GM_ADD(PL, a);
        GM_ADD(PL, b);
        GM_ADD(PL, c);

        a = PB::SYM('a');
        b = PB::SYM('b');
        c = PB::OR(
                PB::PROD(a),
                PB::PROD(b));

        GM_END(PL);
        EXPECT_EQ_SET(a.FIRST(), TokenSet({'a'}));
        EXPECT_EQ_SET(b.FIRST(), TokenSet({'b'}));
        EXPECT_EQ_SET(c.FIRST(), TokenSet({'a', 'b'}));
    }

    // and + or
    {
        GM_BEGIN(PL);
        GM_ADD(PL, a);
        GM_ADD(PL, b);
        GM_ADD(PL, c);
        GM_ADD(PL, d);

        a = PB::SYM('a');
        b = PB::SYM('b');
        c = PB::SYM('c');
        d = PB::OR(
                PB::AND(
                    PB::PROD(a),
                    PB::PROD(b)),
                PB::AND(
                    PB::PROD(c),
                    PB::PROD(b)));

        GM_END(PL);
        EXPECT_EQ_SET(a.FIRST(), TokenSet({'a'}));
        EXPECT_EQ_SET(b.FIRST(), TokenSet({'b'}));
        EXPECT_EQ_SET(c.FIRST(), TokenSet({'c'}));
        EXPECT_EQ_SET(d.FIRST(), TokenSet({'a', 'c'}));
    }

    // optional
    {
        GM_BEGIN(PL);
        GM_ADD(PL, a);
        GM_ADD(PL, b);
        GM_ADD(PL, c);

        a = PB::SYM('a');
        b = PB::SYM('b');
        c = PB::AND(
                PB::OPT(PB::PROD(a)),
                PB::PROD(b));

        GM_END(PL);
        EXPECT_EQ_SET(a.FIRST(), TokenSet({'a'}));
        EXPECT_EQ_SET(b.FIRST(), TokenSet({'b'}));
        EXPECT_EQ_SET(c.FIRST(), TokenSet({'a', 'b'}));
    }

    // repeat
    {
        GM_BEGIN(PL);
        GM_ADD(PL, a);
        GM_ADD(PL, b);
        GM_ADD(PL, c);

        a = PB::SYM('a');
        b = PB::SYM('b');
        c = PB::AND(
                PB::REP(PB::PROD(a)),
                PB::PROD(b));

        GM_END(PL);
        EXPECT_EQ_SET(a.FIRST(), TokenSet({'a'}));
        EXPECT_EQ_SET(b.FIRST(), TokenSet({'b'}));
        EXPECT_EQ_SET(c.FIRST(), TokenSet({'a', 'b'}));
    }

    // complex
    {
        GM_BEGIN(PL);
        GM_ADD(PL, start);
        GM_ADD(PL, expr);
        GM_ADD(PL, term);
        GM_ADD(PL, fact);

        start =
            PB::PROD(expr);
        expr =
            PB::AND(
                PB::PROD(term),
                PB::REP(
                    PB::AND(
                        PB::SYM('*'),
                        PB::PROD(term))));
        term =
            PB::AND(
                PB::PROD(fact),
                PB::REP(
                    PB::AND(
                        PB::SYM('+'),
                        PB::PROD(fact))));
        fact =
            PB::SYM('1');

        GM_END(PL);
        EXPECT_EQ_SET(start.FIRST(), TokenSet({'1'}));
        EXPECT_EQ_SET(expr.FIRST(), TokenSet({'1'}));
        EXPECT_EQ_SET(term.FIRST(), TokenSet({'1'}));
        EXPECT_EQ_SET(fact.FIRST(), TokenSet({'1'}));
    }
    */
}

/*
TEST_F(ParserTester, Follow)
{
    return;
    // symbol
    {
        GM_BEGIN(PL);
        GM_ADD(PL, a);

        a = PB::SYM('a');

        GM_END(PL);
        EXPECT_EQ_SET(a.FOLLOW(), TokenSet({}));
    }

    // and
    {
        GM_BEGIN(PL);
        GM_ADD(PL, a);
        GM_ADD(PL, b);
        GM_ADD(PL, c);

        a = PB::SYM('a');
        b = PB::SYM('b');
        c = PB::AND(
                PB::PROD(a),
                PB::PROD(b));

        GM_END(PL);
        EXPECT_EQ_SET(a.FOLLOW(), TokenSet({'b'}));
        EXPECT_EQ_SET(b.FOLLOW(), TokenSet({}));
        EXPECT_EQ_SET(c.FOLLOW(), TokenSet({}));
    }

    // or
    {
        GM_BEGIN(PL);
        GM_ADD(PL, a);
        GM_ADD(PL, b);
        GM_ADD(PL, c);

        a = PB::SYM('a');
        b = PB::SYM('b');
        c = PB::OR(
                PB::PROD(a),
                PB::PROD(b));

        GM_END(PL);
        EXPECT_EQ_SET(a.FOLLOW(), TokenSet({}));
        EXPECT_EQ_SET(b.FOLLOW(), TokenSet({}));
        EXPECT_EQ_SET(c.FOLLOW(), TokenSet({}));
    }

    // and + or
    {
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
        d = PB::OR(
                PB::AND(
                    PB::PROD(a),
                    PB::PROD(b)),
                PB::PROD(c));
        e = PB::SYM('e');
        f = PB::AND(
                PB::PROD(d),
                PB::PROD(e));

        GM_END(PL);
        EXPECT_EQ_SET(a.FOLLOW(), TokenSet({'b'}));
        EXPECT_EQ_SET(b.FOLLOW(), TokenSet({'e'}));
        EXPECT_EQ_SET(c.FOLLOW(), TokenSet({'e'}));
        EXPECT_EQ_SET(d.FOLLOW(), TokenSet({'e'}));
        EXPECT_EQ_SET(e.FOLLOW(), TokenSet({}));
        EXPECT_EQ_SET(f.FOLLOW(), TokenSet({}));
    }

    // optional
    {
        GM_BEGIN(PL);
        GM_ADD(PL, a);
        GM_ADD(PL, b);
        GM_ADD(PL, c);
        GM_ADD(PL, d);

        a = PB::SYM('a');
        b = PB::SYM('b');
        c = PB::SYM('c');
        d = PB::AND(
                PB::PROD(a),
                PB::OPT(PB::PROD(b)),
                PB::PROD(c));

        GM_END(PL);
        EXPECT_EQ_SET(a.FOLLOW(), TokenSet({'b', 'c'}));
        EXPECT_EQ_SET(b.FOLLOW(), TokenSet({'c'}));
        EXPECT_EQ_SET(c.FOLLOW(), TokenSet({}));
        EXPECT_EQ_SET(d.FOLLOW(), TokenSet({}));
    }

    // repeat
    {
        GM_BEGIN(PL);
        GM_ADD(PL, a);
        GM_ADD(PL, b);
        GM_ADD(PL, c);
        GM_ADD(PL, d);

        a = PB::SYM('a');
        b = PB::SYM('b');
        c = PB::SYM('c');
        d = PB::AND(
                PB::PROD(a),
                PB::REP(PB::PROD(b)),
                PB::PROD(c));

        GM_END(PL);
        EXPECT_EQ_SET(a.FOLLOW(), TokenSet({'b', 'c'}));
        EXPECT_EQ_SET(b.FOLLOW(), TokenSet({'c'}));
        EXPECT_EQ_SET(c.FOLLOW(), TokenSet({}));
        EXPECT_EQ_SET(d.FOLLOW(), TokenSet({}));
    }

    // complex
    {
        GM_BEGIN(PL);
        GM_ADD(PL, start);
        GM_ADD(PL, expr);
        GM_ADD(PL, term);
        GM_ADD(PL, fact);

        start =
            PB::PROD(expr);
        expr =
            PB::AND(
                PB::PROD(term),
                PB::REP(
                    PB::AND(
                        PB::SYM('*'),
                        PB::PROD(term))));
        term =
            PB::AND(
                PB::PROD(fact),
                PB::REP(
                    PB::AND(
                        PB::SYM('+'),
                        PB::PROD(fact))));
        fact =
            PB::SYM('1');

        GM_END(PL);
        EXPECT_EQ_SET(start.FOLLOW(), TokenSet({}));
        EXPECT_EQ_SET(expr.FOLLOW(), TokenSet({}));
        EXPECT_EQ_SET(term.FOLLOW(), TokenSet({'*'}));
        EXPECT_EQ_SET(fact.FOLLOW(), TokenSet({'*', '+'}));
    }
}
*/