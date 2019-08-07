#include "SpanFreeList.h"

namespace LowLevel {

// ===========================================================================
// SpanFreeList: init, push, pop, empty
// LazySpanFreeList: init, push, pop, empty
// ===========================================================================

SpanFreeList::Position
SpanFreeList::BeginPos()
{
    return Position(&psHead);
}

SpanFreeList::Position
SpanFreeList::EndPos()
{
    Span ** pNextSpanAddr;

    pNextSpanAddr = &psHead;
    while (*pNextSpanAddr)
        pNextSpanAddr = &(*pNextSpanAddr)->psNext;

    return Position(pNextSpanAddr);
}

SpanFreeList::Position
SpanFreeList::FindPos(Span * ps)
{
    Span ** pNextSpanAddr;

    pNextSpanAddr = &psHead;
    while (*pNextSpanAddr && *pNextSpanAddr < ps)
        pNextSpanAddr = &(*pNextSpanAddr)->psNext;

    return Position(pNextSpanAddr);
}

SpanFreeList::Position
SpanFreeList::FindPosBefore(Position pos)
{
    ASSERT(psHead && pos != BeginPos());

    Span ** pNextSpanAddr;
    Span * ps;

    ps = SpanStart(pos.pNextSpanAddr);

    pNextSpanAddr = &psHead;
    while (*pNextSpanAddr != ps)
        pNextSpanAddr = &(*pNextSpanAddr)->psNext;

    return Position(pNextSpanAddr);
}

void
SpanFreeList::Insert(Span * span)
{
    ASSERT(span);

    Position pos = FindPos(span);
    span->psNext = *pos.pNextSpanAddr;
    *pos.pNextSpanAddr = span;
}

Span *
SpanFreeList::Remove(Position & pos)
{
    ASSERT(pos.HasValue());

    Span * ps;
    ps = *pos.pNextSpanAddr;
    *pos.pNextSpanAddr = ps->psNext;
    return ps;
}

Span *
SpanFreeList::Pop()
{
    ASSERT(psHead);
    Span * ps;
    ps = psHead;
    psHead = psHead->psNext;
    return ps;
}

bool
SpanFreeList::Empty() const
{
    return psHead == nullptr;
}


void
LazySpanFreeList::Insert(Span * ps)
{
    Span ** ppsInsert;

    ppsInsert = &psHead;
    while (*ppsInsert != nullptr && *ppsInsert < ps)
    {
        ppsInsert = &(*ppsInsert)->psNext;
    }
    ps->psNext = *ppsInsert;
    *ppsInsert = ps;
}

void
LazySpanFreeList::Insert(void * pvMemBegin, size_t nPage)
{
    Span * ps;
    ps = (Span *)pvMemBegin;
    ps->nPage = nPage;
    Insert(ps);
}

void
CommitPage(void * pvMemBegin, size_t nPage);
Span *
LazySpanFreeList::Pop()
{
    if (!psHead)
    {
        ASSERT(pvLazySpan);

        CommitPage(pvLazySpan, nLazySpanPage);

        psHead = (Span *)pvLazySpan;
        psHead->psNext = nullptr;
        psHead->nPage = nLazySpanPage;

        pvLazySpan = nullptr;
        nLazySpanPage = 0;
    }

    Span * ps;
    ps = psHead;
    psHead = psHead->psNext;
    return ps;
}

bool
LazySpanFreeList::Empty() const
{
    return psHead == nullptr && pvLazySpan == nullptr;
}

bool LazySpanFreeList::HasLazySpan() const
{
    return pvLazySpan != nullptr;
}

// ===========================================================================
// SpanFreeList::ForwardIterator
// LazySpanFreeList::ForwardIterator
// ===========================================================================

SpanFreeList::ForwardIterator::ForwardIterator()
    : cpsCurr(nullptr)
{
}

SpanFreeList::ForwardIterator::ForwardIterator(const Span * cpsCurr)
    : cpsCurr(cpsCurr)
{
}

SpanFreeList::ForwardIterator &
SpanFreeList::ForwardIterator::operator ++ ()
{
    if (cpsCurr)
        cpsCurr = cpsCurr->psNext;
    return *this;
}

SpanFreeList::ForwardIterator
SpanFreeList::ForwardIterator::operator ++ (int)
{
    ForwardIterator tmp(*this);
    operator++();
    return tmp;
}

bool
SpanFreeList::ForwardIterator::operator == (const ForwardIterator & o)
{
    return cpsCurr == o.cpsCurr;
}

bool
SpanFreeList::ForwardIterator::operator != (const ForwardIterator & o)
{
    return !operator==(o);
}

SpanDescriptor
SpanFreeList::ForwardIterator::operator * ()
{
    ASSERT(cpsCurr);
    SpanDescriptor sd;
    sd.cpvMemBegin = cpsCurr;
    sd.nPage = cpsCurr->nPage;
    return sd;
}

SpanFreeList::Position::Position(Span ** pNextSpanAddr)
    : pNextSpanAddr(pNextSpanAddr)
{
    ASSERT(pNextSpanAddr);
}

SpanDescriptor
SpanFreeList::Position::GetValue()
{
    ASSERT(HasValue());
    SpanDescriptor sd;
    sd.cpvMemBegin = *pNextSpanAddr;
    sd.nPage = (*pNextSpanAddr)->nPage;
    return sd;
}

SpanFreeList::Position &
SpanFreeList::Position::operator++()
{
    if (*pNextSpanAddr)
        pNextSpanAddr = &(*pNextSpanAddr)->psNext;
    return *this;
}

SpanFreeList::Position
SpanFreeList::Position::operator++(int)
{
    Position tmp(*this);
    operator++();
    return tmp;
}


LazySpanFreeList::ForwardIterator::ForwardIterator()
    : cpsHead(nullptr), cpsCurr(nullptr)
{
    sdLazySpan.cpvMemBegin = nullptr;
    sdLazySpan.nPage = 0;
}

LazySpanFreeList::ForwardIterator::ForwardIterator(const void * cpvLazySpan, size_t nLazySpanPage, const Span * cpsHead, bool bBegin)
    : cpsHead(cpsHead)
{
    sdLazySpan.cpvMemBegin = cpvLazySpan;
    sdLazySpan.nPage = cpvLazySpan ? nLazySpanPage : 0;
    this->cpsCurr = bBegin
        ? (cpvLazySpan ? (const Span *)cpvLazySpan : cpsHead)
        : nullptr;
}

LazySpanFreeList::ForwardIterator &
LazySpanFreeList::ForwardIterator::operator ++ ()
{
    if (cpsCurr == (const Span *)sdLazySpan.cpvMemBegin)
        cpsCurr = cpsHead;
    else if (cpsCurr != nullptr)
        cpsCurr = cpsCurr->psNext;
    return *this;
}

LazySpanFreeList::ForwardIterator
LazySpanFreeList::ForwardIterator::operator ++ (int)
{
    ForwardIterator tmp(*this);
    operator++();
    return tmp;
}

bool
LazySpanFreeList::ForwardIterator::operator == (const ForwardIterator & o)
{
    ASSERT(sdLazySpan.cpvMemBegin == o.sdLazySpan.cpvMemBegin && cpsHead == o.cpsHead);
    return cpsCurr == o.cpsCurr;
}

bool
LazySpanFreeList::ForwardIterator::operator != (const ForwardIterator & o)
{
    return !operator==(o);
}

SpanDescriptor
LazySpanFreeList::ForwardIterator::operator * ()
{
    ASSERT(cpsCurr);
    if (cpsCurr == (const Span *)sdLazySpan.cpvMemBegin)
    {
        return sdLazySpan;
    }
    else
    {
        SpanDescriptor sd;
        sd.cpvMemBegin = cpsCurr;
        sd.nPage = cpsCurr->nPage;
        return sd;
    }
}

}
