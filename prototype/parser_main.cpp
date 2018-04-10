#include "parser.h"

#include <iostream>

typedef ProductionBuilder PB;

void emit(char c) {
    if (c == '\n')
        std::cout << c;
    else
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
        PB::AND(expr,
                PB::CODE([](CodeContext *c) {
                    emit('\n');
                    if (c)
                        std::cout << c->DebugString() << std::endl; }));
    expr =
        PB::AND(
            term,
            PB::REP(
                PB::AND(
                    PB::SYM('*'),
                    term,
                    PB::CODE([](CodeContext *c) {
                        emit('*');
                        if (c)
                            std::cout << c->DebugString() << std::endl; }))));
    term =
        PB::AND(
            fact,
            PB::REP(
                PB::AND(
                    PB::SYM('+'),
                    fact,
                    PB::CODE([](CodeContext *c) {
                        emit('+');
                        if (c)
                            std::cout << c->DebugString() << std::endl; }))));
    fact =
        PB::AND(
            PB::SYM('1'),
            PB::CODE([](CodeContext *c) {
                emit('1');
                if (c)
                    std::cout << c->DebugString() << std::endl; }));

    Grammer g;
    g.add(start);
    g.add(expr);
    g.add(term);
    g.add(fact);
    g.compile();

    TokenIterator tokens("1+1*1+1");
    g.run(tokens);

    tokens.reset();
    Ast *ast = g.match(tokens);

    std::cout << "Ast: " << ast->DebugString() << std::endl;
    return 0;
}
