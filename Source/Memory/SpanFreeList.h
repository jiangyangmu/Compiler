#pragma once

#include "Span.h"

namespace LowLevel {

class SpanCtrlBlock;

// Manage spans with equal nPage.
class SpanFreeList
{
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
    class Position {
    public:
        explicit Position(Span ** ppsCurr);

        bool            HasValue() { return *pNextSpanAddr != nullptr; }
        // Assert: has value
        SpanDescriptor  GetValue();

        Position & operator ++ ();
        Position   operator ++ (int);

        bool  operator == (const Position & o) { return pNextSpanAddr == o.pNextSpanAddr; }
        bool  operator != (const Position & o) { return pNextSpanAddr != o.pNextSpanAddr; }

    private:
        Span * * pNextSpanAddr;

        friend class SpanFreeList;
    };

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
private:
    SpanFreeList(const SpanFreeList & o) = delete;
    SpanFreeList & operator = (const SpanFreeList & o) = delete;

public:
    ForwardIterator Begin() const { return ForwardIterator(psHead); }
    ForwardIterator End() const { return ForwardIterator(nullptr); }

    Position BeginPos();
    Position EndPos(); // O(n)

                       // Return: end pos, or pos to first span after _ps_
    Position FindPos(Span * ps);
    // Return: before pos
    // Assert: before pos exists (pos != begin pos)
    Position FindPosBefore(Position pos); // O(n)

    void    Insert(Span * ps);
    // Assert: pos has value
    Span *  Remove(Position & pos);
    Span *  Pop();

    bool    Empty() const;

private:
    Span * psHead;

    friend class SpanCtrlBlock;
};

// SpanFreeList + one commit-on-demand span (with equal nPage).
class LazySpanFreeList
{
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

public:
    LazySpanFreeList(void * pvLazySpan, size_t nLazySpanPage)
        : psHead(nullptr), pvLazySpan(pvLazySpan), nLazySpanPage(nLazySpanPage)
    {
    }
    LazySpanFreeList(const LazySpanFreeList & o) = delete;
    LazySpanFreeList & operator = (const LazySpanFreeList & o) = delete;
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

public:
    ForwardIterator Begin() const { return ForwardIterator(pvLazySpan, nLazySpanPage, psHead, true); }
    ForwardIterator End() const { return ForwardIterator(pvLazySpan, nLazySpanPage, psHead, false); }

    // Sorted by memory begin address.
    void    Insert(Span * ps);
    void    Insert(void * pvMemBegin, size_t nPage);
    Span *  Pop();
    bool    Empty() const;

    bool    HasLazySpan() const;

private:
    Span * psHead;
    void * pvLazySpan;
    size_t nLazySpanPage;

    friend class SpanCtrlBlock;
};

}
