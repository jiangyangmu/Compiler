#include "Linq.h"

#ifdef UNIT_TEST
#include "../UnitTest/UnitTest.h"

#include <vector>
#include <complex>

std::ostream & operator << (std::ostream & o, const std::vector<int> & v)
{
    o << "{";
    if (!v.empty())
    {
        o << " ";
        auto it = v.begin();
        o << *it;
        while (++it != v.end()) o << ", " << *it;
        o << " ";
    }
    o << "}";
    return o;
}

TEST(LINQ_API)
{
    std::vector<int> ints = {1, 2, 3, 4, 5};

    std::function<int(char)> f;

    auto ord    = ints | linq::order_by_func<int, int>([](int i){return -i;});

    int max_int = ints | linq::max();
    int min_int = ints | linq::min();
    int cnt     = ints | linq::count();
    int avg     = ints | linq::average();
    int mul     = ints | linq::aggregate(1, [](int x, int y) -> int { return x * y; });

    std::cout
        << "origin: " << ints << std::endl
        << std::endl
        << "ord: " << ord << std::endl
        << "max: " << max_int << std::endl
        << "min: " << min_int << std::endl
        << "cnt: " << cnt << std::endl
        << "avg: " << avg << std::endl
        << "mul: " << mul << std::endl;
}

#endif