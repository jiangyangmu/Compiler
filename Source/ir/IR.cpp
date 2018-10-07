#include "../parse2/parser2.h"

struct CVariable
{
    unsigned int size;
};

struct CVariableList
{
    struct CVariable var;
    struct CVariableList *next;
};

struct CProgram {
    // global var
    // TODO: global func definition
};

// always valid assertion
#define ASSERT(e)                                                          \
    (void)((!!(e)) ||                                                      \
           (_wassert(                                                      \
                _CRT_WIDE(#e), _CRT_WIDE(__FILE__), (unsigned)(__LINE__)), \
            0))

CVariableList AstToCVariableList(Ast * ast)
{
    ASSERT(ast && ast->type == DECLARATION);
    // TODO: storage, qualifier, specifier
    return {};
}
