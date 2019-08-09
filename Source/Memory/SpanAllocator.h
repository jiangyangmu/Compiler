#pragma once

#include "SpanFreeList.h"

namespace LowLevel {

class SpanCtrlBlock;

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

    // Alloc/free span

    void *  Alloc(size_t nPage);
    void    Free(void * pvMemBegin, size_t nPage);

    // Query address space

    const void * AddrBegin() const;
    const void * AddrEnd() const;
    bool IsOwnerOf(const void * pvAddr) const;

    // Query page usage

    // All pages (Control + Allocable).
    size_t NumOfAllPages() const;
    // Control pages.
    size_t NumOfCtrlPages() const;
    // Allocable pages.
    size_t NumOfAllocablePages() const;
    size_t NumOfFreePages() const;
    size_t NumOfUsedPages() const;

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

        SpanDescriptor    operator * ();

    private:
        const SpanFreeList * cpsflBegin;
        const SpanFreeList * cpsflEnd;
        const SpanFreeList * cpsflCurr;
        SpanFreeList::ForwardIterator fiCurr;
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

}
