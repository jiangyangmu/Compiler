#pragma once

#include "../IR/Definition.h"
#include "../Codegen/FunctionCompiler.h"

namespace Language
{

struct x64Program
{
    // PUBLIC xxx
    // EXTERN xxx
    // CONST segment...
    // _BSS segment...
    // _DATA segment...
    // _TEXT segment...

    std::vector<std::string> publicLabels;
    std::vector<std::string> externLabels;
    std::string constSegment;
    std::string bssSegment;
    std::string dataSegment;
    std::string textSegment;
};

extern x64Program Translate(DefinitionContext * definitionContext,
                            ConstantContext * constantContext,
                            std::vector<FunctionContext *> & functionContexts);

extern std::string GetProgram(const x64Program & program);

// Debug

extern void PrintProgram(x64Program * program);

}
