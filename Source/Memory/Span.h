#pragma once

#include "../Base/Integer.h"
#include "Address.h"

namespace LowLevel {

struct Span {
    Span * psNext;
    size_t nPage;
};

struct SpanDescriptor {
    const void * cpvMemBegin;
    size_t nPage;
};

class SpanCtrlBlock;

class SpanFreeList
{
public:
    SpanFreeList() : psHead(nullptr) {}
    SpanFreeList(SpanFreeList && o)
        : psHead(o.psHead)
    {
        o.psHead = nullptr;
    }
    SpanFreeList & operator = (SpanFreeList && o)
    {
        psHead = o.psHead;
        o.psHead = nullptr;
    }

    // Sorted by memory begin address.
    void    Insert(Span * ps);
    void    Insert(void * pvMemBegin, size_t nPage);
    Span *  Pop();
    bool    Empty() const;

public:
    class ForwardIterator {
    public:
        ForwardIterator();
        explicit ForwardIterator(const Span * cpsCurr);

        ForwardIterator(const ForwardIterator &) = default;
        ForwardIterator(ForwardIterator &&) = default;
        ForwardIterator & operator = (const ForwardIterator &) = default;
        ForwardIterator & operator = (ForwardIterator &&) = default;

        ForwardIterator & operator ++ ();
        ForwardIterator   operator ++ (int);

        bool              operator == (const ForwardIterator & o);
        bool              operator != (const ForwardIterator & o);

        SpanDescriptor    operator * ();

    private:
        const Span * cpsCurr;
    };

    ForwardIterator Begin() const { return ForwardIterator(psHead); }
    ForwardIterator End() const { return ForwardIterator(nullptr); }

private:
    SpanFreeList(const SpanFreeList & o) = delete;
    SpanFreeList & operator = (const SpanFreeList & o) = delete;

    Span * psHead;

    friend class SpanCtrlBlock;
};

class LazySpanFreeList
{
public:
    LazySpanFreeList(void * pvLazySpan, size_t nLazySpanPage)
        : psHead(nullptr), pvLazySpan(pvLazySpan), nLazySpanPage(nLazySpanPage)
    {
    }
    LazySpanFreeList(LazySpanFreeList && o)
        : psHead(o.psHead)
    {
        o.psHead = nullptr;
    }
    LazySpanFreeList & operator = (LazySpanFreeList && o)
    {
        psHead = o.psHead;
        o.psHead = nullptr;
    }

    // Sorted by memory begin address.
    void    Insert(Span * ps);
    void    Insert(void * pvMemBegin, size_t nPage);
    Span *  Pop();
    bool    Empty() const;

    bool    HasLazySpan() const;

public:
    class ForwardIterator {
    public:
        ForwardIterator();
        ForwardIterator(const void * cpvLazySpan, size_t nLazySpanPage, const Span * cpsHead, bool bBegin);

        ForwardIterator(const ForwardIterator &) = default;
        ForwardIterator(ForwardIterator &&) = default;
        ForwardIterator & operator = (const ForwardIterator &) = default;
        ForwardIterator & operator = (ForwardIterator &&) = default;

        ForwardIterator & operator ++ ();
        ForwardIterator   operator ++ (int);

        bool              operator == (const ForwardIterator & o);
        bool              operator != (const ForwardIterator & o);

        SpanDescriptor    operator * ();

    private:
        SpanDescriptor sdLazySpan;
        const Span * cpsHead;
        const Span * cpsCurr;
    };

    ForwardIterator Begin() const { return ForwardIterator(pvLazySpan, nLazySpanPage, psHead, true); }
    ForwardIterator End() const { return ForwardIterator(pvLazySpan, nLazySpanPage, psHead, false); }

private:
    LazySpanFreeList(const LazySpanFreeList & o) = delete;
    LazySpanFreeList & operator = (const LazySpanFreeList & o) = delete;

    Span * psHead;
    void * pvLazySpan;
    size_t nLazySpanPage;

    friend class SpanCtrlBlock;
};

// Front-end of SCB.
class SpanAllocator
{
public:
    SpanAllocator(SpanAllocator && other)
        : pscb(other.pscb)
    {
        other.pscb = nullptr;
    }
    SpanAllocator & operator = (SpanAllocator && other)
    {
        pscb = other.pscb;
        other.pscb = nullptr;
    }
    ~SpanAllocator();

    void *  Alloc(size_t nPage);
    void    Free(void * pvMemBegin, size_t nPage);

    const void * MemBegin() const;
    bool Contains(const void * pvAddr) const;

public:
    class ForwardIterator {
    public:
        ForwardIterator();
        ForwardIterator(const LazySpanFreeList * psflBegin, const LazySpanFreeList * psflEnd, const LazySpanFreeList * psflCurr);

        ForwardIterator(const ForwardIterator &) = default;
        ForwardIterator(ForwardIterator &&) = default;
        ForwardIterator & operator = (const ForwardIterator &) = default;
        ForwardIterator & operator = (ForwardIterator &&) = default;

        ForwardIterator & operator ++ ();
        ForwardIterator   operator ++ (int);

        bool              operator == (const ForwardIterator & o);
        bool              operator != (const ForwardIterator & o);

        SpanDescriptor    operator * ();

    private:
        const LazySpanFreeList * cpsflBegin;
        const LazySpanFreeList * cpsflEnd;
        const LazySpanFreeList * cpsflCurr;
        LazySpanFreeList::ForwardIterator fiCurr;
    };

    ForwardIterator Begin() const;
    ForwardIterator End() const;

private:
    SpanAllocator() = default;
    SpanAllocator(const SpanAllocator &) = delete;
    SpanAllocator & operator = (const SpanAllocator &) = delete;

    SpanCtrlBlock * pscb;

    friend SpanAllocator   CreateSpanAllocator(size_t nReservedPage);
    friend SpanAllocator   CreateSpanAllocator(void * pvMemBegin, size_t nPage);
    friend SpanAllocator * GetDefaultSpanAllocator();
};

// constexpr size_t DEFAULT_NUM_PAGE_PER_SPAN = 16 * 1024; // 64MB
constexpr size_t DEFAULT_NUM_PAGE_PER_SPAN = 4; // 1MB

// nReservedPage must be power of 2, at least 16
SpanAllocator   CreateSpanAllocator(size_t nReservedPage);
SpanAllocator   CreateSpanAllocator(void * pvMemBegin, size_t nPage);
SpanAllocator * GetDefaultSpanAllocator();

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
