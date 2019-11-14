#pragma once

#include "Span.h"

namespace memory {

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
        explicit Position(Span ** pNextSpanAddr);

        bool   HasPointedSpan() const;
        Span * GetPointedSpan() const;
        Span * GetHostSpan(); // not on psHead

        void   InsertSpan(Span * ps);
        // Assert: has pointed span
        Span * RemoveSpan();

        Position & operator ++ ();
        Position   operator ++ (int);

        bool  operator == (const Position & o)
        {
            return pNextSpanAddr == o.pNextSpanAddr;
        }
        bool  operator != (const Position & o)
        {
            return pNextSpanAddr != o.pNextSpanAddr;
        }

    private:
        Span ** pNextSpanAddr;
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
    void    Insert(void * pvSpanBegin, size_t nPage);
    // Assert: pos has value
    Span *  Remove(Position & pos);
    Span *  Pop();

    bool    Empty() const;

private:
    Span * psHead;

    friend class SpanCtrlBlock;
};

}
