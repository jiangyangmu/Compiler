#pragma once

#include "../IR/ConstantContext.h"
#include "../IR/DefinitionContext.h"
#include "../IR/FunctionContext.h"

namespace Language
{

struct x64StackLayout
{
    // layout:
    //      returnAddress                   (8-byte aligned)
    //      saved rbp                       (8-byte aligned) <-- rbp
    //      nonVolatileReg                  (8-byte aligned)
    //      localZoneSize
    //      gap                             (8-byte aligned)
    //      maxTempZoneSize
    //      gap                             (8-byte aligned)
    //      maxSpillZoneSize                (8-byte aligned)
    //      gap
    //      maxCallZoneSize                 (16-byte aligned) <-- rsp
    // allocation zone
    size_t localZoneSize;
    size_t maxTempZoneSize;
    size_t maxSpillZoneSize;
    size_t maxCallZoneSize;
    size_t stackAllocSize; // local zone + temp zone + spill zone + call zone + gaps
    size_t stackFrameSize;
    // allocation offset - "offset to previous stack frame"
    size_t registerZoneEndOffset;   // for debug
    size_t localZoneEndOffset;      // for debug
    size_t tempZoneBeginOffset;     // for temp var location computation
    size_t spillZoneEndOffset;      // needed by some expression node (e.g. EXPR_PVAL, EXPR_CVT_F2B)
    size_t callZoneEndOffset;
    std::map<Definition *, size_t> localObjectOffsets;
    // non-volatile register mask (used in stack allocation)
    size_t dirtyRegisters;
};

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
