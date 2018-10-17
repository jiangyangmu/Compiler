#include <deque>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <streambuf>
using namespace std;

#include "parse2/parser2.h"

extern void GenerateX64ASM(Ast * ast);

std::string GetFileContent(const char * fileName)
{
    std::ifstream ifs(fileName, std::ifstream::in);
    ifs.seekg(3);
    return std::string(std::istreambuf_iterator<char>(ifs),
                       std::istreambuf_iterator<char>());
}

std::string GetLineInput()
{
    std::string line;
    std::getline(std::cin, line);
    return line;
}

void Compile(std::string sourceCode)
{
    SourceScanner scanner(StringRef(sourceCode.data(), sourceCode.length()));
    Tokenizer tokenizer;
    tokenizer.compile(scanner);
    TokenIterator ti = tokenizer.getIterator();

    Ast * ast = ParseTranslationUnit(ti);
    DebugPrintAst(ast);

    GenerateX64ASM(ast);
}

int main(int argc, char *argv[])
{
    auto sourceCode = GetFileContent("C:\\Users\\celsi\\Documents\\Github\\jcc\\Test\\Program.txt");
    Compile(sourceCode);
    return 0;
}
