#pragma once

#include <functional>
#include <algorithm>
#include <tuple>

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
// Aggregation
//
//  linq::aggregate(seed, pred)
//  linq::average()
//  linq::count()
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
struct max {};
struct min {};

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
