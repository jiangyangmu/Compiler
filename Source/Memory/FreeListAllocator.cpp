#include "FreeListAllocator.h"

#include "../Base/Bits.h"
#include "../Base/common.h"

#include "SpanAllocator.h"

namespace LowLevel {

struct FreeListPage
{
    FreeListPage * pflpNext;

    void * pvNextFree;
    void * pvUntouched;
    size_t nBlkSize;

    size_t nFree; // free + untouched
    size_t nTotal;
};

size_t GetMaxAllocNumPerPage(size_t nBlkSize)
{
    return (PAGE_SIZE - sizeof(FreeListPage)) / nBlkSize;
}

// nBlkSize is pow of 2.
FreeListPage *
InitFreeListPage(void * pvPage, size_t nBlkSize)
{
    FreeListPage * pflp;

    ASSERT(pvPage && nBlkSize <= 128);

    pflp                = (FreeListPage *)pvPage;
    pflp->pflpNext      = nullptr;
    
    pflp->pvNextFree    = nullptr;
    pflp->pvUntouched   = (void *)(
        (char *)pvPage +
        ((sizeof(FreeListPage) + (nBlkSize - 1)) / nBlkSize * nBlkSize)
        );

    pflp->nBlkSize      = nBlkSize;
    pflp->nFree         = (((char *)pvPage + PAGE_SIZE) - (char *)pflp->pvUntouched) / nBlkSize;
    pflp->nTotal        = pflp->nFree;

    return pflp;
}

void *
FreeListPage_Alloc(FreeListPage * pflp)
{
    void * pvBlk;

    if (pflp->nFree == 0)
        return nullptr;
    
    if (pflp->pvNextFree)
    {
        pvBlk               = pflp->pvNextFree;
        pflp->pvNextFree    = *(void **)pflp->pvNextFree;
    }
    else
    {
        pvBlk               = pflp->pvUntouched;
        pflp->pvUntouched   = (void *)((char *)pflp->pvUntouched + pflp->nBlkSize);
    }
    --pflp->nFree;

    return pvBlk;
}

void
FreeListPage_Free(FreeListPage * pflp, void * pvBlkBegin)
{
    ASSERT(PageBegin(pvBlkBegin) == (void *)pflp);
    
    *(void **)pvBlkBegin    = pflp->pvNextFree;
    pflp->pvNextFree        = pvBlkBegin;

    ++pflp->nFree;
}

bool
FreeListPage_IsEmpty(const FreeListPage * pflp)
{
    return pflp->nFree == pflp->nTotal;
}

bool
FreeListPage_IsFull(const FreeListPage * pflp)
{
    return pflp->nFree == 0;
}

void
FreeListPageList_Push(FreeListPage ** ppflpHead, FreeListPage * pflp)
{
    pflp->pflpNext  = *ppflpHead;
    *ppflpHead      = pflp;
}

FreeListPage *
FreeListPageList_Pop(FreeListPage ** ppflpHead)
{
    FreeListPage * pflp;

    ASSERT(ppflpHead);
    
    if (*ppflpHead)
    {
        pflp            = *ppflpHead;
        *ppflpHead      = pflp->pflpNext;
        pflp->pflpNext  = nullptr;
    }
    else
    {
        pflp            = nullptr;
    }

    return pflp;
}

void
FreeListPageList_Remove(FreeListPage ** ppflpHead, FreeListPage * pflp)
{
    while (*ppflpHead != pflp)
        ppflpHead = &(*ppflpHead)->pflpNext;

    *ppflpHead = pflp->pflpNext;
}

bool
FreeListPageList_Contains(const FreeListPage * pflpList, FreeListPage * pflp)
{
    ASSERT(pflp);

    while (pflpList && pflpList != pflp)
        pflpList = pflpList->pflpNext;

    return pflpList == pflp;
}


FreeListAllocator::~FreeListAllocator()
{
    ASSERT(pflpFullList == nullptr && pflpHalfList == nullptr);
    ReleaseAllUnusedPages();
}

void *
FreeListAllocator::Alloc()
{
    void * pvBlkBegin;

    // Empty to half, half to full.

    if (pflpHalfList != nullptr)
    {
        pvBlkBegin = FreeListPage_Alloc(pflpHalfList);

        if (FreeListPage_IsFull(pflpHalfList))
        {
            FreeListPageList_Push(
                &pflpFullList,
                FreeListPageList_Pop(&pflpHalfList));
        }
    }
    else
    {
        FreeListPage * pflp;

        if (pflpEmptyList == nullptr)
        {
            void * pvPage;
            
            pvPage = GetDefaultSpanAllocator()->Alloc(1);
            
            if (pvPage == nullptr)
                return nullptr;

            pflp = InitFreeListPage(pvPage, nBlkSize);
        }
        else
        {
            pflp = FreeListPageList_Pop(&pflpEmptyList);
        }

        pvBlkBegin = FreeListPage_Alloc(pflp);

        FreeListPageList_Push(
            &pflpHalfList,
            pflp
        );
    }

    return pvBlkBegin;
}

void
FreeListAllocator::Free(void * pvMemBegin)
{
    FreeListPage * pflp;

    pflp = (FreeListPage *)PageBegin(pvMemBegin);

    // Full to half, half to empty.

    if (FreeListPage_IsFull(pflp))
    {
        ASSERT(FreeListPageList_Contains(pflpFullList, pflp));
        FreeListPageList_Remove(&pflpFullList, pflp);
        FreeListPageList_Push(&pflpHalfList, pflp);
    }
    
    FreeListPage_Free(pflp, pvMemBegin);
    
    if (FreeListPage_IsEmpty(pflp))
    {
        ASSERT(FreeListPageList_Contains(pflpHalfList, pflp));
        FreeListPageList_Remove(&pflpHalfList, pflp);
        FreeListPageList_Push(&pflpEmptyList, pflp);
    }
}


bool
FreeListAllocator::ReleaseAllUnusedPages()
{
    FreeListPage * pflp;
    FreeListPage * pflpNext;
    bool bReleased;

    bReleased = (pflpEmptyList != nullptr);
    
    pflp = pflpEmptyList;
    while (pflp)
    {
        pflpNext    = pflp->pflpNext;
        GetDefaultSpanAllocator()->Free(pflp, 1);
        pflp        = pflpNext;
    }

    pflpEmptyList = nullptr;

    return bReleased;
}

GenericFreeListAllocator::GenericFreeListAllocator()
    : vpfla { FreeListAllocator(8),
              FreeListAllocator(16),
              FreeListAllocator(32),
              FreeListAllocator(64),
              FreeListAllocator(128) }
{
}

GenericFreeListAllocator::GenericFreeListAllocator(GenericFreeListAllocator && o)
    : vpfla { std::move(o.vpfla[0]),
              std::move(o.vpfla[1]),
              std::move(o.vpfla[2]),
              std::move(o.vpfla[3]),
              std::move(o.vpfla[4]) }
{
}

GenericFreeListAllocator &
GenericFreeListAllocator::operator = (GenericFreeListAllocator && o)
{
    this->~GenericFreeListAllocator();
    new (this) GenericFreeListAllocator(std::move(o));
    return *this;
}

void *
GenericFreeListAllocator::Alloc(size_t nBytes)
{
    ASSERT(nBytes <= 128);

    void * pvBlkBegin;
    size_t ifla;

    ifla        = IntLog2(nBytes / 8);
    pvBlkBegin  = vpfla[ifla].Alloc();

    if (pvBlkBegin == nullptr)
    {
        for (size_t i = ifla + 1; i < ELEMENT_COUNT(vpfla); ++i)
        {
            if (vpfla[i].ReleaseAllUnusedPages() &&
                (pvBlkBegin = vpfla[ifla].Alloc()) != nullptr)
                break;
        }
        if (pvBlkBegin == nullptr)
        {
            size_t i = ifla;
            while (i != 0)
            {
                --i;
                if (vpfla[i].ReleaseAllUnusedPages() &&
                    (pvBlkBegin = vpfla[ifla].Alloc()) != nullptr)
                    break;
            }
        }
    }

    return pvBlkBegin;
}

void
GenericFreeListAllocator::Free(void * pvMemBegin)
{
    size_t nBlkSize;

    nBlkSize = ((FreeListPage *)PageBegin(pvMemBegin))->nBlkSize;

    ASSERT(CountBits(nBlkSize) == 1);

    vpfla[IntLog2(nBlkSize) - 3].Free(pvMemBegin);
}

}

#ifdef UNIT_TEST
#include "../UnitTest/UnitTest.h"

using namespace LowLevel;

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
        if (GetDefaultSpanAllocator()->IsOwnerOf(pvBlkBegin))
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

    void * pvPrev, *pvCurr;
    size_t count;

    AllocationVerifier av(128);

    pvPrev = nullptr;
    count = 0;
    while ((pvCurr = fa.Alloc()) != nullptr)
    {
        av.Alloc(pvCurr);

        *(void **)pvCurr = pvPrev;
        pvPrev = pvCurr;

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
            pvPrev = pvCurr;

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

#endif