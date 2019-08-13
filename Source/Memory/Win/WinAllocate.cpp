#include "WinAllocate.h"

#include <windows.h>
#include <tchar.h>
#include <cstdio>
#include <cassert>

#include "../Address.h"

namespace LowLevel {

VOID
ErrorExit(const char * lpMsg)
{
    printf(("Error! %s with error code of %ld.\n"),
           lpMsg, GetLastError());
    exit(0);
}

void *
ReserveAddressSpace(size_t nReservedPage)
{
    printf(("ReserveAddressSpace: %zd pages. "), nReservedPage);

    LPVOID lpvBase;
    lpvBase = VirtualAlloc(
        NULL,                       // System selects address
        nReservedPage * PAGE_SIZE,  // Size of allocation
        MEM_RESERVE,                // Allocate reserved pages
        PAGE_NOACCESS);             // Protection = no access
    if (lpvBase == NULL)
        ErrorExit(("ReserveAddressSpace failed."));

    printf("%p\n", lpvBase);

    return lpvBase;
}

void *
ReserveAddressSpaceAndCommitPages(size_t nReservedPage)
{
    printf(("ReserveAddressSpaceAndCommitPages: %zd pages. "), nReservedPage);

    LPVOID lpvBase;
    lpvBase = VirtualAlloc(
        NULL,                       // System selects address
        nReservedPage * PAGE_SIZE,  // Size of allocation
        MEM_RESERVE | MEM_COMMIT,
        PAGE_READWRITE);
    if (lpvBase == NULL)
        ErrorExit(("ReserveAddressSpaceAndCommitPages failed."));

    printf("%p\n", lpvBase);

    return lpvBase;
}

// Also decommits all pages.
void
ReleaseAddressSpace(void * pvMemBegin, size_t nReservedPage)
{
    printf(("ReleasePage: %p %zd pages.\n"), pvMemBegin, nReservedPage);

    BOOL bSuccess;
    bSuccess = VirtualFree(
        pvMemBegin,    // Base address of block
        0,             // Bytes of committed pages
        MEM_RELEASE);  // Decommit the pages
    if (!bSuccess)
        ErrorExit(("ReleasePage failed.\n"));
}

void
CommitPage(void * pvMemBegin, size_t nPage)
{
    printf(("CommitPage: %p %zd pages.\n"), pvMemBegin, nPage);

    LPVOID lpvResult;
    lpvResult = VirtualAlloc(
        pvMemBegin,         // Next page to commit
        nPage * PAGE_SIZE,  // Page size, in bytes
        MEM_COMMIT,         // Allocate a committed page
        PAGE_READWRITE);    // Read/write access
    if (lpvResult == NULL)
        ErrorExit(("CommitPage failed.\n"));
}

// Safe to call over uncommited pages.
void
DecommitPage(void * pvMemBegin, size_t nPage)
{
    printf(("DecommitPage: %p %zd pages.\n"), pvMemBegin, nPage);

    BOOL bSuccess;
    bSuccess = VirtualFree(
        pvMemBegin,
        nPage * PAGE_SIZE,
        MEM_DECOMMIT);
    if (!bSuccess)
        ErrorExit(("DecommitPage failed.\n"));
}

}
