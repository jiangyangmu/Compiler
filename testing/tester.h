#pragma once

// #include <cassert>
#include <iostream>
#include <vector>

class Tester
{
    friend class TestRunner;

protected:
    virtual const char * getName() = 0;
    virtual void setUp()
    {
    }
    virtual void shutDown()
    {
    }
    virtual void run() = 0;
};

class TestRunner
{
public:
    static TestRunner & Get();
    void AddTest(Tester & test);
    void SetError(bool has_error);
    void RunAllTest();

private:
    std::vector<Tester *> all_tests_;
    bool has_error_;
};

#define TEST_F(TesterClassName, TestCaseName)                    \
    class TesterClassName##TestCaseName : public TesterClassName \
    {                                                            \
    protected:                                                   \
        virtual const char * getName()                           \
        {                                                        \
            return #TesterClassName "_" #TestCaseName;           \
        }                                                        \
        virtual void run();                                      \
                                                                 \
    public:                                                      \
        TesterClassName##TestCaseName()                          \
        {                                                        \
            TestRunner::Get().AddTest(*this);                    \
        }                                                        \
    };                                                           \
    static TesterClassName##TestCaseName                         \
        TesterClassName##TestCaseName##_instance;                \
    void TesterClassName##TestCaseName::run()

#define EXPECT_EQ(actual, expect)                                             \
    do                                                                        \
    {                                                                         \
        auto __e = (expect);                                                  \
        auto __a = (actual);                                                  \
        if (!((__e) == (__a)))                                                \
        {                                                                     \
            std::cerr << "Expect: " << (__e) << std::endl                     \
                      << "Actual: " << (__a) << std::endl                     \
                      << "\tat " << __FILE__ << ":" << __LINE__ << std::endl; \
            TestRunner::Get().SetError(true);                                 \
        }                                                                     \
    } while (false)

#define EXPECT_EQ_PRINT(actual, expect, msg)            \
    do                                                  \
    {                                                   \
        auto e = (expect);                              \
        auto a = (actual);                              \
        if (!((e) == (a)))                              \
        {                                               \
            std::cerr << "Expect: " << (e) << std::endl \
                      << "Actual: " << (a) << std::endl \
                      << (msg) << std::endl;            \
            TestRunner::Get().SetError(true);           \
        }                                               \
    } while (false)

#define EXPECT_EQ_SET(actual, expect)                                 \
    do                                                                \
    {                                                                 \
        auto __a = (actual);                                          \
        decltype(__a) __e = (expect);                                 \
        bool _local_has_error = false;                                \
        for (auto ei : __e)                                           \
        {                                                             \
            if (__a.find(ei) == __a.end())                            \
            {                                                         \
                /* std::cerr << "removing: " << (ei) << std::endl; */ \
                TestRunner::Get().SetError(true);                     \
                _local_has_error = true;                              \
            }                                                         \
        }                                                             \
        for (auto ai : __a)                                           \
        {                                                             \
            if (__e.find(ai) == __e.end())                            \
            {                                                         \
                /* std::cerr << "adding: " << (ai) << std::endl;  */  \
                TestRunner::Get().SetError(true);                     \
                _local_has_error = true;                              \
            }                                                         \
        }                                                             \
        if (_local_has_error)                                         \
        {                                                             \
            std::cerr << "Expect: {";                                 \
            for (auto ei : __e)                                       \
            {                                                         \
                std::cerr << ei << ',';                               \
            }                                                         \
            std::cerr << '}' << std::endl;                            \
            std::cerr << "Actual: {";                                 \
            for (auto ai : __a)                                       \
            {                                                         \
                std::cerr << ai << ',';                               \
            }                                                         \
            std::cerr << '}' << " at " << __FILE__ << ":" << __LINE__ \
                      << std::endl;                                   \
        }                                                             \
    } while (false)
