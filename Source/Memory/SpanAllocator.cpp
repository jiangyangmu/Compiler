#include "SpanAllocator.h"

#include <windows.h>
#include <tchar.h>
#include <cstdio>
#include <cassert>

namespace LowLevel {

#define PAGE_BEGIN(base, index) \
    (void *)((char *)(base) + (index) * PAGE_SIZE)

// ===========================================================================
// AddressSpace: reserve, release
// Page: commit, decommit
// ===========================================================================

VOID
ErrorExit(const char * lpMsg)
{
    printf(("Error! %s with error code of %ld.\n"),
           lpMsg, GetLastError());
    exit(0);
}

static
void *
ReserveAddressSpace(size_t nReservedPage)
{
    printf(("ReserveAddressSpace: %zd pages. "), nReservedPage);

    LPVOID lpvBase;
    lpvBase = VirtualAlloc(
        NULL,                       // System selects address
        nReservedPage * PAGE_SIZE,  // Size of allocation
        MEM_RESERVE,                // Allocate reserved pages
        PAGE_NOACCESS);             // Protection = no access
    if (lpvBase == NULL)
        ErrorExit(("ReserveAddressSpace failed."));

    printf("%p\n", lpvBase);

    return lpvBase;
}

static
void *
ReserveAddressSpaceAndCommitPages(size_t nReservedPage)
{
    printf(("ReserveAddressSpaceAndCommitPages: %zd pages. "), nReservedPage);

    LPVOID lpvBase;
    lpvBase = VirtualAlloc(
        NULL,                       // System selects address
        nReservedPage * PAGE_SIZE,  // Size of allocation
        MEM_RESERVE | MEM_COMMIT,
        PAGE_READWRITE);
    if (lpvBase == NULL)
        ErrorExit(("ReserveAddressSpaceAndCommitPages failed."));

    printf("%p\n", lpvBase);

    return lpvBase;
}

// Also decommits all pages.
static
void
ReleaseAddressSpace(void * pvMemBegin, size_t nReservedPage)
{
    printf(("ReleasePage: %p %zd pages.\n"), pvMemBegin, nReservedPage);

    BOOL bSuccess;
    bSuccess = VirtualFree(
        pvMemBegin,    // Base address of block
        0,             // Bytes of committed pages
        MEM_RELEASE);  // Decommit the pages
    if (!bSuccess)
        ErrorExit(("ReleasePage failed.\n"));
}

void
CommitPage(void * pvMemBegin, size_t nPage)
{
    printf(("CommitPage: %p %zd pages.\n"), pvMemBegin, nPage);

    LPVOID lpvResult;
    lpvResult = VirtualAlloc(
        pvMemBegin,         // Next page to commit
        nPage * PAGE_SIZE,  // Page size, in bytes
        MEM_COMMIT,         // Allocate a committed page
        PAGE_READWRITE);    // Read/write access
    if (lpvResult == NULL)
        ErrorExit(("CommitPage failed.\n"));
}

// Safe to call over uncommited pages.
static
void
DecommitPage(void * pvMemBegin, size_t nPage)
{
    printf(("DecommitPage: %p %zd pages.\n"), pvMemBegin, nPage);

    BOOL bSuccess;
    bSuccess = VirtualFree(
        pvMemBegin,
        nPage * PAGE_SIZE,
        MEM_DECOMMIT);
    if (!bSuccess)
        ErrorExit(("DecommitPage failed.\n"));
}

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
    LazySpanFreeList * FindFreeList(size_t nPage);

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
    LazySpanFreeList vsfl[14];

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

    LazySpanFreeList * psfl;
    LazySpanFreeList * psflEnd;
    size_t nSpanPage;

    psfl = pscb->vsfl;
    psflEnd = pscb->vsfl + pscb->nSpanFreeList;
    nSpanPage = 1;

    for (;
         psfl < psflEnd;
         ++psfl, nSpanPage <<= 1)
    {
        if (bOwnMemory)
        {
            new (psfl) LazySpanFreeList(
                PAGE_BEGIN(pvMemBegin, nPage - (nSpanPage << 1)),
                nSpanPage
            );
        }
        else
        {
            new (psfl) LazySpanFreeList(
                nullptr,
                0
            );
            psfl->Insert(
                PAGE_BEGIN(pvMemBegin, nPage - (nSpanPage << 1)),
                nSpanPage
            );
        }
    }

    return pscb;
}

LazySpanFreeList *
SpanCtrlBlock::FindFreeList(size_t nPage)
{
    LazySpanFreeList * psfl;
    LazySpanFreeList * psflEnd;

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

    LazySpanFreeList * psfl;
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

    LazySpanFreeList * psfl;

    ASSERT(PageBegin(pvMemBegin) == pvMemBegin);
    ASSERT(MemBegin() <= pvMemBegin && pvMemBegin < MemEnd());

    psfl = vsfl + IntLog2(nPage);
    ASSERT(!psfl->HasLazySpan());
    if (psfl->Empty())
    {
        psfl->Insert(pvMemBegin, nPage);
        return;
    }

    Span * ps;

    ps = (Span *)pvMemBegin;
    ps->nPage = nPage;
    ps->psNext = nullptr;

    LazySpanFreeList::ForwardIterator fiLastLess;
    LazySpanFreeList::ForwardIterator fiFirstGreater;

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

SpanAllocator::ForwardIterator::ForwardIterator(const LazySpanFreeList * psflBegin, const LazySpanFreeList * psflEnd, const LazySpanFreeList * psflCurr)
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
