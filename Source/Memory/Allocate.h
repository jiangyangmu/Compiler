#pragma once

namespace LowLevel {

// TODO: FreeListAllocator
// TODO: Generic Alloc/Free

void * MemAlloc(size_t nBytes);
void MemFree(void * addr);

}
