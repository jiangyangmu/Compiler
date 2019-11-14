#include "Allocate.h"

#include "FreeListAllocator.h"

#include <iostream>

namespace memory {

static GenericFreeListAllocator gfa;

void * Alloc(size_t nBytes)
{
    void * addr = gfa.Alloc(nBytes);
    std::cout << "Alloc: " << nBytes << " " << addr << std::endl;
    return addr;
    //return gfa.Alloc(nBytes);
}

void Free(void * addr)
{
    std::cout << "Free: " << addr << std::endl;
    gfa.Free(addr);
}

}
