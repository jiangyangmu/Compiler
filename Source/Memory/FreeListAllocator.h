#pragma once

#include <utility>

namespace LowLevel {

struct FreeListPage;

size_t GetMaxAllocNumPerPage(size_t nBlkSize);

class FreeListAllocator
{
public:
    explicit FreeListAllocator(size_t nBlkSize)
        : pflpFullList(nullptr), pflpHalfList(nullptr), pflpEmptyList(nullptr),
          nBlkSize(nBlkSize)
    {
    }
    FreeListAllocator(const FreeListAllocator & o) = delete;
    FreeListAllocator(FreeListAllocator && o)
        : pflpFullList(o.pflpFullList), pflpHalfList(o.pflpHalfList), pflpEmptyList(o.pflpEmptyList),
          nBlkSize(o.nBlkSize)
    {
        o.pflpFullList = nullptr;
        o.pflpHalfList = nullptr;
        o.pflpEmptyList = nullptr;
    }
    FreeListAllocator & operator = (const FreeListAllocator &) = delete;
    FreeListAllocator & operator = (FreeListAllocator && o)
    {
        this->~FreeListAllocator();
        new (this) FreeListAllocator(std::move(o));
        return *this;
    }
    ~FreeListAllocator();

    void * Alloc();
    void Free(void * pvMemBegin);

    bool ReleaseAllUnusedPages();

private:
    FreeListPage * pflpFullList;
    FreeListPage * pflpHalfList;
    FreeListPage * pflpEmptyList;
    const size_t nBlkSize;
};

class GenericFreeListAllocator
{
public:
    GenericFreeListAllocator();
    GenericFreeListAllocator(const GenericFreeListAllocator & o) = delete;
    GenericFreeListAllocator(GenericFreeListAllocator && o);
    GenericFreeListAllocator & operator = (const GenericFreeListAllocator &) = delete;
    GenericFreeListAllocator & operator = (GenericFreeListAllocator && o);
    ~GenericFreeListAllocator() = default;

    void * Alloc(size_t nBytes);
    void Free(void * pvMemBegin);

private:
    // 8, 16, 32, 64, 128
    FreeListAllocator vpfla[5];
};

}
