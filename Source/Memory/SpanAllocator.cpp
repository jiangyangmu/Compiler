#include "SpanAllocator.h"

#include "Win/Allocate.h"

namespace LowLevel {

#define PAGE_BEGIN(base, index) \
    (void *)((char *)(base) + (index) * PAGE_SIZE)

// ===========================================================================
// SpanCtrlBlock: init, alloc, free
// ===========================================================================

// Span + Alloc/Free (buddy alogrithm)
class SpanCtrlBlock
{
public:
    ~SpanCtrlBlock()
    {
        if (bOwnMemory)
        {
            DecommitPage(MemBegin(), nTotalPage);
        }
    }

    void * Alloc(size_t nPage);
    void   Free(void * pvMemBegin, size_t nPage);

    // Helper for Alloc/Free.

    // Return first SFL with >=nPage Span, or nullptr.
    SpanFreeList * FindFreeList(size_t nPage);

    void * MemBegin()
    {
        return (void *)((char *)this - (nTotalPage - 1) * PAGE_SIZE);
    }
    void * MemEnd()
    {
        return (void *)((char *)this + PAGE_SIZE);
    }

private:
    // Memory
    size_t nTotalPage;
    bool bOwnMemory;

    // 2^12, 2^13, ..., 2^25 byte
    // 1,    2,    ..., 8192 page
    // 4KB,  8KB,  ..., 32MB
    size_t nSpanFreeList;
    SpanFreeList vsfl[14];

    friend class SpanAllocator; // TODO - SpanCtrlBlock: remove friend SpanAllocator
    friend SpanCtrlBlock * CreateSpanCtrlBlock(void * pvMemBegin, size_t nPage, bool bOwnMemory);
};

SpanCtrlBlock *
CreateSpanCtrlBlock(void * pvMemBegin, size_t nPage, bool bOwnMemory)
{
    SpanCtrlBlock * pscb;

    // Init SCB.

    pscb = (SpanCtrlBlock *)PAGE_BEGIN(pvMemBegin, nPage - 1);
    pscb->nTotalPage = nPage;
    pscb->bOwnMemory = bOwnMemory;
    pscb->nSpanFreeList = IntLog2(nPage);

    // Init free lists.

    SpanFreeList * psfl;
    SpanFreeList * psflEnd;
    size_t nSpanPage;

    psfl = pscb->vsfl;
    psflEnd = pscb->vsfl + pscb->nSpanFreeList;
    nSpanPage = 1;

    for (;
         psfl < psflEnd;
         ++psfl, nSpanPage <<= 1)
    {
        new (psfl) SpanFreeList();
        psfl->Insert(
            PAGE_BEGIN(pvMemBegin, nPage - (nSpanPage << 1)),
            nSpanPage
        );
    }

    return pscb;
}

SpanFreeList *
SpanCtrlBlock::FindFreeList(size_t nPage)
{
    SpanFreeList * psfl;
    SpanFreeList * psflEnd;

    psfl = vsfl + IntLog2(nPage);
    psflEnd = vsfl + nSpanFreeList;

    while (psfl->Empty() && psfl < psflEnd)
        ++psfl;

    return psfl != psflEnd ? psfl : nullptr;
}

// nPage must be power of 2
void *
SpanCtrlBlock::Alloc(size_t nPage)
{
    // L Find-Pop A, (Split AB, L Down, L Insert B)*, A

    SpanFreeList * psfl;
    Span * ps;
    Span * psSecond;

    psfl = FindFreeList(nPage);
    if (psfl == nullptr)
        return nullptr;

    ps = psfl->Pop();
    // psfl: N-Page Span Free List
    // ps:   Free N-Page Span

    while (nPage < ps->nPage)
    {
        psSecond = SplitSpan(ps);

        --psfl;
        psfl->Insert(psSecond);

        // psfl: N-Page Span Free List
        // ps:   Free N-Page Span
    }

    return (void *)ps;
}

static inline
bool
IsEvenSpan(void * pvFirstPage, void * pvCurrPage, size_t nPage)
{
    return (((((char *)pvCurrPage - (char *)pvFirstPage) >> PAGE_SIZE_BITS) >> IntLog2(nPage)) & 1) == 0;
}

// nPage must be power of 2
void
SpanCtrlBlock::Free(void * pvMemBegin, size_t nPage)
{
    // A: edge to last smaller
    // B: edge to first larger
    // L Find A/AB, Merge AI, Extract AI => I, L Up, ...
    // L Find B/AB, Merge IB, Extract IB => I, L Up, ...
    // L Find /A/B/AB, Insert I

    SpanFreeList * psfl;

    ASSERT(PageBegin(pvMemBegin) == pvMemBegin);
    ASSERT(MemBegin() <= pvMemBegin && pvMemBegin < MemEnd());

    psfl = vsfl + IntLog2(nPage);
    if (psfl->Empty())
    {
        psfl->Insert(pvMemBegin, nPage);
        return;
    }

    Span * ps;

    ps = (Span *)pvMemBegin;
    ps->nPage = nPage;
    ps->psNext = nullptr;

    SpanFreeList::ForwardIterator fiLastLess;
    SpanFreeList::ForwardIterator fiFirstGreater;

    while (true)
    {
        // psfl: Non-empty N-Page Span Free List
        // ps:   Free N-Page Span

        fiLastLess = psfl->End();
        fiFirstGreater = psfl->Begin();
        while (fiFirstGreater != psfl->End() && (*fiFirstGreater).cpvMemBegin < pvMemBegin)
        {
            fiLastLess = fiFirstGreater;
            ++fiFirstGreater;
        }

        if (fiLastLess != psfl->End() &&
            !IsEvenSpan(MemBegin(), pvMemBegin, ps->nPage) &&
            (ps = MergeSpan((Span *)(*fiLastLess).cpvMemBegin, ps)) != nullptr)
        {
            // Extract merged span.
            Span * psLastLess;
            Span ** ppsLastLess;

            psLastLess = (Span *)(*fiLastLess).cpvMemBegin;
            for (ppsLastLess = &psfl->psHead;
                 *ppsLastLess != psLastLess;
                 ppsLastLess = &(*ppsLastLess)->psNext)
            {
            }
            *ppsLastLess = psLastLess->psNext;
        }
        else if (fiFirstGreater != psfl->End() &&
                 IsEvenSpan(MemBegin(), pvMemBegin, ps->nPage) &&
                 (ps = MergeSpan(ps, (Span *)(*fiFirstGreater).cpvMemBegin)) != nullptr)
        {
            // Extract merged span.
            Span * psFirstGreater;
            Span ** ppsFirstGreater;

            psFirstGreater = (Span *)(*fiFirstGreater).cpvMemBegin;
            for (ppsFirstGreater = &psfl->psHead;
                 *ppsFirstGreater != psFirstGreater;
                 ppsFirstGreater = &(*ppsFirstGreater)->psNext)
            {
            }
            *ppsFirstGreater = psFirstGreater->psNext;
        }
        else
            break;

        ++psfl;
    }

    if (fiFirstGreater == psfl->End())
    {
        ASSERT(psfl->psHead == nullptr);
        psfl->psHead = ps;
        ps->psNext = nullptr;
    }
    else
    {
        Span ** ppsFirstGreater;
        for (ppsFirstGreater = &psfl->psHead;
             *ppsFirstGreater != (Span *)(*fiFirstGreater).cpvMemBegin;
             ppsFirstGreater = &(*ppsFirstGreater)->psNext)
        {
        }
        ps->psNext = *ppsFirstGreater;
        *ppsFirstGreater = ps;
    }
}


// ===========================================================================
// SpanAllocator
// ===========================================================================

SpanAllocator CreateSpanAllocator(size_t nReservedPage)
{
    SpanAllocator sa;

    sa.pscb =
        CreateSpanCtrlBlock(
            ReserveAddressSpaceAndCommitPages(nReservedPage),
            nReservedPage,
            true // bOwnMemory
        );

    return sa;
}
SpanAllocator CreateSpanAllocator(void * pvMemBegin, size_t nPage)
{
    SpanAllocator sa;

    sa.pscb =
        CreateSpanCtrlBlock(
            pvMemBegin,
            nPage,
            false // bOwnMemory
        );

    return sa;
}

SpanAllocator * GetDefaultSpanAllocator()
{
    static SpanAllocator sa;

    if (sa.pscb == nullptr)
    {
        new (&sa) SpanAllocator(
            CreateSpanAllocator(DEFAULT_NUM_PAGE_PER_SPAN)
        );
        ASSERT(sa.pscb);
    }

    return &sa;
}

SpanAllocator::~SpanAllocator()
{
    if (pscb)
        pscb->~SpanCtrlBlock();
}

void *
SpanAllocator::Alloc(size_t nPage)
{
    return pscb->Alloc(nPage);
}

void
SpanAllocator::Free(void * pvMemBegin, size_t nPage)
{
    return pscb->Free(pvMemBegin, nPage);
}

const void *
SpanAllocator::AddrBegin() const
{
    return pscb->MemBegin();
}

const void *
SpanAllocator::AddrEnd() const
{
    return pscb->MemEnd();
}

bool
SpanAllocator::IsOwnerOf(const void * pvAddr) const
{
    return pscb->MemBegin() <= pvAddr && pvAddr < pscb->MemEnd();
}

size_t
SpanAllocator::NumOfAllPages() const
{
    return pscb->nTotalPage;
}

size_t
SpanAllocator::NumOfCtrlPages() const
{
    return 1;
}

size_t
SpanAllocator::NumOfAllocablePages() const
{
    ASSERT(1 < pscb->nTotalPage);
    return pscb->nTotalPage - 1;
}

size_t
SpanAllocator::NumOfFreePages() const
{
    // TODO:
    ASSERT(false);
    return 0;
}

size_t
SpanAllocator::NumOfUsedPages() const
{
    // TODO:
    ASSERT(false);
    return 0;
}


// ===========================================================================
// SpanAllocator::ForwardIterator
// ===========================================================================

SpanAllocator::ForwardIterator::ForwardIterator()
    : cpsflBegin(nullptr), cpsflEnd(nullptr), cpsflCurr(nullptr), fiCurr()
{
}

SpanAllocator::ForwardIterator::ForwardIterator(const SpanFreeList * psflBegin, const SpanFreeList * psflEnd, const SpanFreeList * psflCurr)
    : cpsflBegin(psflBegin), cpsflEnd(psflEnd), cpsflCurr(psflCurr)
{
    fiCurr = cpsflCurr->Begin();
    while (fiCurr == cpsflCurr->End() &&
           cpsflCurr != cpsflEnd)
    {
        ++cpsflCurr, fiCurr = cpsflCurr->Begin();
    }
}

SpanAllocator::ForwardIterator &
SpanAllocator::ForwardIterator::operator ++ ()
{
    if (fiCurr != cpsflCurr->End())
    {
        ++fiCurr;
        while (fiCurr == cpsflCurr->End() &&
               cpsflCurr != cpsflEnd)
        {
            ++cpsflCurr, fiCurr = cpsflCurr->Begin();
        }
    }
    return *this;
}

SpanAllocator::ForwardIterator
SpanAllocator::ForwardIterator::operator ++ (int)
{
    ForwardIterator tmp(*this);
    operator++();
    return tmp;
}

bool
SpanAllocator::ForwardIterator::operator == (const ForwardIterator & o)
{
    ASSERT(cpsflBegin == o.cpsflBegin && cpsflEnd == o.cpsflEnd);
    return cpsflCurr == o.cpsflCurr && fiCurr == o.fiCurr;
}

bool
SpanAllocator::ForwardIterator::operator != (const ForwardIterator & o)
{
    return !operator==(o);
}

SpanDescriptor
SpanAllocator::ForwardIterator::operator * ()
{
    ASSERT(fiCurr != cpsflCurr->End());
    return *fiCurr;
}

SpanAllocator::ForwardIterator
SpanAllocator::Begin() const
{
    return ForwardIterator(pscb->vsfl, pscb->vsfl + pscb->nSpanFreeList, pscb->vsfl);
}

SpanAllocator::ForwardIterator
SpanAllocator::End() const
{
    return ForwardIterator(pscb->vsfl, pscb->vsfl + pscb->nSpanFreeList, pscb->vsfl + pscb->nSpanFreeList);
}

}
