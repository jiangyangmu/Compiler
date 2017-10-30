#include "tester.h"

#include "../parser.h"
#include "../type.h"
#include "../ir.h" // IRStorage

#include <queue>
#include <vector>
#include <unordered_map>

std::vector<std::string> SplitString(const char *s, char c)
{
    std::vector<std::string> result;
    const char *p = s, *q = s;
    while (*p != '\0')
    {
        while (*q != '\0' && *q != c)
            ++q;
        result.emplace_back(p, q - p);
        if (*q == c)
            ++q;
        p = q;
    }
    return result;
}

class TypeTester : public Tester
{
    Lexer lex;

   protected:
    bool debug;
    std::vector<Symbol *> parseProgram(const char *str, const char *obj,
                                       int pass = 1)
    {
        // Not recycle str, so when function ends, symbol info still there.
        StringBuf *sb = new StringBuf(str);
        lex.tokenize(*sb);
        Environment *env = new Environment(new IRStorage());
        sn_translation_unit *tu = sn_translation_unit::parse(lex);
        for (int p = 0; p < pass; ++p)
            tu->visit(env, p);
        if (debug)
            env->debugPrint();

        auto names = SplitString(obj, ',');
        std::vector<Symbol *> result;

        std::queue<Environment *> q;
        q.push(env);
        while (!q.empty())
        {
            Environment *curr = q.front();
            q.pop();
            for (auto child : curr->getChildren())
                q.push(child);

            for (auto symbol : curr->symbols)
            {
                if (symbol->type && symbol->space == SYMBOL_NAMESPACE_id)
                {
                    for (auto name : names)
                    {
                        if (symbol->name == name.data())
                        {
                            result.push_back(symbol);
                            break;
                        }
                    }
                }
            }
        }
        return result;
    }
    Type *parseProgramAndGetType(const char *str, const char *obj, int pass = 1)
    {
        auto objs = parseProgram(str, obj, pass);
        return !objs.empty() ? objs.front()->type : nullptr;
    }
    std::vector<Type *> parseProgramAndGetTypes(std::string &&str, const char *obj, int pass = 1)
    {
        auto objs = parseProgram(str.data(), obj, pass);
        std::vector<Type *> types;
        for (auto o : objs)
            types.push_back(o->type);
        return types;
    }
    std::string parseProgramAndGetTypeString(const char *str, const char *obj,
                                             int pass = 1)
    {
        auto objs = parseProgram(str, obj, pass);
        return (!objs.empty() && objs.front()->type)
                   ? objs.front()->type->toString()
                   : "<null>";
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

TEST_F(TypeTester, Arithmetic)
{
    std::unordered_map<std::string, int> spec =
    {
        // {"void",1},
        {"char",2},
        {"signed char",3},
        {"unsigned char",4},
        {"short",5},{"signed short",5},{"short int",5},{"signed short int",5},
        {"unsigned short",6},{"unsigned short int",6},
        {"int",7},{"signed",7},{"signed int",7},//{"",7},
        {"unsigned",8},{"unsigned int",8},
        {"long",9},{"signed long",9},{"long int",9},{"signed long int",9},
        {"unsigned long",10},{"unsigned long int",10},
        {"float",11},
        {"double",12},
        {"long double",13},
        // {"struct-or-union specifier",14},
        // {"enum-specifier",15},
        // {"typedef-name",16},
    };
    for (auto s : spec)
    {
        EXPECT_EQ(s.first,
                  parseProgramAndGetTypeString((s.first + " i;").data(), "i"));
    }
}

TEST_F(TypeTester, StructUnion)
{
    EXPECT_EQ(
        "struct SimpleStruct { int }",
        parseProgramAndGetTypeString("struct SimpleStruct { int i; } s;", "s"));

    EXPECT_EQ(
        "union SimpleUnion { int }",
        parseProgramAndGetTypeString("union SimpleUnion { int i; } s;", "s"));
}
TEST_F(TypeTester, Enum)
{
    // size, align, member offset
    EXPECT_EQ(
        "enum SimpleEnum { HAHA(0),XIXI(1) }",
        parseProgramAndGetTypeString("enum SimpleEnum { HAHA, XIXI } e;", "e"));
    EXPECT_EQ("enum EnumField { HAHA(3),HEHE(4),XIXI(0),YIYI(1) }",
              parseProgramAndGetTypeString(
                  "enum EnumField { HAHA = 3, HEHE, XIXI = 0, YIYI } e;", "e"));
}
// typedef

TEST_F(TypeTester, Pointer)
{
    EXPECT_EQ("pointer to int", parseProgramAndGetTypeString("int *p;", "p"));
    EXPECT_EQ("pointer to pointer to int",
              parseProgramAndGetTypeString("int **p;", "p"));
    EXPECT_EQ("pointer to pointer to pointer to int",
              parseProgramAndGetTypeString("int ***p;", "p"));
}

TEST_F(TypeTester, Array)
{
    EXPECT_EQ("array of int", parseProgramAndGetTypeString("int a[3];", "a"));
    EXPECT_EQ("array of array of int",
              parseProgramAndGetTypeString("int a[3][3];", "a"));
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
    EXPECT_EQ(true, parseProgramAndGetType("const char i;", "i")->isConst());
    EXPECT_EQ(true, parseProgramAndGetType("const int i;", "i")->isConst());
    EXPECT_EQ(true, parseProgramAndGetType("const float i;", "i")->isConst());
    EXPECT_EQ(true, parseProgramAndGetType(
                        "const struct ConstStruct { int i; } s;", "s")
                        ->isConst());
    EXPECT_EQ(true,
              parseProgramAndGetType("const enum ConstEnum { XIXI } s;", "s")
                  ->isConst());
}

TEST_F(TypeTester, Compatible)
{
    // TODO: void/tag/enum-const/pointer/array/function

    std::unordered_map<std::string, int> spec =
    {
        // {"void",1},
        {"char",2},
        {"signed char",3},
        {"unsigned char",4},
        {"short",5},{"signed short",5},{"short int",5},{"signed short int",5},
        {"unsigned short",6},{"unsigned short int",6},
        {"int",7},{"signed",7},{"signed int",7},//{"",7},
        {"unsigned",8},{"unsigned int",8},
        {"long",9},{"signed long",9},{"long int",9},{"signed long int",9},
        {"unsigned long",10},{"unsigned long int",10},
        {"float",11},
        {"double",12},
        {"long double",13},
        // {"struct-or-union specifier",14},
        // {"enum-specifier",15},
        // {"typedef-name",16},
    };
    std::unordered_map<std::string, int> qual =
    {
        {"", 1},
        {"const", 2},
        {"volatile", 3},
        {"const volatile", 4},
    };

    // type-specifiers type-qualifiers [ pointer [qualifiers] ] [ array | func ]

    // arithmetic type-qualifiers
    for (auto const &s1 : spec)
    {
        for (auto const &s2 : spec)
            for (auto const &q1 : qual)
                for (auto const &q2 : qual)
                {
                    auto types = parseProgramAndGetTypes(
                        q1.first + " " + s1.first + " a; " + q2.first + " " +
                            s2.first + " b;",
                        "a,b");
                    bool expected =
                        (s1.second == s2.second && q1.second == q2.second);
                    EXPECT_EQ(expected,
                              TypeUtil::Compatible(types[0], types[1]));
                    if (expected != TypeUtil::Compatible(types[0], types[1]))
                    {
                        std::cout << "type a: " << types[0]->toString()
                                  << "\ttype b: " << types[1]->toString()
                                  << std::endl;
                        return;
                    }
                }
    }

    // pointer [qualifiers]
    for (auto const &q1 : qual)
    {
        for (auto const &q2 : qual)
        {
            // compatible target type
            auto types = parseProgramAndGetTypes(
                "int *" + q1.first + " a; int *" + q2.first + " b;", "a,b");
            EXPECT_EQ(q1.second == q2.second,
                      TypeUtil::Compatible(types[0], types[1]));
            // incompatible target type
            types = parseProgramAndGetTypes(
                "char *" + q1.first + " a; int *" + q2.first + " b;", "a,b");
            EXPECT_EQ(false, TypeUtil::Compatible(types[0], types[1]));
        }
    }

    // array [qualifiers]
    std::unordered_map<std::string, int> array_size = {
        {"[]", 0}, {"[1]", 1}, {"[2]", 2},
    };
    for (auto const &q1 : qual)
    {
        for (auto const &q2 : qual)
        {
            for (auto const &sz1 : array_size)
            {
                for (auto const &sz2 : array_size)
                {
                    // present size + qualifiers AND
                    bool expected = ((sz1.second == sz2.second) ||
                                     (sz1.second == 0 || sz2.second == 0)) &&
                                    (q1.second == q2.second);
                    // compatible target type
                    auto types = parseProgramAndGetTypes(
                        "int " + q1.first + " a" + sz1.first + "; int " +
                            q2.first + " b" + sz2.first + ";",
                        "a,b");
                    EXPECT_EQ(expected,
                              TypeUtil::Compatible(types[0], types[1]));
                    // incompatible target type
                    types = parseProgramAndGetTypes(
                        "char " + q1.first + " a" + sz1.first + "; int " +
                            q2.first + " b" + sz2.first + ";",
                        "a,b");
                    EXPECT_EQ(false, TypeUtil::Compatible(types[0], types[1]));
                }
            }
        }
    }

    // struct
    // number + name + type + order
    bool TF[] = {false, true};
    for (auto number : TF)
    {
        for (auto name : TF)
            for (auto type : TF)
                for (auto order : TF)
                {
                    bool expected = !(number || name || type || order);
                    std::string a = "struct S { int i; int j; } a;";

                    std::string i = std::string() + (type ? "char " : "int ") +
                                    (name ? "ii;" : "i;");
                    std::string j = "int j;";
                    std::string b = "struct S { " + (order ? j + i : i + j) +
                                    (number ? "int k;" : "") + "} b;";
                    auto t1 = parseProgramAndGetTypes(std::string(a), "a")[0];
                    auto t2 = parseProgramAndGetTypes(std::string(b), "b")[0];
                    StringRef reason;
                    EXPECT_EQ(expected, TypeUtil::Compatible(t1, t2));
                    if(expected != TypeUtil::Compatible(t1, t2, &reason))
                        std::cout << "a: " << a << "\tb: " << b << std::endl
                            << "reason: " << reason << std::endl;
                }
    }

    // union
    // number + name + type + unorder
    for (auto number : TF)
    {
        for (auto name : TF)
            for (auto type : TF)
                for (auto order : TF)
                {
                    bool expected = !(number || name || type);
                    std::string a = "union S { int i; int j; } a;";

                    std::string i = std::string() + (type ? "char " : "int ") +
                                    (name ? "ii;" : "i;");
                    std::string j = "int j;";
                    std::string b = "union S { " + (order ? j + i : i + j) +
                                    (number ? "int k;" : "") + "} b;";
                    auto t1 = parseProgramAndGetTypes(std::string(a), "a")[0];
                    auto t2 = parseProgramAndGetTypes(std::string(b), "b")[0];
                    StringRef reason;
                    EXPECT_EQ(expected, TypeUtil::Compatible(t1, t2));
                    if(expected != TypeUtil::Compatible(t1, t2, &reason))
                        std::cout << "a: " << a << "\tb: " << b << std::endl
                            << "reason: " << reason << std::endl;
                }
    }

    // enum
    // number + name + value + unorder
    for (auto number : TF)
    {
        for (auto name : TF)
            for (auto value : TF)
                for (auto order : TF)
                {
                    bool expected = !(number || name || value);
                    std::string a = "enum E { HAHA = 1, HEHE = 2 } a;";

                    std::string i = std::string() + (name ? "XIXI" : "HAHA") +
                                    (value ? "= 3" : " = 1");
                    std::string j = "HEHE = 2";
                    std::string b = "enum E { " +
                                    (order ? j + "," + i : i + "," + j) +
                                    (number ? ",LOL" : "") + "} b;";
                    auto t1 = parseProgramAndGetTypes(std::string(a), "a")[0];
                    auto t2 = parseProgramAndGetTypes(std::string(b), "b")[0];
                    StringRef reason;
                    EXPECT_EQ(expected, TypeUtil::Compatible(t1, t2));
                    if (expected != TypeUtil::Compatible(t1, t2, &reason))
                        std::cout << "a: " << a << "\tb: " << b << std::endl
                                  << "reason: " << reason << std::endl;
                }
    }
    // enum + enum-const + integral
    {
        auto t = parseProgramAndGetTypes("enum E { HAHA } a; int i;", "a,HAHA,i");
        EXPECT_EQ(true, TypeUtil::Compatible(t[0],t[1]));
        EXPECT_EQ(true, TypeUtil::Compatible(t[0],t[2]));
        EXPECT_EQ(true, TypeUtil::Compatible(t[1],t[2]));
    }

    // function
}
