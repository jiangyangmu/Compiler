#define HAS_LEXER

#include "parser.h"

#include <cstdint>
#include <map>
#include <string>

void Parser::parse()
{
    tu = sn_translation_unit::parse(lex);
    params.env = new Environment(new IRStorage());
    params.begin = params.end = nullptr;
    for (int pass = 0; pass <= 2; ++pass)
    {
        params.pass = pass;
        tu->visit(params);
    }

    // SymbolFactory::check();
}

