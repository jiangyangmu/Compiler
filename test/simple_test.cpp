#include "tester.h"

class SimpleTester : public Tester
{
   protected:
    virtual void setUp()
    {
        std::cout << "SimpleTester: setUp()" << std::endl;
    }
    virtual void shutDown()
    {
        std::cout << "SimpleTester: shutDown()" << std::endl;
    }
};

TEST_F(SimpleTester, TestCase0)
{
    EXPECT_EQ(1, 1);
}

TEST_F(SimpleTester, TestCase1)
{
    EXPECT_EQ(1, 2);
}

