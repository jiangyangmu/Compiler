#include "tester.h"

#include "../common.h"
#include "../ir.h"
#include "../parser.h"

class IRTester : public Tester
{
   protected:
    virtual void setUp() {}
    virtual void shutDown() {}

    int executeProgramAndGetExitCode(const char *prog, std::string *msg = nullptr)
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

            ret = simu.run(code);
            if (msg)
            {
                *msg = prog;
                *msg += "\n" + code.toString();
            }
        }

        return ret;
    }
};

// TEST_F(IRTester, IRValueFactory)
// TEST_F(IRTester, IRObjectBuilder)

// expression
TEST_F(IRTester, BasicExpression)
{
    // comma
    EXPECT_EQ(49, executeProgramAndGetExitCode("int main() { return 2,3,11,49; }"));

    // assign
    EXPECT_EQ(
        49, executeProgramAndGetExitCode("int main() { int a; a = 49; return a; }"));

    // cond
    EXPECT_EQ(49, executeProgramAndGetExitCode(
                      "int main() { int a; a = 1 ? 49 : 22; return a; }"));
    EXPECT_EQ(49, executeProgramAndGetExitCode(
                      "int main() { int a; a = 0 ? 22 : 49; return a; }"));

    // logical
    // TODO: short-circuit
    std::string msg;
    EXPECT_EQ_PRINT(
        1, executeProgramAndGetExitCode("int main() { return 49 || 0; }", &msg), msg);
    EXPECT_EQ_PRINT(
        1, executeProgramAndGetExitCode("int main() { return 0 || 49; }", &msg), msg);
    EXPECT_EQ_PRINT(
        0, executeProgramAndGetExitCode("int main() { return 0 || 0; }", &msg), msg);
    EXPECT_EQ_PRINT(
        1, executeProgramAndGetExitCode("int main() { return 49 && 49; }", &msg),
        msg);
    EXPECT_EQ_PRINT(
        0, executeProgramAndGetExitCode("int main() { return 0 && 49; }", &msg), msg);
    EXPECT_EQ_PRINT(
        0, executeProgramAndGetExitCode("int main() { return 49 && 0; }", &msg), msg);

    // bit-wise
    EXPECT_EQ_PRINT(
        7, executeProgramAndGetExitCode("int main() { return 5 | 3; }", &msg), msg);
    EXPECT_EQ_PRINT(
        1, executeProgramAndGetExitCode("int main() { return 5 & 3; }", &msg), msg);
    EXPECT_EQ_PRINT(
        6, executeProgramAndGetExitCode("int main() { return 5 ^ 3; }", &msg), msg);

    // equality
    EXPECT_EQ_PRINT(
        1, executeProgramAndGetExitCode("int main() { return 49 == 49; }", &msg), msg);
    EXPECT_EQ_PRINT(
        0, executeProgramAndGetExitCode("int main() { return 49 == 47; }", &msg), msg);
    EXPECT_EQ_PRINT(
        0, executeProgramAndGetExitCode("int main() { return 49 != 49; }", &msg), msg);
    EXPECT_EQ_PRINT(
        1, executeProgramAndGetExitCode("int main() { return 49 != 47; }", &msg), msg);

    // relation
    EXPECT_EQ_PRINT(
        0, executeProgramAndGetExitCode("int main() { return 49 < 47; }", &msg), msg);
    EXPECT_EQ_PRINT(
        0, executeProgramAndGetExitCode("int main() { return 49 <= 47; }", &msg), msg);
    EXPECT_EQ_PRINT(
        1, executeProgramAndGetExitCode("int main() { return 49 > 47; }", &msg), msg);
    EXPECT_EQ_PRINT(
        1, executeProgramAndGetExitCode("int main() { return 49 >= 47; }", &msg), msg);

    // shift
    EXPECT_EQ_PRINT(
        14, executeProgramAndGetExitCode("int main() { return 7 << 1; }", &msg), msg);
    EXPECT_EQ_PRINT(
        3, executeProgramAndGetExitCode("int main() { return 7 >> 1; }", &msg), msg);

    // additive
    EXPECT_EQ_PRINT(
        12, executeProgramAndGetExitCode("int main() { return 7 + 5; }", &msg), msg);
    EXPECT_EQ_PRINT(
        2, executeProgramAndGetExitCode("int main() { return 7 - 5; }", &msg), msg);

    // multiplicative
    EXPECT_EQ_PRINT(
        35, executeProgramAndGetExitCode("int main() { return 7 * 5; }", &msg), msg);
    EXPECT_EQ_PRINT(
        7, executeProgramAndGetExitCode("int main() { return 35 / 5; }", &msg), msg);
    EXPECT_EQ_PRINT(
        2, executeProgramAndGetExitCode("int main() { return 7 % 5; }", &msg), msg);

}
// statement
