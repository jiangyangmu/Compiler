#include "Span.h"

#include <windows.h>
#include <tchar.h>
#include <cstdio>
#include <cassert>

#include "../Util/Common.h"

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
        MEM_COMMIT,
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

static
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

static inline
void
TestAndCommitPage(void * pvMemBegin, size_t nPage)
{
    printf(("TestAndCommitPage: %p %zd pages.\n"), pvMemBegin, nPage);
    ErrorExit(("TestAndCommitPage not implemented.\n"));
    //__try
    //{
    //    // Test read/write.
    //    char byte;
    //    byte = *(char *)pvMemBegin;
    //    *(char *)pvMemBegin = byte;
    //}
    //__except (PageFaultExceptionFilter(GetExceptionCode()))
    //{
    //}
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
// Span: split
// SpanFreeList: init, push, pop, empty
// ===========================================================================

Span *
SplitSpan(Span * ps)
{
    Span * psSecond;
    size_t nPage;

    nPage           = ps->nPage >> 1;
    
    psSecond        = (Span *)((char *)ps + nPage * PAGE_SIZE);
    psSecond->nPage = nPage;

    ps->psNext      = psSecond;
    ps->nPage       = nPage;

    return psSecond;
}

Span *
MergeSpan(Span * psLeft, Span * psRight)
{
    ASSERT(psLeft && psRight && psLeft->nPage == psRight->nPage);

    if ((char *)psLeft + psLeft->nPage * PAGE_SIZE == (char *)psRight)
    {
        psLeft->nPage <<= 1;
        return psLeft;
    }
    else
    {
        return nullptr;
    }
}

void
SpanFreeList::Insert(Span * ps)
{
    Span ** ppsInsert;

    ppsInsert = &psHead;
    while (*ppsInsert != nullptr && *ppsInsert < ps)
    {
        ppsInsert = &(*ppsInsert)->psNext;
    }
    ps->psNext  = *ppsInsert;
    *ppsInsert  = ps;
}

void
SpanFreeList::Insert(void * pvMemBegin, size_t nPage)
{
    Span * ps;
    ps          = (Span *)pvMemBegin;
    ps->nPage   = nPage;
    Insert(ps);
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
// SpanCtrlBlock: init, alloc, free
// ===========================================================================

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

    // Return first SFL with >=nPage Span, or nullptr.
    SpanFreeList * FindFirst(size_t nPage);

    void * Alloc(size_t nPage);
    void   Free(void * pvMemBegin, size_t nPage);

    void * MemBegin()
    {
        return (void *)((char *)this - (nTotalPage - 1) * PAGE_SIZE);
    }
    void * MemEnd()
    {
        return (void *)((char *)this + PAGE_SIZE);
    }
    
//private:
    // Memory
    size_t nTotalPage;
    bool bOwnMemory;
    // TODO: Lazy commit

    // 2^12, 2^13, ..., 2^25 byte
    // 1,    2,    ..., 8192 page
    // 4KB,  8KB,  ..., 32MB
    size_t nSpanFreeList;
    SpanFreeList vsfl[14];
};

SpanCtrlBlock *
CreateSpanCtrlBlock(void * pvMemBegin, size_t nPage, bool bOwnMemory)
{
    SpanCtrlBlock * pscb;

    // Init SCB.

    pscb                = (SpanCtrlBlock *)PAGE_BEGIN(pvMemBegin, nPage - 1);
    pscb->nTotalPage    = nPage;
    pscb->bOwnMemory    = bOwnMemory;
    pscb->nSpanFreeList = IntLog2(nPage);

    // Init free lists.

    SpanFreeList * psfl;
    SpanFreeList * psflEnd;
    size_t nSpanPage;

    psfl        = pscb->vsfl;
    psflEnd     = pscb->vsfl + pscb->nSpanFreeList;
    nSpanPage   = 1;
    
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
SpanCtrlBlock::FindFirst(size_t nPage)
{
    SpanFreeList * psfl;
    SpanFreeList * psflEnd;
    
    psfl    = vsfl + IntLog2(nPage);
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

    psfl = FindFirst(nPage);
    if (psfl == nullptr)
        return nullptr;

    ps = psfl->Pop();
    // psfl: N-Page Span Free List
    // ps: Free N-Page Span

    while (nPage < ps->nPage)
    {
        psSecond = SplitSpan(ps);
       
        --psfl;
        psfl->Insert(psSecond);

        // psfl: N-Page Span Free List
        // ps: Free N-Page Span
    }

    return (void *)ps;
}

static inline
bool
IsEvenPage(void * pvFirstPage, void * pvCurrPage, size_t nPage)
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

    Span ** ppsLastLess;
    Span ** ppsFirstGreater;

    while (true)
    {
        ppsLastLess = nullptr;
        ppsFirstGreater = &psfl->psHead;

        while (*ppsFirstGreater != nullptr && *ppsFirstGreater < pvMemBegin)
        {
            ppsLastLess = ppsFirstGreater;
            ppsFirstGreater = &(*ppsFirstGreater)->psNext;
        }

        if (ppsLastLess &&
            *ppsLastLess &&
            !IsEvenPage(MemBegin(), pvMemBegin, ps->nPage) &&
            (ps = MergeSpan(*ppsLastLess, ps)) != nullptr)
        {
            *ppsLastLess = (*ppsLastLess)->psNext;
        }
        else if (*ppsFirstGreater &&
                 IsEvenPage(MemBegin(), pvMemBegin, ps->nPage) &&
                 (ps = MergeSpan(ps, *ppsFirstGreater)) != nullptr)
        {
            *ppsFirstGreater = (*ppsFirstGreater)->psNext;
        }
        else
            break;

        ++psfl;
    }

    ps->psNext          = *ppsFirstGreater;
    *ppsFirstGreater    = ps;
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
SpanAllocator::~SpanAllocator()
{
    if (pscb)
        pscb->~SpanCtrlBlock();
}

void * SpanAllocator::Alloc(size_t nPage)
{
    return pscb->Alloc(nPage);
}
void SpanAllocator::Free(void * pvMemBegin, size_t nPage)
{
    return pscb->Free(pvMemBegin, nPage);
}


const void * SpanAllocator::MemBegin() const
{
    return pscb->MemBegin();
}

// ===========================================================================
// SpanView
// ===========================================================================

SpanView::ForwardIterator::ForwardIterator()
    : cpsflBegin(nullptr), cpsflEnd(nullptr), cpsflCurr(nullptr), cpsCurr(nullptr)
{
}

SpanView::ForwardIterator::ForwardIterator(const SpanFreeList * psflBegin, const SpanFreeList * psflEnd, const SpanFreeList * psflCurr)
    : cpsflBegin(psflBegin), cpsflEnd(psflEnd), cpsflCurr(psflCurr)
{
    cpsCurr = cpsflCurr->psHead;
    while (cpsCurr == nullptr && cpsflCurr < cpsflEnd)
    {
        ++cpsflCurr;
        cpsCurr = cpsflCurr->psHead;
    }
}

SpanView::ForwardIterator &
SpanView::ForwardIterator::operator ++ ()
{
    if (cpsflCurr < cpsflEnd)
    {
        // Curr in free list or first non-null in Curr free lists.
        if (cpsCurr)
        {
            cpsCurr = cpsCurr->psNext;
        }
        while (cpsCurr == nullptr && cpsflCurr < cpsflEnd)
        {
            ++cpsflCurr;
            cpsCurr = cpsflCurr->psHead;
        }
    }
    return *this;
}

SpanView::ForwardIterator
SpanView::ForwardIterator::operator ++ (int)
{
    ForwardIterator tmp(*this);
    operator++();
    return tmp;
}

bool
SpanView::ForwardIterator::operator == (const ForwardIterator & o)
{
    ASSERT(cpsflBegin == o.cpsflBegin && cpsflEnd == o.cpsflEnd);
    return cpsCurr == o.cpsCurr;
}

bool
SpanView::ForwardIterator::operator != (const ForwardIterator & o)
{
    return !operator==(o);
}

const Span *
SpanView::ForwardIterator::operator * ()
{
    ASSERT(cpsCurr);
    return cpsCurr;
}

SpanView::SpanView(const SpanFreeList & sfl)
    : beginIterator(&sfl, &sfl + 1, &sfl),
      endIterator(&sfl, &sfl + 1, &sfl + 1)
{
}

SpanView::SpanView(const SpanAllocator & sa)
    : beginIterator(sa.pscb->vsfl,
                    sa.pscb->vsfl + sa.pscb->nSpanFreeList,
                    sa.pscb->vsfl),
      endIterator(sa.pscb->vsfl,
                  sa.pscb->vsfl + sa.pscb->nSpanFreeList,
                  sa.pscb->vsfl + sa.pscb->nSpanFreeList)
{
}

}
