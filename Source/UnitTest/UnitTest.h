#pragma once

#ifdef UNIT_TEST

// #include <cassert>
#include <iostream>
#include <vector>
#include <unordered_map>

class Tester
{
    friend class TestRunner;

protected:
    virtual const char * GetTestClassName() = 0;
    virtual const char * GetTestCaseName() = 0;
    virtual void RunTestCase() = 0;

    virtual void BeforeAllTestCases() {}
    virtual void BeforeTestCase() {}
    virtual void AfterTestCase() {}
    virtual void AfterAllTestCases() {}
};

class TestRunner
{
public:
    static TestRunner & Get();
    void AddTest(Tester & test);
    void AddTest(const char * testClassName, Tester & test);
    void SetError(bool has_error);
    void RunAllTest();

private:
    std::unordered_map<std::string, std::vector<Tester *>> all_tests_;
    bool has_error_;
};

#define TEST(CaseName)                        \
    class CaseName : public Tester {          \
    protected:                                \
        virtual const char * GetTestClassName() { \
            return "";                        \
        }                                     \
        virtual const char * GetTestCaseName() { \
            return #CaseName;                 \
        }                                     \
        virtual void RunTestCase();           \
                                              \
    public:                                   \
        CaseName() {                          \
            TestRunner::Get().AddTest(*this); \
        }                                     \
    };                                        \
    static CaseName CaseName##_instance;      \
    void CaseName::RunTestCase()

// TesterClassName is defined by user, inherit from class Tester.
#define TEST_F(TesterClassName, TestCaseName)                    \
    class TesterClassName##TestCaseName : public TesterClassName \
    {                                                            \
    protected:                                                   \
        virtual const char * GetTestClassName()                  \
        {                                                        \
            return #TesterClassName;                             \
        }                                                        \
        virtual const char * GetTestCaseName()                   \
        {                                                        \
            return #TestCaseName;                                \
        }                                                        \
        virtual void RunTestCase();                              \
                                                                 \
    public:                                                      \
        TesterClassName##TestCaseName()                          \
        {                                                        \
            TestRunner::Get().AddTest(#TesterClassName, *this);  \
        }                                                        \
    };                                                           \
    static TesterClassName##TestCaseName                         \
        TesterClassName##TestCaseName##_instance;                \
    void TesterClassName##TestCaseName::RunTestCase()

#define EXPECT_NOT_REACH \
    do { \
        std::cerr << "Should not reach here." << std::endl; \
        TestRunner::Get().SetError(true); \
    } while (false)

#define EXPECT_TRUE(expr) \
    do                                                                        \
    {                                                                         \
        if (!(expr))                                                          \
        {                                                                     \
            std::cerr << "Expr:  " #expr << std::endl                         \
                      << "  at " << __FILE__ << ":" << __LINE__ << std::endl  \
                      << "Is false, should be true." << std::endl;            \
            TestRunner::Get().SetError(true);                                 \
        }                                                                     \
    } while (false)
#define EXPECT_FALSE(expr) \
    do                                                                        \
    {                                                                         \
        if ((expr))                                                           \
        {                                                                     \
            std::cerr << "Expr:  " #expr << std::endl                         \
                      << "  at " << __FILE__ << ":" << __LINE__ << std::endl  \
                      << "Is false, should be true." << std::endl;            \
            TestRunner::Get().SetError(true);                                 \
        }                                                                     \
    } while (false)

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

#endif