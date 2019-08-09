#include "../Source/UnitTest/UnitTest.h"

#include "../Source/Memory/SpanAllocator.h"
#include "../Source/Memory/FreeListAllocator.h"

using namespace LowLevel;

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
        s1.nPage  = 1;
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
        {1, vPage[14]},
        {2, vPage[12]},
        {4, vPage[8]},
        {8, vPage[0]},
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
            {1, vPage[0]},
        }));
    }
    {
        sa.Free(vPage[1], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {2, vPage[0]},
        }));
    }
    {
        sa.Free(vPage[2], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {1, vPage[2]},
            {2, vPage[0]},
        }));
    }
    {
        sa.Free(vPage[3], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {4, vPage[0]},
        }));
    }
    {
        sa.Free(vPage[4], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {1, vPage[4]},
            {4, vPage[0]},
        }));
    }
    {
        sa.Free(vPage[5], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {2, vPage[4]},
            {4, vPage[0]},
        }));
    }
    {
        sa.Free(vPage[6], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {1, vPage[6]},
            {2, vPage[4]},
            {4, vPage[0]},
        }));
    }
    {
        sa.Free(vPage[7], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {8, vPage[0]},
        }));
    }
    {
        sa.Free(vPage[8], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {1, vPage[8]},
            {8, vPage[0]},
        }));
    }
    {
        sa.Free(vPage[9], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {2, vPage[8]},
            {8, vPage[0]},
        }));
    }
    {
        sa.Free(vPage[10], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {1, vPage[10]},
            {2, vPage[8]},
            {8, vPage[0]},
        }));
    }
    {
        sa.Free(vPage[11], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {4, vPage[8]},
            {8, vPage[0]},
        }));
    }
    {
        sa.Free(vPage[12], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {1, vPage[12]},
            {4, vPage[8]},
            {8, vPage[0]},
        }));
    }
    {
        sa.Free(vPage[13], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {2, vPage[12]},
            {4, vPage[8]},
            {8, vPage[0]},
        }));
    }
    {
        sa.Free(vPage[14], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {1, vPage[14]},
            {2, vPage[12]},
            {4, vPage[8]},
            {8, vPage[0]},
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
            {1, vPage[14]},
        }));
    }
    {
        sa.Free(vPage[13], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {1, vPage[13]},
            {1, vPage[14]},
        }));
    }
    {
        sa.Free(vPage[12], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {1, vPage[14]},
            {2, vPage[12]},
        }));
    }
    {
        sa.Free(vPage[11], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {1, vPage[11]},
            {1, vPage[14]},
            {2, vPage[12]},
        }));
    }
    {
        sa.Free(vPage[10], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {1, vPage[14]},
            {2, vPage[10]},
            {2, vPage[12]},
        }));
    }
    {
        sa.Free(vPage[9], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {1, vPage[9]},
            {1, vPage[14]},
            {2, vPage[10]},
            {2, vPage[12]},
        }));
    }
    {
        sa.Free(vPage[8], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {1, vPage[14]},
            {2, vPage[12]},
            {4, vPage[8]},
        }));
    }
    {
        sa.Free(vPage[7], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {1, vPage[7]},
            {1, vPage[14]},
            {2, vPage[12]},
            {4, vPage[8]},
        }));
    }
    {
        sa.Free(vPage[6], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {1, vPage[14]},
            {2, vPage[6]},
            {2, vPage[12]},
            {4, vPage[8]},
        }));
    }
    {
        sa.Free(vPage[5], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {1, vPage[5]},
            {1, vPage[14]},
            {2, vPage[6]},
            {2, vPage[12]},
            {4, vPage[8]},
        }));
    }
    {
        sa.Free(vPage[4], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {1, vPage[14]},
            {2, vPage[12]},
            {4, vPage[4]},
            {4, vPage[8]},
        }));
    }
    {
        sa.Free(vPage[3], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {1, vPage[3]},
            {1, vPage[14]},
            {2, vPage[12]},
            {4, vPage[4]},
            {4, vPage[8]},
        }));
    }
    {
        sa.Free(vPage[2], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {1, vPage[14]},
            {2, vPage[2]},
            {2, vPage[12]},
            {4, vPage[4]},
            {4, vPage[8]},
        }));
    }
    {
        sa.Free(vPage[1], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {1, vPage[1]},
            {1, vPage[14]},
            {2, vPage[2]},
            {2, vPage[12]},
            {4, vPage[4]},
            {4, vPage[8]},
        }));
    }
    {
        sa.Free(vPage[0], 1);

        ASSERT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {1, vPage[14]},
            {2, vPage[12]},
            {4, vPage[8]},
            {8, vPage[0]},
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
            {1, vPage[6]},
            {2, vPage[4]},
            {4, vPage[0]},
        }));
    }
    while (std::next_permutation(vPageToFree, vPageToFree + nFreePage));
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

/*
    Free List Allocator Test Cases

    1. FreeListAllocator create/destroy (verify: three page list)

    2. FreeListAllocator alloc/free (verify: three page list, pvMemBegin)
        - alloc A*, free A*
        - alloc A*, zero A*, free A*
        - alloc A*, rand A*, free A*

    3. FreeListAllocator OOM

    4. GenericFreeListAllocator create/destroy

    5. GenericFreeListAllocator alloc/free

    6. GenericFreeListAllocator OOM

*/

// Use default span allocator
class AllocationVerifier
{
public:
    AllocationVerifier(size_t szBlk)
    {
        nPage = DEFAULT_NUM_PAGE_PER_SPAN - 1;
        this->szBlk = szBlk;
        vmDupAlloc.resize(nPage);
        vmDupFree.resize(nPage);
        vmUnalignedAlloc.resize(nPage);
        vmUnalignedFree.resize(nPage);
    }

    void Alloc(void * pvBlkBegin)
    {
        if(GetDefaultSpanAllocator()->IsOwnerOf(pvBlkBegin))
        {
            if (!Aligned(pvBlkBegin))
                vmUnalignedAlloc.at(PageIndex(pvBlkBegin)).insert(pvBlkBegin);
            ++vmDupAlloc.at(PageIndex(pvBlkBegin))[pvBlkBegin];
        }
        else
            ++mOutOfRangeAlloc[pvBlkBegin];
    }

    void Free(void * pvBlkBegin)
    {
        if (GetDefaultSpanAllocator()->IsOwnerOf(pvBlkBegin))
        {
            if (!Aligned(pvBlkBegin))
                vmUnalignedFree.at(PageIndex(pvBlkBegin)).insert(pvBlkBegin);
            ++vmDupFree.at(PageIndex(pvBlkBegin))[pvBlkBegin];
        }
        else
            ++mOutOfRangeFree[pvBlkBegin];
    }

    bool Report()
    {
        // Duplicate alloc/free: page index, address, count
        // Unaligned alloc/free: page index, address
        // Out-of-range alloc/free: address, count

        bool bError = false;

        for (size_t i = 0; i < nPage; ++i)
        {
            for (auto & kv : vmDupAlloc[i])
            {
                if (kv.second > 1)
                    bError = true, std::cout << "Duplicate Alloc: Page=" << i << " Addr=" << kv.first << " Count=" << kv.second << std::endl;
            }
        }
        for (size_t i = 0; i < nPage; ++i)
        {
            for (auto & kv : vmDupFree[i])
            {
                if (kv.second > 1)
                    bError = true, std::cout << "Duplicate Free: Page=" << i << " Addr=" << kv.first << " Count=" << kv.second << std::endl;
            }
        }
        for (size_t i = 0; i < nPage; ++i)
        {
            for (auto & v : vmUnalignedAlloc[i])
            {
                bError = true, std::cout << "Unaligned Alloc: Page=" << i << " Addr=" << v << std::endl;
            }
        }
        for (size_t i = 0; i < nPage; ++i)
        {
            for (auto & v : vmUnalignedFree[i])
            {
                bError = true, std::cout << "Unaligned Free: Page=" << i << " Addr=" << v << std::endl;
            }
        }
        for (auto & kv : mOutOfRangeAlloc)
        {
            bError = true, std::cout << "Out-of-range Alloc: Addr=" << kv.first << " Count=" << kv.second << std::endl;
        }
        for (auto & kv : mOutOfRangeFree)
        {
            bError = true, std::cout << "Out-of-range Free: Addr=" << kv.first << " Count=" << kv.second << std::endl;
        }

        return bError;
    }

private:
    bool Aligned(void * pvBlkBegin)
    {
        return ((char *)pvBlkBegin - (char *)PageBegin(pvBlkBegin)) % szBlk == 0;
    }
    size_t PageIndex(void * pv)
    {
        return ((char *)PageBegin(pv) - (char *)GetDefaultSpanAllocator()->AddrBegin()) >> PAGE_SIZE_BITS;
    }

    size_t nPage;
    size_t szBlk;

    std::vector<std::map<void *, size_t>> vmDupAlloc;
    std::vector<std::map<void *, size_t>> vmDupFree;

    std::vector<std::set<void *>> vmUnalignedAlloc;
    std::vector<std::set<void *>> vmUnalignedFree;

    std::map<void *, size_t> mOutOfRangeAlloc;
    std::map<void *, size_t> mOutOfRangeFree;
};

//#define 

TEST(FreeListAllocator_Create)
{
    FreeListAllocator fa(8);
}

TEST(FreeListAllocator_Create_Complete)
{
    for (size_t szBlk = 8; szBlk <= 128; szBlk <<= 1)
    {
        FreeListAllocator fa(szBlk);
    }
}

TEST(FreeListAllocator_AllocFree_Verbose)
{
    FreeListAllocator fa(8);

    void * addr[(DEFAULT_NUM_PAGE_PER_SPAN - 1) * PAGE_SIZE / 8];
    
    size_t nMaxAllocNum = (DEFAULT_NUM_PAGE_PER_SPAN - 1) * GetMaxAllocNumPerPage(8);

    {
        AllocationVerifier av(8);

        for (size_t i = 0; i < nMaxAllocNum; ++i)
        {
            addr[i] = fa.Alloc();
            av.Alloc(addr[i]);
        }

        for (size_t i = 0; i < nMaxAllocNum; ++i)
        {
            fa.Free(addr[i]);
            av.Free(addr[i]);
        }

        ASSERT_EQ(av.Report(), false);
    }

    {
        AllocationVerifier av(8);
        for (size_t i = 0; i < nMaxAllocNum; ++i)
        {
            addr[i] = fa.Alloc();
            av.Alloc(addr[i]);
        }

        for (size_t i = 0; i < nMaxAllocNum; ++i)
        {
            fa.Free(addr[nMaxAllocNum - i - 1]);
            av.Free(addr[i]);
        }

        ASSERT_EQ(av.Report(), false);
    }
}

TEST(FreeListAllocator_AllocFree_Complete)
{
    void * addr[(DEFAULT_NUM_PAGE_PER_SPAN - 1) * PAGE_SIZE / 8];

    for (size_t szBlk = 8; szBlk <= 128; szBlk <<= 1)
    {
        FreeListAllocator fa(szBlk);

        size_t nMaxAllocNum = GetMaxAllocNumPerPage(szBlk);
        nMaxAllocNum *= (DEFAULT_NUM_PAGE_PER_SPAN - 1);

        {
            AllocationVerifier av(szBlk);

            for (size_t i = 0; i < nMaxAllocNum; ++i)
            {
                addr[i] = fa.Alloc();
                av.Alloc(addr[i]);
            }

            for (size_t i = 0; i < nMaxAllocNum; ++i)
            {
                fa.Free(addr[i]);
                av.Free(addr[i]);
            }

            ASSERT_EQ(av.Report(), false);
        }

        {
            AllocationVerifier av(szBlk);
            for (size_t i = 0; i < nMaxAllocNum; ++i)
            {
                addr[i] = fa.Alloc();
                av.Alloc(addr[i]);
            }

            for (size_t i = 0; i < nMaxAllocNum; ++i)
            {
                fa.Free(addr[nMaxAllocNum - i - 1]);
                av.Free(addr[i]);
            }

            ASSERT_EQ(av.Report(), false);
        }
    }
}

TEST(FreeListAllocator_OOM)
{
    FreeListAllocator fa(128);

    void * pvPrev, * pvCurr;
    size_t count;

    AllocationVerifier av(128);

    pvPrev = nullptr;
    count  = 0;
    while ((pvCurr = fa.Alloc()) != nullptr)
    {
        av.Alloc(pvCurr);

        *(void **)pvCurr = pvPrev;
        pvPrev           = pvCurr;
        
        ++count;
    }

    EXPECT_EQ(count, (DEFAULT_NUM_PAGE_PER_SPAN - 1) * GetMaxAllocNumPerPage(128));
    std::cout << "OOM after alloc " << count << " block (128 byte)" << std::endl;

    while (pvPrev != nullptr)
    {
        pvCurr = pvPrev;
        pvPrev = *(void **)pvPrev;

        fa.Free(pvCurr);
        av.Free(pvCurr);
        
        --count;
    }

    EXPECT_EQ(count, 0);
    ASSERT_EQ(av.Report(), false);
}

TEST(FreeListAllocator_OOM_Complete)
{
    for (size_t szBlk = 8; szBlk <= 128; szBlk <<= 1)
    {
        FreeListAllocator fa(szBlk);

        void * pvPrev, *pvCurr;
        size_t count;

        AllocationVerifier av(szBlk);

        pvPrev = nullptr;
        count = 0;
        while ((pvCurr = fa.Alloc()) != nullptr)
        {
            av.Alloc(pvCurr);

            *(void **)pvCurr = pvPrev;
            pvPrev           = pvCurr;

            ++count;
        }

        EXPECT_EQ(count, (DEFAULT_NUM_PAGE_PER_SPAN - 1) * GetMaxAllocNumPerPage(szBlk));
        std::cout << "OOM after alloc " << count << " block (" << szBlk << " byte)" << std::endl;

        while (pvPrev != nullptr)
        {
            pvCurr = pvPrev;
            pvPrev = *(void **)pvPrev;

            fa.Free(pvCurr);
            av.Free(pvCurr);

            --count;
        }

        EXPECT_EQ(count, 0);
        ASSERT_EQ(av.Report(), false);
    }
}

TEST(GenericFreeListAllocator_Create)
{
    GenericFreeListAllocator gfa;
}

TEST(GenericFreeListAllocator_AllocFree_Complete)
{
    GenericFreeListAllocator gfa;

    void * pvPrev, *pvCurr;
    size_t count;

    for (size_t szBlk = 8; szBlk <= 128; szBlk <<= 1)
    {
        AllocationVerifier av(szBlk);

        pvPrev = nullptr;
        count = 0;
        while ((pvCurr = gfa.Alloc(szBlk)) != nullptr)
        {
            av.Alloc(pvCurr);

            *(void **)pvCurr = pvPrev;
            pvPrev = pvCurr;

            ++count;
        }

        EXPECT_EQ(count, (DEFAULT_NUM_PAGE_PER_SPAN - 1) * GetMaxAllocNumPerPage(szBlk));
        std::cout << "OOM after alloc " << count << " block (" << szBlk << " byte)" << std::endl;

        while (pvPrev != nullptr)
        {
            pvCurr = pvPrev;
            pvPrev = *(void **)pvPrev;

            gfa.Free(pvCurr);
            av.Free(pvCurr);

            --count;
        }

        ASSERT_EQ(av.Report(), false);
    }
}
