#include "RegexImpl.h"

namespace v2 {



}

#ifdef UNIT_TEST
#include "../UnitTest/UnitTest.h"

using namespace v2;

class REToStringConverter : public re::Visitor
{
public:
    virtual void Visit(re::ConcatOperator &) override
    {
        ASSERT(arrStr.Count() >= 2);
        String s1 = arrStr[arrStr.Count() - 2];
        String s2 = arrStr[arrStr.Count() - 1];
        s1.Add('(').Append(s2).Add(')');
        arrStr.RemoveAt(arrStr.Count() - 2, 2);
        arrStr.Add(s1);
    }
    virtual void Visit(re::AlterOperator &) override
    {
        ASSERT(arrStr.Count() >= 2);
        String s;
        String s1 = arrStr[arrStr.Count() - 2];
        String s2 = arrStr[arrStr.Count() - 1];
        s.Add('(').Append(s1).Add('|').Append(s2).Add(')');
        arrStr.RemoveAt(arrStr.Count() - 2, 2);
        arrStr.Add(s1);
    }
    virtual void Visit(re::KleeneStarOperator &) override
    {
        ASSERT(arrStr.Count() >= 1);
        String s;
        s.Add('(').Append(arrStr[arrStr.Count() - 1]).Add(')').Add('*');
        arrStr.RemoveAt(arrStr.Count() - 1, 1);
        arrStr.Add(s);
    }
    virtual void Visit(re::ASCIICharacter & ch) override
    {
        arrStr.Add(String((char)ch.Index(), 1));
    }

    static String Convert(re::Tree & tree)
    {
        REToStringConverter cvt;
        tree.Accept(cvt);
        return cvt.arrStr[0];
    }

private:
    Array<String> arrStr;
};

TEST(Regex_API_Build)
{
    re::CompositorContext context;

    // abc*|d
    re::Compositor cp =
        re::Alter(
            re::Concat(re::Concat(
                re::Ascii('a'),
                re::Ascii('b')),
                re::KleeneStar(re::Ascii('c'))),
            re::Ascii('d'));

    re::Tree re = cp.Get();

    EXPECT_EQ(
        REToStringConverter::Convert(re),
        String(""));
}

#endif