#pragma once

namespace memory {

// TODO: report line number at the same scope
// TODO: add report hierarchy default::freelist::alloc addr bytes

class TraceMemory
{
public:
    TraceMemory(const char * tag = "");
    ~TraceMemory();
private:
    const char * pTagSave;
    bool bSwitchSave;
};

struct TraceNode
{
public:
    // Alloc
    TraceNode(size_t bytes, const char * tag = "");
    // Free
    TraceNode(void * addr, const char * tag = "");
    ~TraceNode();
private:
    const char * pTag;
    TraceNode * pUp;
    TraceNode * pDown;
    size_t nBytes;
    void * pAddr;

    friend class TraceLog;
};

class TraceLog
{
public:
    static void Alloc(void * addr, const char * tag = "");
    static void Free(const char * tag = "");
};

#define TRACE_MEMORY(tag) ::memory::TraceMemory __trace_##tag(#tag)

#define TRACE_MEMORY_ALLOC(tag, bytes) ::memory::TraceNode __trace_node_##tag((size_t)(bytes), #tag)
#define TRACE_MEMORY_ALLOC_LOG(tag, addr) ::memory::TraceLog::Alloc(addr, #tag)
#define TRACE_MEMORY_FREE(tag, addr) ::memory::TraceNode __trace_node_##tag((void *)(addr), #tag)
#define TRACE_MEMORY_FREE_LOG(tag, addr) ::memory::TraceLog::Free(#tag)

void DumpAddrStats();
// extern void PrintCallStack();

}
