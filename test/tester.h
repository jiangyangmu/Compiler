#pragma once

// #include <cassert>
#include <iostream>
#include <vector>

class Tester
{
   protected:
    static std::vector<Tester *> _all_tests;
    static bool _has_error;

   protected:
    virtual const char *getName() = 0;
    virtual void setUp() {}
    virtual void shutDown() {}
    virtual void run() = 0;

   public:
    static void RunAllTest();
};

#define TEST_F(TesterClassName, TestCaseName)                    \
    class TesterClassName##TestCaseName : public TesterClassName \
    {                                                            \
       protected:                                                \
        virtual const char *getName()                            \
        {                                                        \
            return #TesterClassName "_" #TestCaseName;           \
        }                                                        \
        virtual void run();                                      \
                                                                 \
       public:                                                   \
        TesterClassName##TestCaseName()                          \
        {                                                        \
            _all_tests.push_back(this);                          \
        }                                                        \
    };                                                           \
    static TesterClassName##TestCaseName                         \
        TesterClassName##TestCaseName##_instance;                \
    void TesterClassName##TestCaseName::run()

#define EXPECT_EQ(expect, actual)                        \
    do                                                   \
    {                                                    \
        auto e = (expect);                               \
        auto a = (actual);                               \
        if (!((e) == (a)))                               \
        {                                                \
            std::cerr << "Expect: " << (e) << std::endl  \
                      << "Actual: " << (a) << std::endl; \
            _has_error = true;                           \
        }                                                \
    } while (false)
