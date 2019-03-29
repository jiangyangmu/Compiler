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
#include "codegen/Translation.h"

std::string GetFileContent(const char * fileName)
{
    std::ifstream ifs(fileName, std::ifstream::in);
    //ifs.seekg(3);
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
    // 1. Token
    SourceScanner scanner(StringRef(sourceCode.data(), sourceCode.length()));
    Tokenizer tokenizer;
    tokenizer.compile(scanner);
    TokenIterator ti = tokenizer.getIterator();

    // 2. Ast
    Ast * ast = ParseTranslationUnit(ti);

    std::cout << "Ast:" << std::endl;
    DebugPrintAst(ast);

    // 3. IR
    AstCompileContext * context = CreateAstCompileContext();
    CompileAst(context, ast);

    std::cout << std::endl << "TypeContext:" << std::endl;
    Language::PrintTypeContext(context->typeContext);
    std::cout << std::endl << "DefinitionContext:" << std::endl;
    Language::PrintDefinitionContext(context->globalDefinitionContext);
    for (Language::FunctionContext * functionContext : context->functionContexts)
    {
        std::cout << std::endl << "FunctionContext:" << std::endl;
        Language::PrintFunctionContext(functionContext);
    }

    // 4. Assembly
    Language::x64Program program = Language::Translate(context->globalDefinitionContext,
                                                       context->constantContext,
                                                       context->functionContexts);

    std::cout << std::endl << "Program:" << std::endl;
    Language::PrintProgram(&program);
}

int main(int argc, char *argv[])
{
    auto sourceCode = GetFileContent("C:\\Users\\celsi\\Documents\\Github\\jcc\\Test\\Simple.txt");
    Compile(sourceCode);
    return 0;
}
