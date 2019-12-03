#pragma once

#include <functional>
#include <algorithm>
#include <tuple>
#include <set>
#include <map>
#include <iterator>

namespace linq {

// Excludes T from argument deduction.
template <typename T> struct no_arg_deduction { typedef T type; };

// as seen on http://functionalcpp.wordpress.com/2013/08/05/function-traits/
template<class F>
struct function_traits;

// function pointer
template<class R, class... Args>
struct function_traits<R(*)(Args...)> : public function_traits<R(Args...)>
{};

template<class R, class... Args>
struct function_traits<R(Args...)>
{
    using return_type = R;

    static constexpr std::size_t arity = sizeof...(Args);

    template <std::size_t N>
    struct argument
    {
        static_assert(N < arity, "error: invalid parameter index.");
        using type = typename std::tuple_element<N, std::tuple<Args...>>::type;
    };
};

// member function pointer
template<class C, class R, class... Args>
struct function_traits<R(C::*)(Args...)> : public function_traits<R(C&, Args...)>
{};

// const member function pointer
template<class C, class R, class... Args>
struct function_traits<R(C::*)(Args...) const> : public function_traits<R(C&, Args...)>
{};

// member object pointer
template<class C, class R>
struct function_traits<R(C::*)> : public function_traits<R(C&)>
{};

// functor
template<class F>
struct function_traits
{
private:
    using call_type = function_traits<decltype(&F::operator())>;
public:
    using return_type = typename call_type::return_type;

    static constexpr std::size_t arity = call_type::arity - 1;

    template <std::size_t N>
    struct argument
    {
        static_assert(N < arity, "error: invalid parameter index.");
        using type = typename call_type::template argument<N + 1>::type;
    };
};

// ================================================
// Filter
//
//  linq::where(pred)
//

template <typename TElement>
struct where_functor
{
    where_functor(std::function<bool(TElement)> pred) : pred(pred) {}

    std::function<bool(TElement)> pred;
};

template <typename TPred>
auto
where(TPred pred)
{
    using TElement = function_traits<TPred>::argument<0>::type;
    return where_functor<TElement>(pred);
}

// ================================================
// Qualifier
//
//  linq::all(pred)
//  linq::any(pred)
//

template <typename TElement>
struct all_functor
{
    all_functor(std::function<bool(TElement)> pred) : pred(pred) {}

    std::function<bool(TElement)> pred;
};

template <typename TPred>
auto
all(TPred pred)
{
    using TElement = function_traits<TPred>::argument<0>::type;
    return all_functor<TElement>(pred);
}

template <typename TElement>
struct any_functor
{
    any_functor(std::function<bool(TElement)> pred) : pred(pred) {}

    std::function<bool(TElement)> pred;
};

template <typename TPred>
auto
any(TPred pred)
{
    using TElement = function_traits<TPred>::argument<0>::type;
    return any_functor<TElement>(pred);
}

// ================================================
// Projection
//
//  linq::select(pred)
//  linq::select_many(pred)
//

template <typename TElement, typename TSubElement>
struct select_functor
{
    select_functor(std::function<TSubElement(TElement)> pred) : pred(pred) {}

    std::function<TSubElement(TElement)> pred;
};

template <typename TPred>
auto
select(TPred pred)
{
    using TSubElement = function_traits<TPred>::return_type;
    using TElement = function_traits<TPred>::argument<0>::type;
    return select_functor<TElement, TSubElement>(pred);
}

template <typename TElement, typename TSubElementRange>
struct select_many_functor
{
    select_many_functor(std::function<TSubElementRange(TElement)> pred) : pred(pred) {}

    std::function<TSubElementRange(TElement)> pred;
};

template <typename TPred>
auto
select_many(TPred pred)
{
    using TSubElementRange = function_traits<TPred>::return_type;
    using TElement = function_traits<TPred>::argument<0>::type;
    return select_many_functor<TElement, TSubElementRange>(pred);
}


// ================================================
// Sorting
//
//  linq::order_by(pred)
//  linq::reverse()
//

template <typename TElement, typename TKey>
struct order_by_functor
{
    order_by_functor(std::function<TKey(TElement)> pred) : pred(pred) {}

    std::function<TKey(TElement)> pred;
};

template <typename TPred>
auto
order_by(TPred pred)
{
    using TKey = function_traits<TPred>::return_type;
    using TElement = function_traits<TPred>::argument<0>::type;
    return order_by_functor<TElement, TKey>(pred);
}

struct reverse {};

// ================================================
// Set
//
//  linq::distinct()
//  linq::except(range)
//  linq::intersect(range)
//  linq::union(range)
//

struct distinct {};

template <typename TRange>
struct range_functor
{
    range_functor(const TRange & range) : range(range) {}

    const TRange & range;
};

template <typename TRange>
struct except_functor
{
    except_functor(const TRange & range) : range(range) {}

    const TRange & range;
};
template <typename TRange>
auto
except(const TRange & range)
{
    return except_functor<TRange>(range);
};

template <typename TRange>
struct intersect_functor
{
    intersect_functor(const TRange & range) : range(range) {}

    const TRange & range;
};
template <typename TRange>
auto
intersect(const TRange & range)
{
    return intersect_functor<TRange>(range);
};

template <typename TRange>
struct union_functor
{
    union_functor(const TRange & range) : range(range) {}

    const TRange & range;
};
template <typename TRange>
auto
union_(const TRange & range)
{
    return union_functor<TRange>(range);
};

// ================================================
// Partitioning
//
//  linq::skip(n)
//  linq::skip_while(pred)
//  linq::take(n)
//  linq::take_while(pred)
//

struct skip
{
    skip(int n) : n(n) {}

    int n;
};

template <typename TElement>
struct skip_while_functor
{
    skip_while_functor(std::function<bool(TElement)> pred) : pred(pred) {}

    std::function<bool(TElement)> pred;
};
template <typename TPred>
auto
skip_while(TPred pred)
{
    using TElement = function_traits<TPred>::argument<0>::type;
    return skip_while_functor<TElement>(pred);
}

struct take
{
    take(int n) : n(n) {}

    int n;
};

template <typename TElement>
struct take_while_functor
{
    take_while_functor(std::function<bool(TElement)> pred) : pred(pred) {}

    std::function<bool(TElement)> pred;
};
template <typename TPred>
auto
take_while(TPred pred)
{
    using TElement = function_traits<TPred>::argument<0>::type;
    return take_while_functor<TElement>(pred);
}

// ================================================
// Join
//
//  linq::join(range, pred_x, pred_y)
//


template <typename TRange, typename TElementX, typename TElementY, typename TKey>
struct join_functor
{
    join_functor(TRange range,
                 std::function<TKey(TElementX)> pred_x,
                 std::function<TKey(TElementY)> pred_y)
        : range(range)
        , pred_x(pred_x)
        , pred_y(pred_y) {}

    TRange range;
    std::function<TKey(TElementX)> pred_x;
    std::function<TKey(TElementY)> pred_y;
};
template <typename TRange, typename TPredX, typename TPredY>
auto
join(TRange range,
     TPredX pred_x,
     TPredY pred_y)
{
    using TElementX = function_traits<TPredX>::argument<0>::type;
    using TElementY = function_traits<TPredY>::argument<0>::type;
    using TKeyX = function_traits<TPredX>::return_type;
    using TKeyY = function_traits<TPredY>::return_type;
    static_assert(std::is_same<TKeyX, TKeyY>::value, "Preds must have the same return type.");
    return join_functor<TRange, TElementX, TElementY, TKeyX>(range, pred_x, pred_y);
}

template <typename TRange, typename TElementX, typename TElementY, typename TKey>
struct group_join_functor : public join_functor<TRange, TElementX, TElementY, TKey>
{
public:
    using join_functor<TRange, TElementX, TElementY, TKey>::join_functor;
};
template <typename TRange, typename TPredX, typename TPredY>
auto
group_join(TRange range,
           TPredX pred_x,
           TPredY pred_y)
{
    using TElementX = function_traits<TPredX>::argument<0>::type;
    using TElementY = function_traits<TPredY>::argument<0>::type;
    using TKeyX = function_traits<TPredX>::return_type;
    using TKeyY = function_traits<TPredY>::return_type;
    static_assert(std::is_same<TKeyX, TKeyY>::value, "Preds must have the same return type.");
    return group_join_functor<TRange, TElementX, TElementY, TKeyX>(range, pred_x, pred_y);
}


// ================================================
// Aggregation
//
//  linq::aggregate(seed, pred)
//  linq::average()
//  linq::count()
//  linq::sum()
//  linq::max()
//  linq::min()
//

template <typename T>
struct aggregate_functor
{
    aggregate_functor(T seed, std::function<T(T, T)> pred) : result(seed), pred(pred) {}
    void add(T next) { result = pred(result, next); }
    T get() { return result; }
    T result;
    std::function<T(T, T)> pred;
};
template <typename TElement>
auto
aggregate(TElement seed,
          typename no_arg_deduction<std::function<TElement(TElement, TElement)>>::type pred)
{
    return aggregate_functor<TElement>(seed, pred);
}

struct average {};
struct count {};
struct sum {};
struct max {};
struct min {};

}

template <typename TRange, typename TElement = TRange::value_type>
TRange operator | (const TRange & source, linq::where_functor<TElement> whe)
{
    TRange source_filtered;

    auto inserter = std::back_inserter(source_filtered);
    for (auto it = source.begin(); it != source.end(); ++it)
    {
        if (whe.pred(*it))
            inserter = *it;
    }

    return source_filtered;
}

template <typename TRange, typename TElement = TRange::value_type>
bool operator | (const TRange & source, linq::all_functor<TElement> all)
{
    for (auto it = source.begin(); it != source.end(); ++it)
    {
        if (!all.pred(*it))
            return false;
    }
    return true;
}

template <typename TRange, typename TElement = TRange::value_type>
bool operator | (const TRange & source, linq::any_functor<TElement> any)
{
    for (auto it = source.begin(); it != source.end(); ++it)
    {
        if (any.pred(*it))
            return true;
    }
    return false;
}

template <typename TRange, typename TElement = TRange::value_type, typename TSubElement>
auto operator | (const TRange & source, linq::select_functor<TElement, TSubElement> sel)
{
    std::vector<TSubElement> source_selected;
    
    source_selected.reserve(source.size());
    for (auto it = source.begin(); it != source.end(); ++it)
    {
        source_selected.push_back(sel.pred(*it));
    }

    return source_selected;
}

template <typename TRange, typename TElement = TRange::value_type, typename TSubElementRange>
auto operator | (const TRange & source, linq::select_many_functor<TElement, TSubElementRange> sel)
{
    using TSubElement = TSubElementRange::value_type;
    std::vector<TSubElement> source_selected;

    for (auto it = source.begin(); it != source.end(); ++it)
    {
        auto sub_range = sel.pred(*it);
        source_selected.insert(source_selected.end(),
                               sub_range.begin(),
                               sub_range.end());
    }

    return source_selected;
}

template <typename TRange, typename TElement = TRange::value_type, typename TKey>
TRange operator | (const TRange & source, linq::order_by_functor<TElement, TKey> ord)
{
    TRange source_copy(source);
    std::sort(source_copy.begin(),
              source_copy.end(),
              [&ord] (TElement & left, TElement & right) -> bool
              {
                  return ord.pred(left) < ord.pred(right);
              });
    return source_copy;
}

template <typename TRange, typename TElement = TRange::value_type>
TRange operator | (const TRange & source, linq::reverse)
{
    return TRange(source.rbegin(), source.rend());
}

template <typename TRange, typename TElement = TRange::value_type>
auto operator | (const TRange & source, linq::distinct)
{
    return std::set<TElement>(source.begin(), source.end());
}

template <typename TRange, typename TElement = TRange::value_type, typename TSetRange>
auto operator | (const TRange & source, linq::except_functor<TSetRange> sr)
{
    std::set<TElement> source_set(source.begin(), source.end());
    std::set<TElement> result;
    std::set_difference(source_set.begin(), source_set.end(),
                        sr.range.begin(), sr.range.end(),
                        std::inserter(result, result.end()));
    return result;
}

template <typename TRange, typename TElement = TRange::value_type, typename TSetRange>
auto operator | (const TRange & source, linq::intersect_functor<TSetRange> sr)
{
    std::set<TElement> source_set(source.begin(), source.end());
    std::set<TElement> result;
    std::set_intersection(source_set.begin(), source_set.end(),
                          sr.range.begin(), sr.range.end(),
                          std::inserter(result, result.end()));
    return result;
}

template <typename TRange, typename TElement = TRange::value_type, typename TSetRange>
auto operator | (const TRange & source, linq::union_functor<TSetRange> sr)
{
    std::set<TElement> source_set(source.begin(), source.end());
    std::set<TElement> result;
    std::set_union(source_set.begin(), source_set.end(),
                   sr.range.begin(), sr.range.end(),
                   std::inserter(result, result.end()));
    return result;
}

template <typename TRange, typename TElement = TRange::value_type>
TRange operator | (const TRange & source, linq::skip ski)
{
    int n = ski.n;
    auto it = source.begin();
    while (it != source.end() && 0 < n)
        ++it, --n;
    return TRange(it, source.end());
}

template <typename TRange, typename TElement = TRange::value_type>
TRange operator | (const TRange & source, linq::skip_while_functor<TElement> ski)
{
    auto it = source.begin();
    while (it != source.end() && ski.pred(*it))
        ++it;
    return TRange(it, source.end());
}

template <typename TRange, typename TElement = TRange::value_type>
TRange operator | (const TRange & source, linq::take tak)
{
    int n = tak.n;
    auto it = source.begin();
    while (it != source.end() && 0 < n)
        ++it, --n;
    return TRange(source.begin(), it);
}

template <typename TRange, typename TElement = TRange::value_type>
TRange operator | (const TRange & source, linq::take_while_functor<TElement> tak)
{
    auto it = source.begin();
    while (it != source.end() && tak.pred(*it))
        ++it;
    return TRange(source.begin(), it);
}

template <typename TRangeX, typename TElementX = TRangeX::value_type,
    typename TRangeY, typename TElementY = TRangeY::value_type, typename TKey>
auto operator | (const TRangeX & source, linq::join_functor<TRangeY, TElementX, TElementY, TKey> joi)
{
    std::vector<std::pair<
        std::remove_reference<TElementX>::type,
        std::remove_reference<TElementY>::type>> result;
    for (auto it_x = source.begin(); it_x != source.end(); ++it_x)
    {
        for (auto it_y = joi.range.begin(); it_y != joi.range.end(); ++it_y)
        {
            if (joi.pred_x(*it_x) == joi.pred_y(*it_y))
            {
                result.emplace_back(*it_x, *it_y);
            }
        }
    }
    return result;
}

template <typename TRangeX, typename TElementX = TRangeX::value_type,
    typename TRangeY, typename TElementY = TRangeY::value_type, typename TKey>
auto operator | (const TRangeX & source, linq::group_join_functor<TRangeY, TElementX, TElementY, TKey> joi)
{
    using TElementXValue = std::remove_const<std::remove_reference<TElementX>::type>::type;
    using TElementYValue = std::remove_const<std::remove_reference<TElementY>::type>::type;

    std::map<
        TKey,
        std::pair<TElementXValue, std::vector<TElementYValue>>
    > result;

    for (auto it_x = source.begin(); it_x != source.end(); ++it_x)
    {
        for (auto it_y = joi.range.begin(); it_y != joi.range.end(); ++it_y)
        {
            auto key_x = joi.pred_x(*it_x);
            if (key_x == joi.pred_y(*it_y))
            {
                auto & p = result[key_x];
                p.first = *it_x;
                p.second.emplace_back(*it_y);
            }
        }
    }
    return result;
}

template <typename TRange, typename TElement = TRange::value_type>
TElement operator | (const TRange & source, linq::aggregate_functor<TElement> agg)
{
    for (const TElement & elem : source)
    {
        agg.add(elem);
    }
    return agg.get();
}

template <typename TRange, typename TElement = TRange::value_type>
TElement operator | (const TRange & source, linq::average)
{
    auto it = source.begin();
    auto end = source.end();

    int count = 1;
    TElement average = *it;
    while (++it != end)
    {
        average += *it;
        ++count;
    }

    return average / count;
}

template <typename TRange, typename TElement = TRange::value_type>
TElement operator | (const TRange & source, linq::count)
{
    auto it = source.begin();
    auto end = source.end();

    int count = 0;
    while (it != end)
    {
        ++it;
        ++count;
    }

    return count;
}

template <typename TRange, typename TElement = TRange::value_type>
TElement operator | (const TRange & source, linq::sum)
{
    auto it = source.begin();
    auto end = source.end();

    TElement s = TElement(0);
    while (it != end)
    {
        s += *it;
        ++it;
    }

    return s;
}

template <typename TRange, typename TElement = TRange::value_type>
TElement operator | (const TRange & source, linq::max)
{
    auto it = source.begin();
    auto end = source.end();

    TElement max_value = *it;
    while (++it != end)
    {
        auto & next_value = *it;
        max_value =
            max_value < next_value
            ? next_value
            : max_value;
    }

    return max_value;
}

template <typename TRange, typename TElement = TRange::value_type>
TElement operator | (const TRange & source, linq::min)
{
    auto it = source.begin();
    auto end = source.end();

    TElement min_value = *it;
    while (++it != end)
    {
        auto & next_value = *it;
        min_value =
            next_value < min_value
            ? next_value
            : min_value;
    }

    return min_value;
}
