#include "Allocate.h"

#include "FreeListAllocator.h"

namespace LowLevel {

static GenericFreeListAllocator gfa;

void * Alloc(size_t nBytes)
{
    return gfa.Alloc(nBytes);
}

void Free(void * addr)
{
    gfa.Free(addr);
}

}
