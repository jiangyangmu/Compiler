// #pragma once

#include "tester.h"

#include <iostream>
#include <vector>

#define COLOR_OUTPUT
#ifdef COLOR_OUTPUT

#define COLOR_RED "\033[0;31m"
#define COLOR_GREEN "\033[0;32m"
#define COLOR_END "\033[0m"

#else

#define COLOR_RED ""
#define COLOR_GREEN ""
#define COLOR_END ""

#endif

std::vector<Tester *> Tester::_all_tests;
bool Tester::_has_error;

void Tester::RunAllTest()
{
    size_t count = _all_tests.size();
    size_t failed = 0, passed = 0;
    for (auto test : _all_tests)
    {
        std::cout << "[ RUNNING ] " << test->getName() << std::endl;
        _has_error = false;
        test->setUp();
        test->run();
        test->shutDown();
        if (_has_error)
        {
            std::cout << COLOR_RED "[  FAILED ] " COLOR_END << test->getName()
                      << std::endl;
            ++failed;
        }
        else
        {
            std::cout << COLOR_GREEN "[      OK ] " COLOR_END << test->getName()
                      << std::endl;
            ++passed;
        }
    }
    std::cout << std::endl
              << "tested " << count << " methods, " << passed << " passed, "
              << failed << " failed." << std::endl;
}

int main(void)
{
    Tester::RunAllTest();
    return 0;
}
