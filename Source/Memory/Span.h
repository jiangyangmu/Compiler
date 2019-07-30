#pragma once

#include "../Util/Integer.h"

namespace LowLevel {

constexpr size_t PAGE_SIZE = 4096;              // 4KB
constexpr int    PAGE_SIZE_BITS = 12;
//constexpr size_t NUM_PAGE_PER_SPAN = 16 * 1024; // 64MB

struct Span {
    Span * psNext;
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

    void    Insert(Span * ps);
    void    Insert(void * pvMemBegin, size_t nPage);
    Span *  Pop();
    bool    Empty() const;

private:
    SpanFreeList(const SpanFreeList & o) = delete;
    SpanFreeList & operator = (const SpanFreeList & o) = delete;

    Span * psHead;

    friend class SpanView;
    friend class SpanCtrlBlock; // TODO: remove
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

private:
    SpanAllocator() = default;
    SpanAllocator(const SpanAllocator &) = delete;
    SpanAllocator & operator = (const SpanAllocator &) = delete;

    SpanCtrlBlock * pscb;

    friend SpanAllocator CreateSpanAllocator(size_t nReservedPage);
    friend SpanAllocator CreateSpanAllocator(void * pvMemBegin, size_t nPage);
    friend class SpanView;
};

// nReservedPage must be power of 2, at least 16
SpanAllocator CreateSpanAllocator(size_t nReservedPage);
SpanAllocator CreateSpanAllocator(void * pvMemBegin, size_t nPage);

// Iterate Spans in SpanFreeList or SpanAllocator.
class SpanView {
public:
    class ForwardIterator {
    public:
        ForwardIterator();
        ForwardIterator(const SpanFreeList * psflBegin, const SpanFreeList * psflEnd, const SpanFreeList * psflCurr);
        
        ForwardIterator(const ForwardIterator &) = default;
        ForwardIterator(ForwardIterator &&) = default;
        ForwardIterator & operator = (const ForwardIterator &) = default;
        ForwardIterator & operator = (ForwardIterator &&) = default;

        ForwardIterator & operator ++ ();
        ForwardIterator   operator ++ (int);

        bool              operator == (const ForwardIterator & o);
        bool              operator != (const ForwardIterator & o);
        
        const Span *      operator * ();

    private:
        const SpanFreeList * cpsflBegin;
        const SpanFreeList * cpsflEnd;
        const SpanFreeList * cpsflCurr;
        const Span * cpsCurr;
    };

    SpanView() = default;
    SpanView(const SpanFreeList & sfl);
    SpanView(const SpanAllocator & sa);

    SpanView(const SpanView &) = default;
    SpanView(SpanView &&) = default;
    SpanView & operator = (const SpanView &) = default;
    SpanView & operator = (SpanView &&) = default;

    ForwardIterator begin() { return beginIterator; }
    ForwardIterator end() { return endIterator; }

private:
    ForwardIterator beginIterator;
    ForwardIterator endIterator;
};

}
