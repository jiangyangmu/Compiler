#include "File.h"

#include <fstream>

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
