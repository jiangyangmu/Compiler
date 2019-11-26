#pragma once

#include <functional>
#include <algorithm>

namespace linq {

// Excludes T from argument deduction.
template <typename T> struct no_arg_deduction { typedef T type; };

// ================================================
// Sorting
//
//  linq::order_by(pred)
//  linq::reverse()
//

template <typename TElement, typename TKey>
struct order_by
{
    order_by(std::function<TKey(TElement)> pred) : pred(pred) {}

    std::function<TKey(TElement)> pred;
};

template <typename TElement, typename TKey>
auto
order_by_func(std::function<TKey(TElement)> pred)
{
    return order_by<TElement, TKey>(pred);
}

// ================================================
// Aggregation
//
//  linq::aggregate(seed, pred)
//  linq::average()
//  linq::count()
//  linq::max()
//  linq::min()
//

template <typename T>
struct aggregator
{
    aggregator(T seed, std::function<T(T, T)> pred) : result(seed), pred(pred) {}
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
    return aggregator<TElement>(seed, pred);
}

struct average {};
struct count {};
struct max {};
struct min {};

}

template <typename TRange, typename TKey, typename TElement = TRange::value_type>
TRange operator | (const TRange & source, linq::order_by<TElement, TKey> ord)
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
TElement operator | (const TRange & source, linq::aggregator<TElement> agg)
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
