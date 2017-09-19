#include "tester.h"

#include "../parser.h"
#include "../type.h"

#include <queue>

class TypeTester : public Tester
{
    Lexer lex;

   protected:
    bool debug;
    std::string parseProgram(const char *str, const char *obj, int pass = 1)
    {
        StringBuf sb(str);
        lex.tokenize(sb);
        Environment *env = new Environment();
        sn_translation_unit *tu = sn_translation_unit::parse(lex);
        for (int p = 0; p < pass; ++p)
            tu->visit(env, p);
        if (debug)
            env->debugPrint();

        std::string s;
        std::queue<Environment *> q;
        q.push(env);
        while (!q.empty() && s.empty())
        {
            Environment *curr = q.front();
            q.pop();
            for (auto child : curr->getChildren())
                q.push(child);

            for (auto symbol : curr->symbols)
            {
                if (symbol->type && symbol->space == SYMBOL_NAMESPACE_id &&
                    (symbol->type->isObject() || symbol->type->isFunction()))
                {
                    if (symbol->name == obj)
                    {
                        s += symbol->type->toString();
                        break;
                    }
                }
            }
        }
        return s;
    }

   public:
    virtual void setUp()
    {
        debug = false;
    }
    virtual void shutDown() {}
};

// -------------------- Declaration ---------------------

// TODO: class, size, align
// TODO: prop: completeness, constantness, volatile, lvalue

// TEST_F(TypeTester, Void)
// {
//     EXPECT_EQ("void", parseProgram("void v;"));
// }

TEST_F(TypeTester, Char)
{
    EXPECT_EQ("char", parseProgram("char c;", "c"));
}

TEST_F(TypeTester, Int)
{
    EXPECT_EQ("unsigned char", parseProgram("unsigned char i;", "i"));
    EXPECT_EQ("signed char", parseProgram("signed char i;", "i"));

    EXPECT_EQ("short", parseProgram("short i;", "i"));
    EXPECT_EQ("short int", parseProgram("short int i;", "i"));
    EXPECT_EQ("signed short", parseProgram("signed short i;", "i"));
    EXPECT_EQ("signed short int", parseProgram("signed short int i;", "i"));
    EXPECT_EQ("unsigned short", parseProgram("unsigned short i;", "i"));
    EXPECT_EQ("unsigned short int", parseProgram("unsigned short int i;", "i"));

    // TODO: default int
    EXPECT_EQ("int", parseProgram("int i;", "i"));
    EXPECT_EQ("signed", parseProgram("signed i;", "i"));
    EXPECT_EQ("signed int", parseProgram("signed int i;", "i"));
    EXPECT_EQ("unsigned", parseProgram("unsigned i;", "i"));
    EXPECT_EQ("unsigned int", parseProgram("unsigned int i;", "i"));

    EXPECT_EQ("long", parseProgram("long i;", "i"));
    EXPECT_EQ("long int", parseProgram("long int i;", "i"));
    EXPECT_EQ("signed long", parseProgram("signed long i;", "i"));
    EXPECT_EQ("signed long int", parseProgram("signed long int i;", "i"));
    EXPECT_EQ("unsigned long", parseProgram("unsigned long i;", "i"));
    EXPECT_EQ("unsigned long int", parseProgram("unsigned long int i;", "i"));
}

TEST_F(TypeTester, Float)
{
    EXPECT_EQ("float", parseProgram("float f;", "f"));
    EXPECT_EQ("double", parseProgram("double f;", "f"));
    EXPECT_EQ("long double", parseProgram("long double f;", "f"));
}

TEST_F(TypeTester, StructUnion)
{
    EXPECT_EQ("struct SimpleStruct { int }",
              parseProgram("struct SimpleStruct { int i; } s;", "s"));

    EXPECT_EQ("union SimpleUnion { int }",
              parseProgram("union SimpleUnion { int i; } s;", "s"));
}
TEST_F(TypeTester, Enum)
{
    // size, align, member offset
    EXPECT_EQ("enum SimpleEnum { HAHA(0),XIXI(1) }",
              parseProgram("enum SimpleEnum { HAHA, XIXI } e;", "e"));
    EXPECT_EQ("enum EnumField { HAHA(3),HEHE(4),XIXI(0),YIYI(1) }",
              parseProgram(
                  "enum EnumField { HAHA = 3, HEHE, XIXI = 0, YIYI } e;", "e"));
}
// typedef

TEST_F(TypeTester, Pointer)
{
    EXPECT_EQ("pointer to int", parseProgram("int *p;", "p"));
    EXPECT_EQ("pointer to pointer to int", parseProgram("int **p;", "p"));
    EXPECT_EQ("pointer to pointer to pointer to int",
              parseProgram("int ***p;", "p"));
}

TEST_F(TypeTester, Array)
{
    EXPECT_EQ("array of int", parseProgram("int a[3];", "a"));
    EXPECT_EQ("array of array of int", parseProgram("int a[3][3];", "a"));
    EXPECT_EQ("array of array of array of int",
              parseProgram("int a[3][3][3];", "a"));
}

TEST_F(TypeTester, Function)
{
    EXPECT_EQ("function returns void",
              parseProgram("void func() {}", "func", 2));
}
