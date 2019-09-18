#ifndef UNIT_TEST

#include <deque>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <streambuf>

#include "Base/Common.h"
#include "Base/String.h"
#include "Base/File.h"
#include "Preprocess/Preprocess.h"
#include "Parse/AstParser.h"
#include "CodeGeneration/AstCompiler.h"
#include "CodeGeneration/Translation.h"

using namespace std;

std::string Compile(std::string fileName)
{
    // 1. Token
    std::string sourceAfterPreproc;
    std::vector<Token> tokens = LexProcess(fileName, &sourceAfterPreproc);
    TokenIterator ti(tokens);

    std::cout << "Source:" << std::endl << sourceAfterPreproc << std::endl;

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

    return Language::GetProgram(program);
}

int main(int argc, char *argv[])
{
    if (argc > 1)
    {
        for (int i = 1; i < argc; ++i)
        {
            std::cout << "Input: " << argv[i] << std::endl;

            std::string destCode = Compile(argv[i]);

            SetFileContent(
                ChangeFileExtention(argv[i], ".c", ".asm").c_str(),
                destCode);
        }
    }
    else
    {
        const std::string fileName = "C:\\Users\\celsi\\Documents\\Github\\cc\\Test\\SampleProgram\\Simple.c";
        std::cout << "Input: " << fileName << std::endl;
        (void)Compile(fileName);
    }
    return 0;
}

#endif