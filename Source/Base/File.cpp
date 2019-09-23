#include "File.h"

#include <fstream>
#include <cassert>

std::string GetFileContent(const char * fileName)
{
    std::ifstream ifs(fileName, std::ifstream::in);
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