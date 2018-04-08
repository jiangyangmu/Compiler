#include "parser.h"

#include <iostream>

typedef ProductionBuilder PB;

void emit(char c)
{
    std::cout << c << ' ';
}

int main(int argc, char * argv[])
{
    PRODUCTION start =
        ProductionFactory::CreateWithName(Production::PROD, "start");
    PRODUCTION expr =
        ProductionFactory::CreateWithName(Production::PROD, "expr");
    PRODUCTION term =
        ProductionFactory::CreateWithName(Production::PROD, "term");
    PRODUCTION fact =
        ProductionFactory::CreateWithName(Production::PROD, "fact");

    start =
        PB::AND(expr, PB::CODE([] { emit('\n'); }));
    expr =
        PB::AND(
            term,
            PB::REP(
                PB::AND(
                    PB::SYM('*'),
                    term,
                    PB::CODE([]{ emit('*'); }))));
    term =
        PB::AND(
            fact,
            PB::REP(
                PB::AND(
                    PB::SYM('+'),
                    fact,
                    PB::CODE([]{ emit('+'); }))));
    fact =
        PB::AND(
            PB::SYM('1'),
            PB::CODE([] { emit('1'); }));

    Grammer g;
    g.add(start);
    g.add(expr);
    g.add(term);
    g.add(fact);
    g.compile();

    TokenIterator tokens("1+1*1+1");
    g.run(tokens);
    return 0;
}
