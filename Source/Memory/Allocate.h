#pragma once

namespace memory {

// <= 128 byte
void * Alloc(size_t nBytes);
void   Free(void * addr);

}
