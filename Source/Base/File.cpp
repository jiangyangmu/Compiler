#include "File.h"

#include <fstream>
#include <cassert>
#include <iostream>
#include <windows.h>

std::string GetCurrentWorkDirectory()
{
    char buf[256];
    GetCurrentDirectoryA(256, buf);
    return buf;
}

std::string GetCanonicalFileDirectory(std::string fileName)
{
    DWORD  retval = 0;
    TCHAR  buffer[4096] = TEXT("");
    TCHAR  buf[4096] = TEXT("");
    TCHAR** lppPart = { NULL };

    // Retrieve the full path name for a file. 
    // The file does not need to exist.

    retval = GetFullPathName(fileName.data(),
                             4096,
                             buffer,
                             lppPart);
    assert(retval != 0);

    std::string path = buffer;
    path.erase(path.begin() + path.find_last_of("/\\"), path.end());
    return path;
}

std::string GetFileContent(const char * fileName)
{
    std::ifstream ifs(fileName, std::ifstream::in);
    if (!ifs.is_open())
    {
        std::cerr
            << "Can't open file: " << fileName << std::endl
            << "Current dir = " << GetCurrentWorkDirectory() << std::endl;
    }
    assert(ifs.is_open());
    //ifs.seekg(3);
    return std::string(std::istreambuf_iterator<char>(ifs),
                       std::istreambuf_iterator<char>());
}

void SetFileContent(const char * fileName, const std::string & content)
{
    std::ofstream ofs(fileName, std::ifstream::out);
    ofs << content;
    ofs.close();
}

std::string ChangeFileExtention(const std::string & filename,
                                std::string from,
                                std::string to)
{
    assert(filename.size() > from.size());

    auto i1 = filename.crbegin();
    auto i2 = from.crbegin();
    while (i2 != from.crend())
    {
        assert(*i1 == *i2);
        ++i1, ++i2;
    }

    std::string newFilename = std::string(filename.cbegin(), filename.cend() - from.size());
    newFilename += to;
    return newFilename;
}
