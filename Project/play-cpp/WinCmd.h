#pragma once

#include <windows.h>
#include <stdio.h>
#include <strsafe.h>
#include <tchar.h>
#include <string>

std::string GetCurrentWorkDirectory()
{
    char buf[256];
    GetCurrentDirectoryA(256, buf);
    return buf;
}

void ErrorMessageBox(LPCTSTR lpszPrefix)
{
    // Retrieve the system error message for the last-error code

    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError();

    lpszPrefix = TEXT("CreateProcess");

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf,
        0, NULL);

    // Display the error message and exit the process

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
        (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszPrefix) + 40) * sizeof(TCHAR));
    StringCchPrintf((LPTSTR)lpDisplayBuf,
                    LocalSize(lpDisplayBuf) / sizeof(TCHAR),
                    TEXT("%s failed with error %d: %s"),
                    lpszPrefix, dw, lpMsgBuf);
    MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
}

void Run(std::wstring cmd)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    TCHAR * lpCommandLine;
    LPCWSTR lpCurrentDirectory;

    printf("CWD = %s\n", GetCurrentWorkDirectory().data());

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    lpCommandLine = &cmd[0];
    lpCurrentDirectory = NULL;

    // Start the child process. 
    if (!CreateProcess(NULL,   // No module name (use command line)
                       lpCommandLine,  // Command line
                       NULL,           // Process handle not inheritable
                       NULL,           // Thread handle not inheritable
                       FALSE,          // Set handle inheritance to FALSE
                       0,              // No creation flags
                       NULL,           // Use parent's environment block
                       lpCurrentDirectory, // Use parent's starting directory 
                       &si,            // Pointer to STARTUPINFO structure
                       &pi)           // Pointer to PROCESS_INFORMATION structure
        )
    {
        ErrorMessageBox(TEXT("CreateProcess"));
        return;
    }

    // Wait until child process exits.
    WaitForSingleObject(pi.hProcess, INFINITE);

    // Close process and thread handles. 
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}
