#include <deque>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
using namespace std;

#include "parse2/parser2.h"

extern void GenerateX64ASM(Ast * ast);

int main(int argc, char *argv[])
{
    std::string line;
    while (true)
    {
        std::getline(std::cin, line);
        if (line.empty())
            break;

        SourceScanner scanner(StringRef(line.data(), line.length()));
        Tokenizer tokenizer;
        tokenizer.compile(scanner);
        TokenIterator ti = tokenizer.getIterator();

        Ast * ast = ParseTranslationUnit(ti);
        DebugPrintAst(ast);

        GenerateX64ASM(ast);
    }
    return 0;
}
