#include "tester.h"

class SimpleTester : public Tester
{
   protected:
    virtual void setUp()
    {
    }
    virtual void shutDown()
    {
    }
};

/*
TEST_F(SimpleTester, TestCase0)
{
    EXPECT_EQ(1, 1);
}

TEST_F(SimpleTester, TestCase1)
{
    EXPECT_EQ(1, 2);
}
 */
