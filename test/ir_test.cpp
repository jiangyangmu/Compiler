#include "tester.h"

#include "../common.h"
#include "../ir.h"
#include "../parser.h"

class IRTester : public Tester
{
   protected:
    virtual void setUp()
    {
        debug = false;
    }
    virtual void shutDown() {}

    int executeProgramAndGetExitCode(const char *prog,
                                     std::string *msg = nullptr)
    {
        IRSimulator simu;
        int ret = 0;
        {
            IRCode code;

            Lexer lex;
            StringBuf *sb = new StringBuf(prog);
            lex.tokenize(*sb);

            ParserParams params;
            params.env = new Environment(new IRStorage());
            sn_translation_unit *tu = sn_translation_unit::parse(lex);
            for (int p = 0; p <= 2; ++p)
            {
                params.pass = p;
                tu->visit(params);
            }
            for (auto &o : params.env->getStorage()->get())
            {
                if (o.code != nullptr)
                    code.append(*o.code);
            }

            if (debug)
            {
                __debugPrint(tu->toString());
                std::cout << prog << std::endl;
                std::cout << code.toString() << std::endl;
            }
            if (msg)
            {
                *msg = prog;
                *msg += "\n" + code.toString();
                *msg += "------- Run --------\n";
            }
            ret = simu.run(code, msg);
            if (msg)
            {
                *msg += "---- End of Run ----\n";
            }
        }

        return ret;
    }

    bool debug;
};

// TEST_F(IRTester, IRValueFactory)
// TEST_F(IRTester, IRObjectBuilder)

// expression
TEST_F(IRTester, BasicExpression)
{
    // comma
    EXPECT_EQ(49,
              executeProgramAndGetExitCode("int main() { return 2,3,11,49; }"));

    // assign
    // TODO: more assignment form
    EXPECT_EQ(49, executeProgramAndGetExitCode(
                      "int main() { int a; a = 49; return a; }"));

    // cond
    EXPECT_EQ(49, executeProgramAndGetExitCode(
                      "int main() { int a; a = 1 ? 49 : 22; return a; }"));
    EXPECT_EQ(49, executeProgramAndGetExitCode(
                      "int main() { int a; a = 0 ? 22 : 49; return a; }"));

    // logical
    // TODO: short-circuit
    std::string msg;
    EXPECT_EQ_PRINT(
        1, executeProgramAndGetExitCode("int main() { return 49 || 0; }", &msg),
        msg);
    EXPECT_EQ_PRINT(
        1, executeProgramAndGetExitCode("int main() { return 0 || 49; }", &msg),
        msg);
    EXPECT_EQ_PRINT(
        0, executeProgramAndGetExitCode("int main() { return 0 || 0; }", &msg),
        msg);
    EXPECT_EQ_PRINT(
        1,
        executeProgramAndGetExitCode("int main() { return 49 && 49; }", &msg),
        msg);
    EXPECT_EQ_PRINT(
        0, executeProgramAndGetExitCode("int main() { return 0 && 49; }", &msg),
        msg);
    EXPECT_EQ_PRINT(
        0, executeProgramAndGetExitCode("int main() { return 49 && 0; }", &msg),
        msg);

    // bit-wise
    EXPECT_EQ_PRINT(
        7, executeProgramAndGetExitCode("int main() { return 5 | 3; }", &msg),
        msg);
    EXPECT_EQ_PRINT(
        1, executeProgramAndGetExitCode("int main() { return 5 & 3; }", &msg),
        msg);
    EXPECT_EQ_PRINT(
        6, executeProgramAndGetExitCode("int main() { return 5 ^ 3; }", &msg),
        msg);

    // equality
    EXPECT_EQ_PRINT(
        1,
        executeProgramAndGetExitCode("int main() { return 49 == 49; }", &msg),
        msg);
    EXPECT_EQ_PRINT(
        0,
        executeProgramAndGetExitCode("int main() { return 49 == 47; }", &msg),
        msg);
    EXPECT_EQ_PRINT(
        0,
        executeProgramAndGetExitCode("int main() { return 49 != 49; }", &msg),
        msg);
    EXPECT_EQ_PRINT(
        1,
        executeProgramAndGetExitCode("int main() { return 49 != 47; }", &msg),
        msg);

    // relation
    EXPECT_EQ_PRINT(
        0, executeProgramAndGetExitCode("int main() { return 49 < 47; }", &msg),
        msg);
    EXPECT_EQ_PRINT(
        0,
        executeProgramAndGetExitCode("int main() { return 49 <= 47; }", &msg),
        msg);
    EXPECT_EQ_PRINT(
        1, executeProgramAndGetExitCode("int main() { return 49 > 47; }", &msg),
        msg);
    EXPECT_EQ_PRINT(
        1,
        executeProgramAndGetExitCode("int main() { return 49 >= 47; }", &msg),
        msg);

    // shift
    EXPECT_EQ_PRINT(
        14, executeProgramAndGetExitCode("int main() { return 7 << 1; }", &msg),
        msg);
    EXPECT_EQ_PRINT(
        3, executeProgramAndGetExitCode("int main() { return 7 >> 1; }", &msg),
        msg);

    // additive
    EXPECT_EQ_PRINT(
        12, executeProgramAndGetExitCode("int main() { return 7 + 5; }", &msg),
        msg);
    EXPECT_EQ_PRINT(
        2, executeProgramAndGetExitCode("int main() { return 7 - 5; }", &msg),
        msg);

    // multiplicative
    EXPECT_EQ_PRINT(
        35, executeProgramAndGetExitCode("int main() { return 7 * 5; }", &msg),
        msg);
    EXPECT_EQ_PRINT(
        7, executeProgramAndGetExitCode("int main() { return 35 / 5; }", &msg),
        msg);
    EXPECT_EQ_PRINT(
        2, executeProgramAndGetExitCode("int main() { return 7 % 5; }", &msg),
        msg);
}

// statement
TEST_F(IRTester, Statement)
{
    std::string msg;
    // expression statement
    EXPECT_EQ_PRINT(1,
                    executeProgramAndGetExitCode(
                        "int main() { int a; ; a = 1; return a; }", &msg),
                    msg);
    // selection statement
    // if
    EXPECT_EQ_PRINT(
        1,
        executeProgramAndGetExitCode(
            "int main() { int a; a = 1; if (a < 1) a = 5; return a; }", &msg),
        msg);
    EXPECT_EQ_PRINT(
        5,
        executeProgramAndGetExitCode(
            "int main() { int a; a = 1; if (a >= 1) a = 5; return a; }", &msg),
        msg);
    // if-else
    EXPECT_EQ_PRINT(
        5,
        executeProgramAndGetExitCode("int main() { int a; a = 0; if (a < 1) a "
                                     "= 5; else a = 6; return a; }",
                                     &msg),
        msg);
    EXPECT_EQ_PRINT(
        6,
        executeProgramAndGetExitCode("int main() { int a; a = 1; if (a < 1) a "
                                     "= 5; else a = 6; return a; }",
                                     &msg),
        msg);
    // switch
    // case-break
    EXPECT_EQ_PRINT(1,
                    executeProgramAndGetExitCode(
                        "int main() { int a; a = 0; switch (a) { case 0: a = a + "
                        "1; break; default: a = a + 2; }  return a; }",
                        &msg),
                    msg);
    // case-no-break
    EXPECT_EQ_PRINT(3,
                    executeProgramAndGetExitCode(
                        "int main() { int a; a = 0; switch (a) { case 0: a = a + "
                        "1; default: a = a + 2; }  return a; }",
                        &msg),
                    msg);
    // default
    EXPECT_EQ_PRINT(12,
                    executeProgramAndGetExitCode(
                        "int main() { int a; a = 10; switch (a) { case 0: a = a + "
                        "1; break; default: a = a + 2; break; }  return a; }",
                        &msg),
                    msg);

    // iteration statement
    // while
    EXPECT_EQ_PRINT(
        5,
        executeProgramAndGetExitCode(
            "int main() { int a; a = 1; while (a < 5) a = a + 1; return a; }",
            &msg),
        msg);
    // do-while
    EXPECT_EQ_PRINT(
        5,
        executeProgramAndGetExitCode("int main() { int a; a = 1; do a = a + 1; "
                                     "while (a < 5); return a; }",
                                     &msg),
        msg);
    // for
    EXPECT_EQ_PRINT(
        5,
        executeProgramAndGetExitCode(
            "int main() { int a; for (a = 1; a < 5; a = a + 1) ; return a; }",
            &msg),
        msg);
    EXPECT_EQ_PRINT(
        5,
        executeProgramAndGetExitCode(
            "int main() { int a; a = 1; for (; a < 5; a = a + 1) ; return a; }",
            &msg),
        msg);
    EXPECT_EQ_PRINT(
        5,
        executeProgramAndGetExitCode(
            "int main() { int a; a = 1; for (; a < 5; ) a = a + 1; return a; }",
            &msg),
        msg);
    // nested loop
    EXPECT_EQ_PRINT(
        1000,
        executeProgramAndGetExitCode(
            "int main() { int a, b, c, d; d = 0; for (a = 1; a <= 10; ++a) for "
            "(b = 1; b <= 10; ++b) for (c = 1; c <= 10; ++c) ++d; return d; }",
            &msg),
        msg);

    // jump statement
    // goto
    EXPECT_EQ_PRINT(
        1,
        executeProgramAndGetExitCode(
            "int main() { int a; a = 1; goto L; a = 5; L: return a; }", &msg),
        msg);
    // continue
    EXPECT_EQ_PRINT(3,
                    executeProgramAndGetExitCode(
                        "int main() { int a; a = 1; while (a < 5) if (a < 3) a "
                        "= a + 1; else break; return a; }",
                        &msg),
                    msg);
    // break
    EXPECT_EQ_PRINT(
        103,
        executeProgramAndGetExitCode(
            "int main() { int a; for (a = 1; a < 5; a = a + 1) if (a < 2) "
            "continue; else a = a + 100; return a; }",
            &msg),
        msg);
    // return
    EXPECT_EQ_PRINT(5,
                    executeProgramAndGetExitCode(
                        "int main() { int a; a = 5; return a; }", &msg),
                    msg);
}
