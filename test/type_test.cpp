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
    EXPECT_EQ("int", parseProgram("int i;"));
}

TEST_F(TypeTester, Float)
{
    EXPECT_EQ("float", parseProgram("float f;"));
}

TEST_F(TypeTester, Pointer)
{
    EXPECT_EQ("pointer to int", parseProgram("int *p;"));
    EXPECT_EQ("pointer to pointer to int", parseProgram("int **p;"));
    EXPECT_EQ("pointer to pointer to pointer to int", parseProgram("int ***p;"));
}

TEST_F(TypeTester, Array)
{
    EXPECT_EQ("array of int", parseProgram("int a[3];"));
    EXPECT_EQ("array of array of int", parseProgram("int a[3][3];"));
    EXPECT_EQ("array of array of array of int", parseProgram("int a[3][3][3];"));
}

// TEST_F(TypeTester, Enum)
// {
//     EXPECT_EQ("enum Fruit", parseProgram("enum Fruit { apple, banana } fruit;"));
// }

