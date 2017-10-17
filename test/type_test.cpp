#include "tester.h"

#include "../parser.h"
#include "../type.h"

#include <queue>

class TypeTester : public Tester
{
    Lexer lex;

   protected:
    bool debug;
    Symbol *parseProgram(const char *str, const char *obj, int pass = 1)
    {
        StringBuf sb(str);
        lex.tokenize(sb);
        Environment *env = new Environment();
        sn_translation_unit *tu = sn_translation_unit::parse(lex);
        for (int p = 0; p < pass; ++p)
            tu->visit(env, p);
        if (debug)
            env->debugPrint();

        Symbol *s = nullptr;
        std::queue<Environment *> q;
        q.push(env);
        while (!q.empty() && s == nullptr)
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
                        s = symbol;
                        break;
                    }
                }
            }
        }
        return s;
    }
    Type *parseProgramAndGetType(const char *str, const char *obj, int pass = 1)
    {
        Symbol *s = parseProgram(str, obj, pass);
        return s ? s->type : nullptr;
    }
    std::string parseProgramAndGetTypeString(const char *str, const char *obj, int pass = 1)
    {
        Symbol *s = parseProgram(str, obj, pass);
        return (s && s->type) ? s->type->toString() : "<null>";
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

TEST_F(TypeTester, Void)
{
    EXPECT_EQ("pointer to void", parseProgramAndGetTypeString("void *v;", "v"));
}

TEST_F(TypeTester, Char)
{
    EXPECT_EQ("char", parseProgramAndGetTypeString("char c;", "c"));
}

TEST_F(TypeTester, Int)
{
    EXPECT_EQ("unsigned char", parseProgramAndGetTypeString("unsigned char i;", "i"));
    EXPECT_EQ("signed char", parseProgramAndGetTypeString("signed char i;", "i"));

    EXPECT_EQ("short", parseProgramAndGetTypeString("short i;", "i"));
    EXPECT_EQ("short int", parseProgramAndGetTypeString("short int i;", "i"));
    EXPECT_EQ("signed short", parseProgramAndGetTypeString("signed short i;", "i"));
    EXPECT_EQ("signed short int", parseProgramAndGetTypeString("signed short int i;", "i"));
    EXPECT_EQ("unsigned short", parseProgramAndGetTypeString("unsigned short i;", "i"));
    EXPECT_EQ("unsigned short int", parseProgramAndGetTypeString("unsigned short int i;", "i"));

    // TODO: default int
    EXPECT_EQ("int", parseProgramAndGetTypeString("int i;", "i"));
    EXPECT_EQ("signed", parseProgramAndGetTypeString("signed i;", "i"));
    EXPECT_EQ("signed int", parseProgramAndGetTypeString("signed int i;", "i"));
    EXPECT_EQ("unsigned", parseProgramAndGetTypeString("unsigned i;", "i"));
    EXPECT_EQ("unsigned int", parseProgramAndGetTypeString("unsigned int i;", "i"));

    EXPECT_EQ("long", parseProgramAndGetTypeString("long i;", "i"));
    EXPECT_EQ("long int", parseProgramAndGetTypeString("long int i;", "i"));
    EXPECT_EQ("signed long", parseProgramAndGetTypeString("signed long i;", "i"));
    EXPECT_EQ("signed long int", parseProgramAndGetTypeString("signed long int i;", "i"));
    EXPECT_EQ("unsigned long", parseProgramAndGetTypeString("unsigned long i;", "i"));
    EXPECT_EQ("unsigned long int", parseProgramAndGetTypeString("unsigned long int i;", "i"));
}

TEST_F(TypeTester, Float)
{
    EXPECT_EQ("float", parseProgramAndGetTypeString("float f;", "f"));
    EXPECT_EQ("double", parseProgramAndGetTypeString("double f;", "f"));
    EXPECT_EQ("long double", parseProgramAndGetTypeString("long double f;", "f"));
}

TEST_F(TypeTester, StructUnion)
{
    EXPECT_EQ("struct SimpleStruct { int }",
              parseProgramAndGetTypeString("struct SimpleStruct { int i; } s;", "s"));

    EXPECT_EQ("union SimpleUnion { int }",
              parseProgramAndGetTypeString("union SimpleUnion { int i; } s;", "s"));
}
TEST_F(TypeTester, Enum)
{
    // size, align, member offset
    EXPECT_EQ("enum SimpleEnum { HAHA(0),XIXI(1) }",
              parseProgramAndGetTypeString("enum SimpleEnum { HAHA, XIXI } e;", "e"));
    EXPECT_EQ("enum EnumField { HAHA(3),HEHE(4),XIXI(0),YIYI(1) }",
              parseProgramAndGetTypeString(
                  "enum EnumField { HAHA = 3, HEHE, XIXI = 0, YIYI } e;", "e"));
}
// typedef

TEST_F(TypeTester, Pointer)
{
    EXPECT_EQ("pointer to int", parseProgramAndGetTypeString("int *p;", "p"));
    EXPECT_EQ("pointer to pointer to int", parseProgramAndGetTypeString("int **p;", "p"));
    EXPECT_EQ("pointer to pointer to pointer to int",
              parseProgramAndGetTypeString("int ***p;", "p"));
}

TEST_F(TypeTester, Array)
{
    EXPECT_EQ("array of int", parseProgramAndGetTypeString("int a[3];", "a"));
    EXPECT_EQ("array of array of int", parseProgramAndGetTypeString("int a[3][3];", "a"));
    EXPECT_EQ("array of array of array of int",
              parseProgramAndGetTypeString("int a[3][3][3];", "a"));
}

TEST_F(TypeTester, Function)
{
    EXPECT_EQ("function returns void",
              parseProgramAndGetTypeString("void func() {}", "func", 2));
}

// TEST_F(TypeTester, Lvalue)
// {
// }

TEST_F(TypeTester, Const)
{
    EXPECT_EQ(true,
              parseProgramAndGetType("const char i;", "i")->isConst());
    EXPECT_EQ(true,
              parseProgramAndGetType("const int i;", "i")->isConst());
    EXPECT_EQ(true,
              parseProgramAndGetType("const float i;", "i")->isConst());
    EXPECT_EQ(true,
              parseProgramAndGetType("const struct ConstStruct { int i; } s;", "s")->isConst());
    EXPECT_EQ(true,
              parseProgramAndGetType("const enum ConstEnum { XIXI } s;", "s")->isConst());
}
