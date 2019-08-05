#include "FreeListAllocator.h"

#include "../Base/Bit.h"

#include "Span.h"

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
        size_t i = (sizeof(vpfla) / sizeof(vpfla[0]));
        
        while (i != 0)
        {
            --i;
            if (i != ifla && vpfla[i].ReleaseAllUnusedPages())
                break;
        }

        pvBlkBegin = vpfla[ifla].Alloc();
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
