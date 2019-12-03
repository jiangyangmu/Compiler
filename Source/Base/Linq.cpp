#include "Linq.h"

#ifdef UNIT_TEST
#include "../UnitTest/UnitTest.h"

#include <vector>
#include <set>
#include <iomanip>

template <typename TContainer>
std::ostream & OutputContainer(std::ostream & o, const TContainer & v)
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
template <template<typename, typename> typename TContainer, typename TElement, typename A>
std::ostream & operator<< (std::ostream & o, const TContainer<TElement, A> & v)
{
    return OutputContainer<TContainer<TElement, A>>(o, v);
}
template <template<typename, typename, typename> typename TContainer, typename TElement, typename A, typename B>
std::ostream & operator<< (std::ostream & o, const TContainer<TElement, A, B> & v)
{
    return OutputContainer<TContainer<TElement, A, B>>(o, v);
}

template <typename A, typename B>
std::ostream & operator<< (std::ostream & o, const std::pair<A, B> & p)
{
    return o << "(" << p.first << ", " << p.second << ")";
}
template <typename K, typename V>
std::ostream & operator<< (std::ostream & o, const std::map<K, V> & m)
{
    o << "{";
    if (!m.empty())
    {
        o << " ";
        auto it = m.begin();
        o << it->first << ":" << it->second;
        while (++it != m.end()) o << ", " << it->first << ":" << it->second;
        o << " ";
    }
    o << "}";
    return o;
}

struct IdAndName {
    int id;
    std::string name;

    friend std::ostream & operator<< (std::ostream & o, const IdAndName & name)
    {
        return o << name.name.c_str();
    }
};
struct IdAndMajor {
    int id;
    std::string major;

    friend std::ostream & operator<< (std::ostream & o, const IdAndMajor & major)
    {
        return o << major.major.c_str();
    }
};

TEST(LINQ_API)
{
    std::vector<int> ints = {2, 4, 4, 5, 3, 1, 3};

    auto filt   = ints | linq::where([](int i){ return i % 2 == 0; });

    auto all    = ints | linq::all([](int i){ return i < 5; });
    auto any    = ints | linq::any([](int i){ return i < 5; });

    struct Point { int x; float y; };
    std::vector<Point> pts = { {1, 2.0f}, {2, 3.0f}, {3, 4.0f} };
    auto sel_x  = pts | linq::select([](const Point & p) { return p.x; });
    auto sel_y  = pts | linq::select([](const Point & p) { return p.y; });
    auto sel_xx = pts | linq::select_many([](const Point & p) { std::vector<int> v = { p.x, p.x }; return v; });

    auto ord    = ints | linq::order_by([](int i){return -i;});
    auto rev    = ints | linq::reverse();

    std::set<int> ints_expt = { 1, 2, 3 };
    std::set<int> ints_itst = { 4, 5, 6, 7 };
    std::set<int> ints_unio = { 4, 5, 6, 7 };
    auto dist   = ints | linq::distinct();
    auto expt   = ints | linq::except(ints_expt);
    auto itst   = ints | linq::intersect(ints_itst);
    auto unio   = ints | linq::union_(ints_unio);

    auto ski    = ints | linq::skip(3);
    auto skw    = ints | linq::skip_while([](int i){return i != 5;});
    auto tak    = ints | linq::take(2);
    auto taw    = ints | linq::take_while([](int i){return i != 1;});

   
    std::vector<IdAndName> names = {
        { 1, "Peter" },
        { 2, "Ann" },
        { 4, "John" },
    };
    std::vector<IdAndMajor> majors = {
        { 2, "Economics" },
        { 3, "Politics" },
        { 2, "Psychology" },
        { 1, "Computer Science" },
    };
    auto joi    = names | linq::join(majors,
                                     [](const IdAndName & name) { return name.id; },
                                     [](const IdAndMajor & major) { return major.id; });
    auto gjoi   = names | linq::group_join(majors,
                                     [](const IdAndName & name) { return name.id; },
                                     [](const IdAndMajor & major) { return major.id; });

    int max_int = ints | linq::max();
    int min_int = ints | linq::min();
    int sum     = ints | linq::sum();
    int cnt     = ints | linq::count();
    int avg     = ints | linq::average();
    int mul     = ints | linq::aggregate(1, [](int x, int y) -> int { return x * y; });

    std::cout
        << std::fixed << std::setprecision(2)
        << "origin: " << ints << std::endl
        << std::endl

        << "filt: " << filt << std::endl

        << "all: " << all << std::endl
        << "any: " << any << std::endl

        << "sel_x: " << sel_x << std::endl
        << "sel_y: " << sel_y << std::endl
        << "sel_xx: " << sel_xx << std::endl

        << "ord: " << ord << std::endl
        << "rev: " << rev << std::endl

        << "dist: " << dist << std::endl
        << "expt" << ints_expt << ": " << expt << std::endl
        << "itst" << ints_itst << ": " << itst << std::endl
        << "unio" << ints_unio << ": " << unio << std::endl

        << "ski: " << ski << std::endl
        << "skw: " << skw << std::endl
        << "tak: " << tak << std::endl
        << "taw: " << taw << std::endl

        << "joi: " << joi << std::endl
        << "gjoi: " << gjoi << std::endl

        << "max: " << max_int << std::endl
        << "min: " << min_int << std::endl
        << "sum: " << sum << std::endl
        << "cnt: " << cnt << std::endl
        << "avg: " << avg << std::endl
        << "mul: " << mul << std::endl;
}

#endif