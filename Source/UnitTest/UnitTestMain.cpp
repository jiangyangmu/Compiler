#ifdef UNIT_TEST

#include "UnitTest.h"

#include <iostream>
#include <vector>
#include <tuple>
#include <unordered_set>
#include <cassert>

//#define COLOR_OUTPUT
#ifdef COLOR_OUTPUT

#define COLOR_RED "\033[0;31m"
#define COLOR_GREEN "\033[0;32m"
#define COLOR_END "\033[0m"

#else

#define COLOR_RED ""
#define COLOR_GREEN ""
#define COLOR_END ""

#endif

#define TEST_FILTER "API"

TestRunner & TestRunner::Get() {
    static TestRunner * runner = new TestRunner();
    return *runner;
}

void TestRunner::AddTest(Tester & test) {
    all_tests_[""].push_back(&test);
}

void TestRunner::AddTest(const char * testClassName, Tester & test)
{
    all_tests_[testClassName].push_back(&test);
}

void TestRunner::SetError(bool has_error) {
    has_error_ = has_error;
}

void TestRunner::RunAllTest() {
    size_t count = 0;
    size_t failed = 0, passed = 0;
    std::string filter = TEST_FILTER;
    
    for (auto testCaseSet : all_tests_)
    {
        assert(!testCaseSet.second.empty());
        testCaseSet.second.front()->BeforeAllTestCases();
        for (auto test : testCaseSet.second)
        {
            std::string testName =
                std::string(test->GetTestClassName()) + "_" + test->GetTestCaseName();
            if (!filter.empty() && testName.find(filter) == std::string::npos)
                continue;

            std::cout << "[ RUN     ] " << testName.data() << std::endl;
            ++count;
            has_error_ = false;
            test->BeforeTestCase();
            test->RunTestCase();
            test->AfterTestCase();
            if (has_error_)
            {
                std::cout << COLOR_RED "[  FAILED ] " COLOR_END << test->GetTestCaseName()
                          << std::endl;
                ++failed;
            } else
            {
                std::cout << COLOR_GREEN "[      OK ] " COLOR_END << test->GetTestCaseName()
                          << std::endl;
                ++passed;
            }
        }
        testCaseSet.second.front()->AfterAllTestCases();
    }

    std::cout << "[ DONE    ] tested " << count << " methods, " << passed
              << " passed, " << failed << " failed." << std::endl;
}

int main(void) {
    TestRunner::Get().RunAllTest();
    return 0;
}

#endif
