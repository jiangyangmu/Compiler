#include "MemoryTrace.h"

#include <iostream>
// #include <Windows.h>
// #include <Dbghelp.h>

#include "../Base/ErrorHandling.h"

namespace memory {

static bool bEnableMemoryTrace = false;
static const char * pTraceTag = "Default";

static TraceNode * pAllocChain = nullptr;
static TraceNode * pFreeChain = nullptr;

struct AddrStats
{
    void * pAddr;
    int nAlloc;
    int nFree;
};
static AddrStats vAddrStats[1024];
static inline AddrStats & FindAddrStat(void * addr)
{
    AddrStats * pAddrStat = vAddrStats;
    AddrStats * pEnd = vAddrStats + 1024;
    for (;;)
    {
        if (pAddrStat->pAddr == nullptr)
        {
            pAddrStat->pAddr = addr;
            return *pAddrStat;
        }
        else if (pAddrStat->pAddr == addr)
        {
            return *pAddrStat;
        }
        else
        {
            ++pAddrStat;
            ASSERT(pAddrStat < pEnd);
        }
    }
}
void DumpAddrStats()
{
    AddrStats * pBegin = vAddrStats;
    AddrStats * pEnd = vAddrStats + 1024;
    std::cout << "----------- Memory Stats Dump -----------" << std::endl;
    for (; pBegin < pEnd && pBegin->pAddr; ++pBegin)
    {
        std::cout
            << "Addr: " << pBegin->pAddr
            << " Alloc: " << pBegin->nAlloc
            << " Free: " << pBegin->nFree <<
            (pBegin->nAlloc > pBegin->nFree ? " Memory Leak !!!" : "")
            << std::endl;
    }
    std::cout << "-----------------------------------------" << std::endl;
}

TraceMemory::TraceMemory(const char * tag)
    : pTagSave(pTraceTag)
    , bSwitchSave(bEnableMemoryTrace)
{
    bEnableMemoryTrace = true;
    pTraceTag = tag;
}

TraceMemory::~TraceMemory()
{
    bEnableMemoryTrace = bSwitchSave;
    pTraceTag = pTagSave;
}

TraceNode::TraceNode(size_t bytes, const char * tag /*= ""*/)
    : pTag(tag)
    , pUp(pAllocChain)
    , pDown(nullptr)
    , nBytes(bytes)
{
    if (pAllocChain)
        pAllocChain->pDown = this;
    pAllocChain = this;
}

TraceNode::TraceNode(void * addr, const char * tag /*= ""*/)
    : pTag(tag)
    , pUp(pFreeChain)
    , pDown(nullptr)
    , pAddr(addr)
{
    if (pFreeChain)
        pFreeChain->pDown = this;
    pFreeChain = this;
}

TraceNode::~TraceNode()
{
    if (pAllocChain == this)
    {
        pAllocChain = pUp;
        if (pAllocChain)
            pAllocChain->pDown = nullptr;
    }
    else
    {
        pFreeChain = pUp;
        if (pFreeChain)
            pFreeChain->pDown = nullptr;
    }
}

void TraceLog::Alloc(void * addr, const char * tag)
{   
    if (bEnableMemoryTrace && pAllocChain)
    {
        TraceNode * pTraceChain = pAllocChain;

        while (pTraceChain->pUp)
            pTraceChain = pTraceChain->pUp;
        while (pTraceChain)
        {   
            std::cout << "::" << pTraceChain->pTag;
            pTraceChain = pTraceChain->pDown;
        }
        if (tag)
            std::cout << "::" << tag;
        std::cout << ": Alloc: " << pAllocChain->nBytes << " " << addr << std::endl;

        ++FindAddrStat(addr).nAlloc;
    }
}

void TraceLog::Free(const char * tag)
{
    if (bEnableMemoryTrace && pFreeChain)
    {
        TraceNode * pTraceChain = pFreeChain;

        while (pTraceChain->pUp)
            pTraceChain = pTraceChain->pUp;
        while (pTraceChain)
        {
            std::cout << "::" << pTraceChain->pTag;
            pTraceChain = pTraceChain->pDown;
        }
        if (tag)
            std::cout << "::" << tag;
        std::cout << ": Free: " << pFreeChain->pAddr << std::endl;

        ++FindAddrStat(pFreeChain->pAddr).nFree;
    }
}

}