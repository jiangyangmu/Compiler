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

#define TEST(CaseName)                        \
    class CaseName : public Tester {          \
    protected:                                \
        virtual const char * getName() {      \
            return #CaseName;                 \
        }                                     \
        virtual void run();                   \
                                              \
    public:                                   \
        CaseName() {                          \
            TestRunner::Get().AddTest(*this); \
        }                                     \
    };                                        \
    static CaseName CaseName##_instance;      \
    void CaseName::run()

// TesterClassName is defined by user.
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

#define EXPECT_EQ(left, right)                                                \
    do                                                                        \
    {                                                                         \
        auto __l = (left);                                                    \
        auto __r = (right);                                                   \
        if (!((__l) == (__r)))                                                \
        {                                                                     \
            std::cerr << "Expr:  (" #left ") == (" #right ")" << std::endl    \
                      << "  at " << __FILE__ << ":" << __LINE__ << std::endl  \
                      << "Left:  " << (__l) << std::endl                      \
                      << "Right: " << (__r) << std::endl;                     \
            TestRunner::Get().SetError(true);                                 \
        }                                                                     \
    } while (false)

#define EXPECT_NE(left, right)                                                \
    do                                                                        \
    {                                                                         \
        auto __l = (left);                                                    \
        auto __r = (right);                                                   \
        if (!((__l) != (__r)))                                                \
        {                                                                     \
            std::cerr << "Expr:  (" #left ") == (" #right ")" << std::endl    \
                      << "  at " << __FILE__ << ":" << __LINE__ << std::endl  \
                      << "Left:  " << (__l) << std::endl                      \
                      << "Right: " << (__r) << std::endl;                     \
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

#define EXPECT_EQ_LIST(actual, expect)                                \
    do                                                                \
    {                                                                 \
        auto __a = (actual);                                          \
        decltype(__a) __e = (expect);                                 \
        bool _local_has_error = false;                                \
        auto __ei = std::begin(__e);                                  \
        auto __ed = std::end(__e);                                    \
        auto __ai = std::begin(__a);                                  \
        auto __ad = std::end(__a);                                    \
        while (__ei != __ed && __ai != __ad)                          \
        {                                                             \
            if (*__ei != *__ai)                                       \
            {                                                         \
                TestRunner::Get().SetError(true);                     \
                _local_has_error = true;                              \
                break;                                                \
            }                                                         \
            ++__ei, ++__ai;                                           \
        }                                                             \
        if (__ei != __ed || __ai != __ad)                             \
            _local_has_error = true;                                  \
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
            TestRunner::Get().SetError(true);                         \
        }                                                             \
    } while (false)

#define EXPECT_EQ_SET(actual, expect)                                 \
    do                                                                \
    {                                                                 \
        auto __a = (actual);                                          \
        decltype(__a) __e = (expect);                                 \
        bool _local_has_error = false;                                \
        for (auto ei : __e)                                           \
        {                                                             \
            if (std::find(__a.begin(), __a.end(), ei) == __a.end())   \
            {                                                         \
                /* std::cerr << "removing: " << (ei) << std::endl; */ \
                TestRunner::Get().SetError(true);                     \
                _local_has_error = true;                              \
            }                                                         \
        }                                                             \
        for (auto ai : __a)                                           \
        {                                                             \
            if (std::find(__e.begin(), __e.end(), ai) == __e.end())   \
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

#define ASSERT_EQ(left, right)                                                \
    do                                                                        \
    {                                                                         \
        auto __l = (left);                                                    \
        auto __r = (right);                                                   \
        if (!((__l) == (__r)))                                                \
        {                                                                     \
            std::cerr << "Expr:  (" #left ") == (" #right ")" << std::endl    \
                      << "  at " << __FILE__ << ":" << __LINE__ << std::endl  \
                      << "Left:  " << (__l) << std::endl                      \
                      << "Right: " << (__r) << std::endl;                     \
            std::exit(EXIT_FAILURE);                                          \
        }                                                                     \
    } while (false)