#include "SpanFreeList.h"

#include "Win/WinAllocate.h"

namespace LowLevel {

// ===========================================================================
// SpanFreeList: init, push, pop, empty
// ===========================================================================

SpanFreeList::Position
SpanFreeList::BeginPos()
{
    return Position(&psHead);
}

SpanFreeList::Position
SpanFreeList::EndPos()
{
    Position pos(&psHead);
    while (pos.HasPointedSpan())
        ++pos;
    return pos;
}

SpanFreeList::Position
SpanFreeList::FindPos(Span * ps)
{
    Position pos(&psHead);

    while (pos.HasPointedSpan() && pos.GetPointedSpan() < ps)
        ++pos;

    return pos;
}

SpanFreeList::Position
SpanFreeList::FindPosBefore(Position pos)
{
    ASSERT(psHead && pos != BeginPos());

    Span * ps;
    ps = pos.GetHostSpan();
    
    Position posBefore(&psHead);
    while (posBefore.HasPointedSpan() && posBefore.GetPointedSpan() != ps)
        ++posBefore;
    return posBefore;
}

void
SpanFreeList::Insert(Span * ps)
{
    ASSERT(ps);
    FindPos(ps).InsertSpan(ps);
}

void
SpanFreeList::Insert(void * pvSpanBegin, size_t nPage)
{
    ASSERT(pvSpanBegin);

    Span * ps;
    ps          = (Span *)pvSpanBegin;
    ps->psNext  = nullptr;
    ps->nPage   = nPage;

    FindPos(ps).InsertSpan(ps);
}

Span *
SpanFreeList::Remove(Position & pos)
{
    return pos.RemoveSpan();
}

Span *
SpanFreeList::Pop()
{
    ASSERT(psHead);
    Span * ps;
    ps      = psHead;
    psHead  = psHead->psNext;
    return ps;
}

bool
SpanFreeList::Empty() const
{
    return psHead == nullptr;
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
    : pNextSpanAddr{pNextSpanAddr}
{
    ASSERT(pNextSpanAddr);
}

bool
SpanFreeList::Position::HasPointedSpan() const
{
    return *pNextSpanAddr != nullptr;
}

Span *
SpanFreeList::Position::GetPointedSpan() const
{
    return *pNextSpanAddr;
}

Span *
SpanFreeList::Position::GetHostSpan()
{
    return (Span *)((char *)pNextSpanAddr - offsetof(Span, psNext));
}

void
SpanFreeList::Position::InsertSpan(Span * ps)
{
    ASSERT(ps);
    ps->psNext      = GetPointedSpan();
    *pNextSpanAddr  = ps;
}

Span *
SpanFreeList::Position::RemoveSpan()
{
    ASSERT(HasPointedSpan());
    Span * ps;
    ps              = GetPointedSpan();
    *pNextSpanAddr  = ps->psNext;
    ps->psNext      = nullptr;
    return ps;
}

SpanFreeList::Position &
SpanFreeList::Position::operator++()
{
    if (*pNextSpanAddr)
        pNextSpanAddr = (Span **)((char *)(*pNextSpanAddr) + offsetof(Span, psNext));
    return *this;
}

SpanFreeList::Position
SpanFreeList::Position::operator++(int)
{
    Position tmp(*this);
    operator++();
    return tmp;
}

}
