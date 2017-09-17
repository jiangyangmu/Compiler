#include "tester.h"

#include "../parser.h"
#include "../type.h"

class TypeTester : public Tester
{
    Lexer lex;

   protected:
    std::string parseProgram(const char *str)
    {
        StringBuf sb(str);
        lex.tokenize(sb);
        Environment *env = new Environment();
        sn_translation_unit *tu = sn_translation_unit::parse(lex);
        tu->visit(env, 0);

        std::string s;
        for (auto decl : tu->getChildren())
        {
            assert(decl->getChildrenCount() == 1);
            assert(decl->getFirstChild()->nodeType() == SN_DECLARATION);
            for (auto type :
                 dynamic_cast<sn_declaration *>(decl->getFirstChild())
                     ->type_infos_)
            {
                if (!s.empty())
                    s += ':';
                s += type ? type->toString() : "null";
            }
        }
        return s;
    }

   public:
    virtual void setUp() {}
    virtual void shutDown() {}
};

// -------------------- Declaration ---------------------

// TEST_F(TypeTester, Void)
// {
//     EXPECT_EQ("void", parseProgram("void v;"));
// }

TEST_F(TypeTester, Char)
{
    EXPECT_EQ("char", parseProgram("char c;"));
}

TEST_F(TypeTester, Int)
{
    EXPECT_EQ("unsigned char", parseProgram("unsigned char i;"));
    EXPECT_EQ("signed char", parseProgram("signed char i;"));

    EXPECT_EQ("short", parseProgram("short i;"));
    EXPECT_EQ("short int", parseProgram("short int i;"));
    EXPECT_EQ("signed short", parseProgram("signed short i;"));
    EXPECT_EQ("signed short int", parseProgram("signed short int i;"));
    EXPECT_EQ("unsigned short", parseProgram("unsigned short i;"));
    EXPECT_EQ("unsigned short int", parseProgram("unsigned short int i;"));

    // TODO: default int
    EXPECT_EQ("int", parseProgram("int i;"));
    EXPECT_EQ("signed", parseProgram("signed i;"));
    EXPECT_EQ("signed int", parseProgram("signed int i;"));
    EXPECT_EQ("unsigned", parseProgram("unsigned i;"));
    EXPECT_EQ("unsigned int", parseProgram("unsigned int i;"));

    EXPECT_EQ("long", parseProgram("long i;"));
    EXPECT_EQ("long int", parseProgram("long int i;"));
    EXPECT_EQ("signed long", parseProgram("signed long i;"));
    EXPECT_EQ("signed long int", parseProgram("signed long int i;"));
    EXPECT_EQ("unsigned long", parseProgram("unsigned long i;"));
    EXPECT_EQ("unsigned long int", parseProgram("unsigned long int i;"));
}

TEST_F(TypeTester, Float)
{
    EXPECT_EQ("float", parseProgram("float f;"));
    EXPECT_EQ("double", parseProgram("double f;"));
    EXPECT_EQ("long double", parseProgram("long double f;"));
}

// struct
TEST_F(TypeTester, Struct)
{
    // size, align, member offset
    EXPECT_EQ("struct SimpleStruct { int }",
              parseProgram("struct SimpleStruct { int i; } s;"));

    EXPECT_EQ("union SimpleUnion { int }",
              parseProgram("union SimpleUnion { int i; } s;"));

    EXPECT_EQ("enum SimpleEnum { HAHA(0),XIXI(1) }",
              parseProgram("enum SimpleEnum { HAHA, XIXI } e;"));
    EXPECT_EQ("enum EnumField { HAHA(3),XIXI(4) }",
              parseProgram("enum EnumField { HAHA = 3, XIXI } e;"));
}
// union
// enum
// TEST_F(TypeTester, Enum)
// {
//     EXPECT_EQ("enum Fruit", parseProgram("enum Fruit { apple, banana }
//     fruit;"));
// }
// typedef

TEST_F(TypeTester, Pointer)
{
    EXPECT_EQ("pointer to int", parseProgram("int *p;"));
    EXPECT_EQ("pointer to pointer to int", parseProgram("int **p;"));
    EXPECT_EQ("pointer to pointer to pointer to int",
              parseProgram("int ***p;"));
}

TEST_F(TypeTester, Array)
{
    EXPECT_EQ("array of int", parseProgram("int a[3];"));
    EXPECT_EQ("array of array of int", parseProgram("int a[3][3];"));
    EXPECT_EQ("array of array of array of int",
              parseProgram("int a[3][3][3];"));
}
