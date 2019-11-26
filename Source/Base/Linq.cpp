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
    std::vector<int> ints = {2, 4, 5, 1, 3};

    std::function<int(char)> f;

    auto ord    = ints | linq::order_by([](int i){return -i;});
    auto rev    = ints | linq::reverse();

    auto ski    = ints | linq::skip(3);
    auto skw    = ints | linq::skip_while([](int i){return i != 5;});
    auto tak    = ints | linq::take(2);
    auto taw    = ints | linq::take_while([](int i){return i != 1;});

    int max_int = ints | linq::max();
    int min_int = ints | linq::min();
    int cnt     = ints | linq::count();
    int avg     = ints | linq::average();
    int mul     = ints | linq::aggregate(1, [](int x, int y) -> int { return x * y; });

    std::cout
        << "origin: " << ints << std::endl
        << std::endl
        << "ord: " << ord << std::endl
        << "rev: " << rev << std::endl
        << "ski: " << ski << std::endl
        << "skw: " << skw << std::endl
        << "tak: " << tak << std::endl
        << "taw: " << taw << std::endl
        << "max: " << max_int << std::endl
        << "min: " << min_int << std::endl
        << "cnt: " << cnt << std::endl
        << "avg: " << avg << std::endl
        << "mul: " << mul << std::endl;
}

#endif