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
            _has_error = true;                                                \
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
            _has_error = true;                          \
        }                                               \
    } while (false)

#define EXPECT_EQ_SET(actual, expect)                                          \
  do {                                                                         \
    auto __a = (actual);                                                       \
    decltype(__a) __e = (expect);                                              \
    bool _local_has_error = false;                                             \
    for (auto ei : __e) {                                                      \
      if (__a.find(ei) == __a.end()) {                                         \
        /* std::cerr << "removing: " << (ei) << std::endl; */                  \
        _has_error = true;                                                     \
        _local_has_error = true;                                               \
      }                                                                        \
    }                                                                          \
    for (auto ai : __a) {                                                      \
      if (__e.find(ai) == __e.end()) {                                         \
        /* std::cerr << "adding: " << (ai) << std::endl;  */                   \
        _has_error = true;                                                     \
        _local_has_error = true;                                               \
      }                                                                        \
    }                                                                          \
    if (_local_has_error) {                                                    \
      std::cerr << "Expect: {";                                                \
      for (auto ei : __e) {                                                    \
        std::cerr << ei << ',';                                                \
      }                                                                        \
      std::cerr << '}' << std::endl;                                           \
      std::cerr << "Actual: {";                                                \
      for (auto ai : __a) {                                                    \
        std::cerr << ai << ',';                                                \
      }                                                                        \
      std::cerr << '}' << " at " << __FILE__ << ":" << __LINE__ << std::endl;  \
    }                                                                          \
  } while (false)