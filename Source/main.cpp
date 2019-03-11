#include <deque>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <streambuf>
using namespace std;

#include "ir/Ast.h"
#include "codegen/AstCompiler.h"

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
    std::cout << "Ast:" << std::endl;
    DebugPrintAst(ast);

    AstCompileContext * context = CreateAstCompileContext();
    CompileAst(context, ast);

    // Debug print
    std::cout << std::endl << "TypeContext:" << std::endl;
    Language::PrintTypeContext(context->typeContext);
    std::cout << std::endl << "DefinitionContext:" << std::endl;
    Language::PrintDefinitionContext(context->globalDefinitionContext);
    for (Language::FunctionContext * functionContext : context->functionContexts)
    {
        std::cout << std::endl << "FunctionContext:" << std::endl;
        Language::PrintFunctionContext(functionContext);
    }
}

int main(int argc, char *argv[])
{
    auto sourceCode = GetFileContent("C:\\Users\\celsi\\Documents\\Github\\jcc\\Test\\Program.txt");
    Compile(sourceCode);
    return 0;
}
