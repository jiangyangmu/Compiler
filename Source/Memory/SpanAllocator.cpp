#include "SpanAllocator.h"

#include "Win/WinAllocate.h"

namespace memory {

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
    {
        throw std::runtime_error("out of memory span!");
        // return nullptr;
    }

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

    while (true)
    {
        // psfl: Non-empty N-Page Span Free List
        // ps:   Free N-Page Span

        SpanFreeList::Position posNext = psfl->FindPos(ps);

        Span * psPrev = (posNext != psfl->BeginPos()) ? posNext.GetHostSpan() : nullptr;
        Span * psNext = posNext.GetPointedSpan();

        ASSERT(psPrev == nullptr || psPrev < ps);
        ASSERT(psNext == nullptr || ps < psNext);

        bool bIsEvenSpan = IsEvenSpan(MemBegin(), pvMemBegin, ps->nPage);

        bool bMergePrev = psPrev && !bIsEvenSpan && CanMergeSpan(psPrev, ps);
        bool bMergeNext = psNext && bIsEvenSpan && CanMergeSpan(ps, psNext);

        if (bMergePrev)
        {
            SpanFreeList::Position posPrev = psfl->FindPosBefore(posNext);
            
            (void *)posPrev.RemoveSpan();
            ps = MergeSpan(psPrev, ps);
        }
        else if (bMergeNext)
        {
            posNext.RemoveSpan();
            ps = MergeSpan(ps, psNext);
        }
        else
        {
            posNext.InsertSpan(ps);
            break;
        }

        ++psfl;
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
    static SpanAllocator * psa = new SpanAllocator();

    if (psa->pscb == nullptr)
    {
        new (psa) SpanAllocator(
            CreateSpanAllocator(DEFAULT_NUM_PAGE_PER_SPAN)
        );
        ASSERT(psa->pscb);
    }

    return psa;
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

#ifdef UNIT_TEST
#include "../UnitTest/UnitTest.h"

using namespace memory;

/*
Span Allocator Test Cases

1. Span allocator create/destroy (verify: SCB, span tree)

2. Span alloc/free (verify: SCB, span tree, pvMemBegin)
- 16 page, alloc 1 page * 15, free 0~14
- 16 page, alloc 1 page * 15, free 14~0

3. Span read/write

4. OOM
- 16 page, alloc 16 page * 1, OOM

5. Lazy initialization

*/

typedef std::vector<std::pair<size_t, const void *>> SpanVector;

#define ASSERT_SPAN(actual, expect) do \
{ \
    SpanVector v1(expect); \
    auto v2 = (actual); \
    auto i1 = v1.begin(); \
    auto i2 = v2.begin(); \
    bool matched = true; \
    while (matched && i1 != v1.end() && i2 != v2.end()) \
    { \
        matched = (i1->first == (*i2).nPage) && (i1->second == (*i2).cpvMemBegin); \
        ++i1, ++i2; \
    } \
    if (!matched || i1 != v1.end() || i2 != v2.end()) \
    { \
        std::cerr << "Wrong span list at " << __FILE__ << ":" << __LINE__ << std::endl;  \
        std::cerr << "Expect:\n"; \
        for (auto i : v1) std::cerr << "\t{ " << i.first << ", " << (void *)i.second << " }\n"; \
        std::cerr << std::endl; \
        std::cerr << "Actual:\n"; \
        for (auto i : v2) std::cerr << "\t{ " << i.nPage << ", " << i.cpvMemBegin << " }\n"; \
        std::cerr << std::endl; \
        TestRunner::Get().SetError(true); \
        std::exit(EXIT_FAILURE); \
    } \
} while (false);

TEST(SpanAllocator_Create)
{
    SpanAllocator sa = CreateSpanAllocator(16);

    void * vPage[16];
    {
        char * pcMemBegin = (char *)sa.AddrBegin();
        for (int i = 0; i < 16; ++i)
            vPage[i] = (void *)(pcMemBegin + i * PAGE_SIZE);
    }

    ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
        { 1, vPage[14] },
        { 2, vPage[12] },
        { 4, vPage[8] },
        { 8, vPage[0] },
        }));
}

TEST(SpanAllocator_AllocFree_Verbose)
{
    SpanAllocator sa = CreateSpanAllocator(16);

    void * vPage[16];
    {
        char * pcMemBegin = (char *)sa.AddrBegin();
        for (int i = 0; i < 16; ++i)
            vPage[i] = (void *)(pcMemBegin + i * PAGE_SIZE);
    }

    EXPECT_EQ(sa.Alloc(1), vPage[14]);
    EXPECT_EQ(sa.Alloc(1), vPage[12]);
    EXPECT_EQ(sa.Alloc(1), vPage[13]);
    EXPECT_EQ(sa.Alloc(1), vPage[8]);
    EXPECT_EQ(sa.Alloc(1), vPage[9]);
    EXPECT_EQ(sa.Alloc(1), vPage[10]);
    EXPECT_EQ(sa.Alloc(1), vPage[11]);
    EXPECT_EQ(sa.Alloc(1), vPage[0]);
    EXPECT_EQ(sa.Alloc(1), vPage[1]);
    EXPECT_EQ(sa.Alloc(1), vPage[2]);
    EXPECT_EQ(sa.Alloc(1), vPage[3]);
    EXPECT_EQ(sa.Alloc(1), vPage[4]);
    EXPECT_EQ(sa.Alloc(1), vPage[5]);
    EXPECT_EQ(sa.Alloc(1), vPage[6]);
    EXPECT_EQ(sa.Alloc(1), vPage[7]);
    {
        sa.Free(vPage[0], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            { 1, vPage[0] },
            }));
    }
    {
        sa.Free(vPage[1], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            { 2, vPage[0] },
            }));
    }
    {
        sa.Free(vPage[2], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            { 1, vPage[2] },
            { 2, vPage[0] },
            }));
    }
    {
        sa.Free(vPage[3], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            { 4, vPage[0] },
            }));
    }
    {
        sa.Free(vPage[4], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            { 1, vPage[4] },
            { 4, vPage[0] },
            }));
    }
    {
        sa.Free(vPage[5], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            { 2, vPage[4] },
            { 4, vPage[0] },
            }));
    }
    {
        sa.Free(vPage[6], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            { 1, vPage[6] },
            { 2, vPage[4] },
            { 4, vPage[0] },
            }));
    }
    {
        sa.Free(vPage[7], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            { 8, vPage[0] },
            }));
    }
    {
        sa.Free(vPage[8], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            { 1, vPage[8] },
            { 8, vPage[0] },
            }));
    }
    {
        sa.Free(vPage[9], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            { 2, vPage[8] },
            { 8, vPage[0] },
            }));
    }
    {
        sa.Free(vPage[10], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            { 1, vPage[10] },
            { 2, vPage[8] },
            { 8, vPage[0] },
            }));
    }
    {
        sa.Free(vPage[11], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            { 4, vPage[8] },
            { 8, vPage[0] },
            }));
    }
    {
        sa.Free(vPage[12], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            { 1, vPage[12] },
            { 4, vPage[8] },
            { 8, vPage[0] },
            }));
    }
    {
        sa.Free(vPage[13], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            { 2, vPage[12] },
            { 4, vPage[8] },
            { 8, vPage[0] },
            }));
    }
    {
        sa.Free(vPage[14], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            { 1, vPage[14] },
            { 2, vPage[12] },
            { 4, vPage[8] },
            { 8, vPage[0] },
            }));
    }

    EXPECT_EQ(sa.Alloc(1), vPage[14]);
    EXPECT_EQ(sa.Alloc(1), vPage[12]);
    EXPECT_EQ(sa.Alloc(1), vPage[13]);
    EXPECT_EQ(sa.Alloc(1), vPage[8]);
    EXPECT_EQ(sa.Alloc(1), vPage[9]);
    EXPECT_EQ(sa.Alloc(1), vPage[10]);
    EXPECT_EQ(sa.Alloc(1), vPage[11]);
    EXPECT_EQ(sa.Alloc(1), vPage[0]);
    EXPECT_EQ(sa.Alloc(1), vPage[1]);
    EXPECT_EQ(sa.Alloc(1), vPage[2]);
    EXPECT_EQ(sa.Alloc(1), vPage[3]);
    EXPECT_EQ(sa.Alloc(1), vPage[4]);
    EXPECT_EQ(sa.Alloc(1), vPage[5]);
    EXPECT_EQ(sa.Alloc(1), vPage[6]);
    EXPECT_EQ(sa.Alloc(1), vPage[7]);
    {
        sa.Free(vPage[14], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            { 1, vPage[14] },
            }));
    }
    {
        sa.Free(vPage[13], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            { 1, vPage[13] },
            { 1, vPage[14] },
            }));
    }
    {
        sa.Free(vPage[12], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            { 1, vPage[14] },
            { 2, vPage[12] },
            }));
    }
    {
        sa.Free(vPage[11], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            { 1, vPage[11] },
            { 1, vPage[14] },
            { 2, vPage[12] },
            }));
    }
    {
        sa.Free(vPage[10], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            { 1, vPage[14] },
            { 2, vPage[10] },
            { 2, vPage[12] },
            }));
    }
    {
        sa.Free(vPage[9], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            { 1, vPage[9] },
            { 1, vPage[14] },
            { 2, vPage[10] },
            { 2, vPage[12] },
            }));
    }
    {
        sa.Free(vPage[8], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            { 1, vPage[14] },
            { 2, vPage[12] },
            { 4, vPage[8] },
            }));
    }
    {
        sa.Free(vPage[7], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            { 1, vPage[7] },
            { 1, vPage[14] },
            { 2, vPage[12] },
            { 4, vPage[8] },
            }));
    }
    {
        sa.Free(vPage[6], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            { 1, vPage[14] },
            { 2, vPage[6] },
            { 2, vPage[12] },
            { 4, vPage[8] },
            }));
    }
    {
        sa.Free(vPage[5], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            { 1, vPage[5] },
            { 1, vPage[14] },
            { 2, vPage[6] },
            { 2, vPage[12] },
            { 4, vPage[8] },
            }));
    }
    {
        sa.Free(vPage[4], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            { 1, vPage[14] },
            { 2, vPage[12] },
            { 4, vPage[4] },
            { 4, vPage[8] },
            }));
    }
    {
        sa.Free(vPage[3], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            { 1, vPage[3] },
            { 1, vPage[14] },
            { 2, vPage[12] },
            { 4, vPage[4] },
            { 4, vPage[8] },
            }));
    }
    {
        sa.Free(vPage[2], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            { 1, vPage[14] },
            { 2, vPage[2] },
            { 2, vPage[12] },
            { 4, vPage[4] },
            { 4, vPage[8] },
            }));
    }
    {
        sa.Free(vPage[1], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            { 1, vPage[1] },
            { 1, vPage[14] },
            { 2, vPage[2] },
            { 2, vPage[12] },
            { 4, vPage[4] },
            { 4, vPage[8] },
            }));
    }
    {
        sa.Free(vPage[0], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            { 1, vPage[14] },
            { 2, vPage[12] },
            { 4, vPage[8] },
            { 8, vPage[0] },
            }));
    }
}

TEST(SpanAllocator_AllocFree_Complete)
{
    const size_t nPage = 8;
    const size_t nFreePage = nPage - 1;

    SpanAllocator sa = CreateSpanAllocator(nPage);

    void * vPage[nFreePage];
    void * vPageToFree[nFreePage];
    {
        char * pcMemBegin = (char *)sa.AddrBegin();
        for (int i = 0; i < nFreePage; ++i)
        {
            vPageToFree[i] = vPage[i] = (void *)(pcMemBegin + i * PAGE_SIZE);
        }
    }

    do
    {
        EXPECT_EQ(sa.Alloc(1), vPage[6]);
        EXPECT_EQ(sa.Alloc(1), vPage[4]);
        EXPECT_EQ(sa.Alloc(1), vPage[5]);
        EXPECT_EQ(sa.Alloc(1), vPage[0]);
        EXPECT_EQ(sa.Alloc(1), vPage[1]);
        EXPECT_EQ(sa.Alloc(1), vPage[2]);
        EXPECT_EQ(sa.Alloc(1), vPage[3]);

        for (int i = 0; i < nFreePage; ++i)
            sa.Free(vPageToFree[i], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            { 1, vPage[6] },
            { 2, vPage[4] },
            { 4, vPage[0] },
            }));
    } while (std::next_permutation(vPageToFree, vPageToFree + nFreePage));
}

TEST(SpanAllocator_ReadWrite)
{
    SpanAllocator sa = CreateSpanAllocator(16);

    char * pcPageBegin = (char *)sa.Alloc(1);
    char * pcPageEnd = pcPageBegin + PAGE_SIZE;

    for (char * pc = pcPageBegin; pc < pcPageEnd; ++pc)
        *pc = 1;
    for (char * pc = pcPageBegin; pc < pcPageEnd; ++pc)
        EXPECT_EQ((int)*pc, 1);
}

TEST(SpanAllocator_OOM)
{
    SpanAllocator sa = CreateSpanAllocator(16);

    EXPECT_EQ(sa.Alloc(16), (void *)nullptr);

    EXPECT_NE(sa.Alloc(8), (void *)nullptr);
    EXPECT_EQ(sa.Alloc(8), (void *)nullptr);

    EXPECT_NE(sa.Alloc(4), (void *)nullptr);
    EXPECT_EQ(sa.Alloc(4), (void *)nullptr);

    EXPECT_NE(sa.Alloc(2), (void *)nullptr);
    EXPECT_EQ(sa.Alloc(2), (void *)nullptr);

    EXPECT_NE(sa.Alloc(1), (void *)nullptr);
    EXPECT_EQ(sa.Alloc(1), (void *)nullptr);
}

#endif