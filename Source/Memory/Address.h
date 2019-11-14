#pragma once

#include "../Base/Common.h"
#include "../Base/Integer.h"

namespace memory {

// ===========================================================================
// page constants: size, bits
// page arith: PageBegin, PagePtr
// ===========================================================================

constexpr size_t PAGE_SIZE = 4096;              // 4KB
constexpr int    PAGE_SIZE_BITS = 12;
constexpr size_t PAGE_OFFSET_MASK = 0x0000000000000FFF;

inline void * PageBegin(void * pvMemBegin)
{
    return (void *)((uptr)pvMemBegin & ~PAGE_OFFSET_MASK);
}

struct PagePtr
{
    void * pvPageBegin;

    PagePtr(void * pvMemBegin)
        : pvPageBegin(pvMemBegin)
    {
        ASSERT(PageBegin(pvMemBegin) == pvMemBegin);
    }
    inline PagePtr & operator += (uptr n)
    {
        pvPageBegin = (void *)((uptr)pvPageBegin + (n << PAGE_SIZE_BITS));
        return *this;
    }
    inline PagePtr operator + (uptr n)
    {
        PagePtr pp = *this;
        return pp += n;
    }
    inline PagePtr & operator ++ ()
    {
        pvPageBegin = (void *)((uptr)pvPageBegin + PAGE_SIZE_BITS);
        return *this;
    }
    inline PagePtr operator ++ (int)
    {
        PagePtr pp = *this;
        ++(*this);
        return pp;
    }
    inline PagePtr & operator -= (uptr n)
    {
        pvPageBegin = (void *)((uptr)pvPageBegin - (n << PAGE_SIZE_BITS));
        return *this;
    }
    inline PagePtr operator - (uptr n)
    {
        PagePtr pp = *this;
        return pp -= n;
    }
    inline PagePtr & operator -- ()
    {
        pvPageBegin = (void *)((uptr)pvPageBegin - PAGE_SIZE_BITS);
        return *this;
    }
    inline PagePtr operator -- (int)
    {
        PagePtr pp = *this;
        --(*this);
        return pp;
    }
    inline PagePtr operator [] (uptr n)
    {
        return *this + n;
    }
    inline operator void* ()
    {
        return pvPageBegin;
    }
};

}
