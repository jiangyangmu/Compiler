#pragma once

#include "../Base/Integer.h"
#include "Address.h"

namespace memory {

struct Span {
    Span * psNext;
    size_t nPage;
};

Span * SplitSpan(Span * ps);
bool   CanMergeSpan(Span * psLeft, Span * psRight);
Span * MergeSpan(Span * psLeft, Span * psRight);

struct SpanDescriptor {
    const void * cpvMemBegin;
    size_t nPage;
};

// Adapter. T = {SpanFreeList, LazySpanFreeList, SpanAllocator}
template <typename T>
class SpanView {
public:
    SpanView() = default;
    SpanView(const T & obj) : beginIterator(obj.Begin()), endIterator(obj.End()) {}

    SpanView(const SpanView &) = default;
    SpanView(SpanView &&) = default;
    SpanView & operator = (const SpanView &) = default;
    SpanView & operator = (SpanView &&) = default;

    typename T::ForwardIterator begin() { return beginIterator; }
    typename T::ForwardIterator end() { return endIterator; }

private:
    typename T::ForwardIterator beginIterator;
    typename T::ForwardIterator endIterator;
};

}
