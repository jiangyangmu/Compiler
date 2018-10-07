#include "parser_api.h"

#include <iostream>

void emit(char c) {
    if (c == '\n')
        std::cout << c;
    else
        std::cout << c << ' ';
}

// clang-format off
int main(int argc, char * argv[])
{
    GM_BEGIN(G);
    GM_ADD(G, start);
    GM_ADD(G, expr);
    GM_ADD(G, term);
    GM_ADD(G, fact);

    start =
        expr &
        GM_CODE({ emit('\n'); });
    expr =
        term &
        *(
            '+' &
            term &
            GM_CODE({ emit('+'); })
            );
    term =
        fact &
        *(
            '*' &
            fact &
            GM_CODE({ emit('*'); })
            );
    fact =
        '1' &
        GM_CODE({ emit('1'); });

    GM_END(G);


    TokenIterator tokens("1+1*1*1+1*1+1");
    GM_RUN(G, tokens);

    tokens.reset();
    Ast *ast;
    GM_MATCH(G, tokens, ast);
    std::cout << "Ast: " << ast->DebugString() << std::endl;

    return 0;
}
// clang-format on
