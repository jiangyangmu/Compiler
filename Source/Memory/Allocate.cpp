#include "Allocate.h"

#include "FreeListAllocator.h"
#include "MemoryTrace.h"

namespace memory {

static GenericFreeListAllocator gfa;

void * Alloc(size_t nBytes)
{
    TRACE_MEMORY_ALLOC(Default, nBytes);

    return gfa.Alloc(nBytes);
}

void Free(void * addr)
{
    TRACE_MEMORY_FREE(Default, addr);

    gfa.Free(addr);
}

}
