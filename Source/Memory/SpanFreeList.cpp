#include "SpanFreeList.h"

#include "Win/WinAllocate.h"

namespace memory {

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


#ifdef UNIT_TEST
#include "../UnitTest/UnitTest.h"

using namespace memory;

std::ostream & operator << (std::ostream & o, const SpanFreeList::ForwardIterator &) { return o; }
std::ostream & operator << (std::ostream & o, const SpanFreeList::Position & p)
{
    if (p.HasPointedSpan())
        o << p.GetPointedSpan() << " " << p.GetPointedSpan()->nPage;
    else
        o << "null";
    return o;
}

TEST(SpanFreeList_Create)
{
    SpanFreeList sfl;

    auto begin = sfl.Begin();
    auto end = sfl.End();
    EXPECT_EQ(begin, end);

    auto beginPos = sfl.BeginPos();
    auto endPos = sfl.EndPos();
    EXPECT_EQ(beginPos, endPos);
    EXPECT_EQ(beginPos.HasPointedSpan(), false);
    EXPECT_EQ(endPos.HasPointedSpan(), false);

    EXPECT_EQ(sfl.Empty(), true);
}

TEST(SpanFreeList_BeginEnd)
{
    SpanFreeList sfl;

    Span s1;
    {
        s1.nPage = 1;
        s1.psNext = nullptr;
        sfl.Insert(&s1);

        auto begin = sfl.Begin();
        auto end = sfl.End();

        EXPECT_EQ((*begin).cpvMemBegin, &s1);
        EXPECT_EQ((*begin).nPage, 1);

        ++begin;
        EXPECT_EQ(begin, end);
    }

    Span s2;
    {
        s2.nPage = 2;
        s2.psNext = nullptr;
        sfl.Insert(&s2);

        auto begin = sfl.Begin();
        auto end = sfl.End();

        EXPECT_EQ((*begin).cpvMemBegin, &s1);
        EXPECT_EQ((*begin).nPage, 1);

        ++begin;
        EXPECT_EQ((*begin).cpvMemBegin, &s2);
        EXPECT_EQ((*begin).nPage, 2);

        ++begin;
        EXPECT_EQ(begin, end);
    }
}

TEST(SpanFreeList_BeginEndFindPos)
{
    SpanFreeList sfl;

    Span s1;
    {
        s1.nPage = 1;
        s1.psNext = nullptr;

        sfl.Insert(&s1);

        auto beginPos = sfl.BeginPos();
        auto endPos = sfl.EndPos();

        EXPECT_EQ(beginPos.HasPointedSpan(), true);
        EXPECT_EQ(beginPos.GetPointedSpan(), &s1);
        EXPECT_EQ(beginPos.GetPointedSpan()->nPage, 1);
        EXPECT_EQ(sfl.FindPos(&s1), beginPos);

        ++beginPos;
        EXPECT_EQ(beginPos, endPos);
    }

    Span s2;
    {
        s2.nPage = 2;
        s2.psNext = nullptr;
        sfl.Insert(&s2);

        auto beginPos = sfl.BeginPos();
        auto endPos = sfl.EndPos();

        EXPECT_EQ(beginPos.HasPointedSpan(), true);
        EXPECT_EQ(beginPos.GetPointedSpan(), &s1);
        EXPECT_EQ(beginPos.GetPointedSpan()->nPage, 1);
        EXPECT_EQ(sfl.FindPos(&s1), beginPos);
        EXPECT_EQ(sfl.FindPosBefore(sfl.FindPos(&s2)), beginPos);

        ++beginPos;
        EXPECT_EQ(beginPos.HasPointedSpan(), true);
        EXPECT_EQ(beginPos.GetPointedSpan(), &s2);
        EXPECT_EQ(beginPos.GetPointedSpan()->nPage, 2);
        EXPECT_EQ(sfl.FindPos(&s2), beginPos);

        ++beginPos;
        EXPECT_EQ(beginPos, endPos);
    }
}

TEST(SpanFreeList_InsertRemovePop)
{
    SpanFreeList sfl;

    Span s1;
    s1.nPage = 1;
    s1.psNext = nullptr;

    Span s2;
    s2.nPage = 2;
    s2.psNext = nullptr;

    Span s3;
    s3.nPage = 4;
    s3.psNext = nullptr;

    Span s4;
    s4.nPage = 8;
    s4.psNext = nullptr;

    sfl.Insert(&s1);
    sfl.Insert(&s2);
    sfl.Insert(&s3);
    sfl.Insert(&s4);

    {
        Span * ps = sfl.Remove(sfl.FindPos(&s2));
        EXPECT_EQ(ps, &s2);

        auto begin = sfl.Begin();
        auto end = sfl.End();

        EXPECT_EQ((*begin).cpvMemBegin, &s1);
        EXPECT_EQ((*begin).nPage, 1);

        ++begin;
        EXPECT_EQ((*begin).cpvMemBegin, &s3);
        EXPECT_EQ((*begin).nPage, 4);

        ++begin;
        EXPECT_EQ((*begin).cpvMemBegin, &s4);
        EXPECT_EQ((*begin).nPage, 8);

        ++begin;
        EXPECT_EQ(begin, end);
    }

    {
        Span * ps = sfl.Remove(sfl.FindPos(&s4));
        EXPECT_EQ(ps, &s4);

        auto begin = sfl.Begin();
        auto end = sfl.End();

        EXPECT_EQ((*begin).cpvMemBegin, &s1);
        EXPECT_EQ((*begin).nPage, 1);

        ++begin;
        EXPECT_EQ((*begin).cpvMemBegin, &s3);
        EXPECT_EQ((*begin).nPage, 4);

        ++begin;
        EXPECT_EQ(begin, end);
    }

    {
        Span * ps = sfl.Remove(sfl.FindPos(&s1));
        EXPECT_EQ(ps, &s1);

        auto begin = sfl.Begin();
        auto end = sfl.End();

        EXPECT_EQ((*begin).cpvMemBegin, &s3);
        EXPECT_EQ((*begin).nPage, 4);

        ++begin;
        EXPECT_EQ(begin, end);
    }

    {
        Span * ps = sfl.Remove(sfl.FindPos(&s3));
        EXPECT_EQ(ps, &s3);

        auto begin = sfl.Begin();
        auto end = sfl.End();

        EXPECT_EQ(begin, end);
        EXPECT_EQ(sfl.Empty(), true);
    }

    sfl.Insert(&s1);
    sfl.Insert(&s2);
    sfl.Insert(&s3);
    sfl.Insert(&s4);

    {
        Span * ps = sfl.Pop();
        EXPECT_EQ(ps, &s1);

        auto begin = sfl.Begin();
        auto end = sfl.End();

        EXPECT_EQ((*begin).cpvMemBegin, &s2);
        EXPECT_EQ((*begin).nPage, 2);

        ++begin;
        EXPECT_EQ((*begin).cpvMemBegin, &s3);
        EXPECT_EQ((*begin).nPage, 4);

        ++begin;
        EXPECT_EQ((*begin).cpvMemBegin, &s4);
        EXPECT_EQ((*begin).nPage, 8);

        ++begin;
        EXPECT_EQ(begin, end);
    }

    {
        Span * ps = sfl.Pop();
        EXPECT_EQ(ps, &s2);

        auto begin = sfl.Begin();
        auto end = sfl.End();

        EXPECT_EQ((*begin).cpvMemBegin, &s3);
        EXPECT_EQ((*begin).nPage, 4);

        ++begin;
        EXPECT_EQ((*begin).cpvMemBegin, &s4);
        EXPECT_EQ((*begin).nPage, 8);

        ++begin;
        EXPECT_EQ(begin, end);
    }

    {
        Span * ps = sfl.Pop();
        EXPECT_EQ(ps, &s3);

        auto begin = sfl.Begin();
        auto end = sfl.End();

        EXPECT_EQ((*begin).cpvMemBegin, &s4);
        EXPECT_EQ((*begin).nPage, 8);

        ++begin;
        EXPECT_EQ(begin, end);
    }

    {
        Span * ps = sfl.Pop();
        EXPECT_EQ(ps, &s4);

        auto begin = sfl.Begin();
        auto end = sfl.End();

        EXPECT_EQ(begin, end);
        EXPECT_EQ(sfl.Empty(), true);
    }
}

#endif