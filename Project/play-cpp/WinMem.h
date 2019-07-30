#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>             // For exit

#define PAGELIMIT 80            // Number of pages to ask for

LPTSTR lpNxtPage;               // Address of the next page to ask for
DWORD dwPages = 0;              // Count of pages gotten so far
DWORD dwPageSize = 4096;        // Page size on this computer

INT PageFaultExceptionFilter(DWORD dwCode)
{
    LPVOID lpvResult;

    // If the exception is not a page fault, exit.

    if (dwCode != EXCEPTION_ACCESS_VIOLATION)
    {
        _tprintf(TEXT("Exception code = %d.\n"), dwCode);
        return EXCEPTION_EXECUTE_HANDLER;
    }

    _tprintf(TEXT("Exception is a page fault.\n"));

    // If the reserved pages are used up, exit.

    if (dwPages >= PAGELIMIT)
    {
        _tprintf(TEXT("Exception: out of pages.\n"));
        return EXCEPTION_EXECUTE_HANDLER;
    }

    // Otherwise, commit another page.

    lpvResult = VirtualAlloc(
        (LPVOID)lpNxtPage, // Next page to commit
        dwPageSize,         // Page size, in bytes
        MEM_COMMIT,         // Allocate a committed page
        PAGE_READWRITE);    // Read/write access
    if (lpvResult == NULL)
    {
        _tprintf(TEXT("VirtualAlloc failed.\n"));
        return EXCEPTION_EXECUTE_HANDLER;
    }
    else
    {
        _tprintf(TEXT("Allocating another page.\n"));
    }

    // Increment the page count, and advance lpNxtPage to the next page.

    dwPages++;
    lpNxtPage = (LPTSTR)((PCHAR)lpNxtPage + dwPageSize);

    // Continue execution where the page fault occurred.

    return EXCEPTION_CONTINUE_EXECUTION;
}

VOID ErrorExit(const wchar_t * lpMsg)
{
    _tprintf(TEXT("Error! %s with error code of %ld.\n"),
             lpMsg, GetLastError());
    exit(0);
}

VOID Test_ReserveCommitRelease(VOID)
{
    LPVOID lpvBase;               // Base address of the test memory
    LPTSTR lpPtr;                 // Generic character pointer
    BOOL bSuccess;                // Flag
    DWORD i;                      // Generic counter
    SYSTEM_INFO sSysInfo;         // Useful information about the system

    GetSystemInfo(&sSysInfo);     // Initialize the structure.

    _tprintf(TEXT("This computer has page size %d.\n"), sSysInfo.dwPageSize);

    dwPageSize = sSysInfo.dwPageSize;

    // Reserve pages in the virtual address space of the process.

    lpvBase = VirtualAlloc(
        NULL,                 // System selects address
        PAGELIMIT*dwPageSize, // Size of allocation
        MEM_RESERVE,          // Allocate reserved pages
        PAGE_NOACCESS);       // Protection = no access
    if (lpvBase == NULL)
        ErrorExit(TEXT("VirtualAlloc reserve failed."));

    lpPtr = lpNxtPage = (LPTSTR)lpvBase;

    // Use structured exception handling when accessing the pages.
    // If a page fault occurs, the exception filter is executed to
    // commit another page from the reserved block of pages.

    for (i = 0; i < PAGELIMIT*dwPageSize; i++)
    {
        __try
        {
            // Write to memory.

            lpPtr[i] = 'a';
        }

        // If there's a page fault, commit another page and try again.

        __except (PageFaultExceptionFilter(GetExceptionCode()))
        {

            // This code is executed only if the filter function
            // is unsuccessful in committing the next page.

            _tprintf(TEXT("Exiting process.\n"));

            ExitProcess(GetLastError());

        }

    }

    // Release the block of pages when you are finished using them.

    bSuccess = VirtualFree(
        lpvBase,       // Base address of block
        0,             // Bytes of committed pages
        MEM_RELEASE);  // Decommit the pages

    _tprintf(TEXT("Release %s.\n"), bSuccess ? TEXT("succeeded") : TEXT("failed"));

}

VOID Test_Reserve4GCommitRelease(VOID)
{
    LPVOID lpvBase;
    LPVOID _1GBase, _2GBase, _3GBase, _4GBase;
    LPVOID lpvNextPage;
    LPVOID lpvResult;
    BOOL bSuccess;
    SIZE_T dwSize;

    // reserve 1GB
    dwSize = 1024 * 1024 * 1024;

    _1GBase = VirtualAlloc(
        NULL,                 // System selects address
        dwSize,               // Size of allocation
        MEM_RESERVE,          // Allocate reserved pages
        PAGE_NOACCESS);       // Protection = no access
    if (_1GBase == NULL)
        ErrorExit(TEXT("VirtualAlloc reserve failed."));
    _2GBase = VirtualAlloc(
        (PCHAR)_1GBase + dwSize,
        dwSize,               // Size of allocation
        MEM_RESERVE,          // Allocate reserved pages
        PAGE_NOACCESS);       // Protection = no access
    if (_2GBase == NULL)
        ErrorExit(TEXT("VirtualAlloc reserve failed."));
    _3GBase = VirtualAlloc(
        (PCHAR)_2GBase + dwSize,
        dwSize,               // Size of allocation
        MEM_RESERVE,          // Allocate reserved pages
        PAGE_NOACCESS);       // Protection = no access
    if (_3GBase == NULL)
        ErrorExit(TEXT("VirtualAlloc reserve failed."));
    _4GBase = VirtualAlloc(
        (PCHAR)_3GBase + dwSize,
        dwSize,               // Size of allocation
        MEM_RESERVE,          // Allocate reserved pages
        PAGE_NOACCESS);       // Protection = no access
    if (_4GBase == NULL)
        ErrorExit(TEXT("VirtualAlloc reserve failed."));

    lpvNextPage = lpvBase = _1GBase;

    // 'c' key press: commit 1 mb (256 pages)
    // 'q' key press: break
    INPUT_RECORD inputRecordBuf[64];
    DWORD dwInputRecordNum = 0;
    BOOL bStop = FALSE;
    do
    {
        if (!ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE),
                              inputRecordBuf,
                              sizeof(inputRecordBuf) / sizeof(INPUT_RECORD),
                              &dwInputRecordNum))
        {
            ErrorExit(TEXT("ReadConsole failed."));
        }
        for (DWORD i = 0; i < dwInputRecordNum && !bStop; i++)
        {
            if (inputRecordBuf[i].EventType == KEY_EVENT && inputRecordBuf[i].Event.KeyEvent.bKeyDown)
            {
                switch (inputRecordBuf[i].Event.KeyEvent.wVirtualKeyCode)
                {
                    case 'N':
                        for (int p = 0; p < 256; ++p)
                        {
                            lpvResult = VirtualAlloc(
                                (LPVOID)lpvNextPage,// Next page to commit
                                dwPageSize,         // Page size, in bytes
                                MEM_COMMIT,         // Allocate a committed page
                                PAGE_READWRITE);    // Read/write access
                            if (lpvResult == NULL)
                            {
                                _tprintf(TEXT("VirtualAlloc failed.\n"));
                            }
                            else
                            {
                                _tprintf(TEXT("Allocating another page %p.\n"), lpvResult);
                            }
                            ((PCHAR)lpvResult)[0] = 'a';
                            lpvNextPage = (LPTSTR)((PCHAR)lpvNextPage + dwPageSize);
                        }
                        break;
                    case 'Q':
                        bStop = TRUE;
                        break;
                    default:
                        break;
                }
            }
        }
    } while (!bStop);

    // release
    bSuccess = VirtualFree(
        lpvBase,       // Base address of block
        0,             // Bytes of committed pages
        MEM_RELEASE);  // Decommit the pages

    _tprintf(TEXT("Release %s.\n"), bSuccess ? TEXT("succeeded") : TEXT("failed"));
}
