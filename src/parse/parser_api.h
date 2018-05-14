#pragma once

#include "parser.h"

// Grammer definition

#define GM_BEGIN(G) Grammer G
#define GM_ADD(G, name)                                             \
    PRODUCTION name =                                               \
        ProductionFactory::CreateWithName(Production::PROD, #name); \
    (G).add(name)
#define GM_END(G) ((G).compile())
#define GM_RUN(G, tokens) ((G).run(tokens))
#define GM_MATCH(G, tokens, ast) ((ast) = (G).match(tokens))

// Production construction

// PRODUCTION::PRODUCTION(const PRODUCTION & p);
// PRODUCTION::PRODUCTION(Token symbol);
// PRODUCTION::PRODUCTION(CodeOnject code);
// void PRODUCTION::operator=(const PRODUCTION & expr);
// void PRODUCTION::operator=(PRODUCTION && expr);
// AND
// PRODUCTION operator&(PRODUCTION p1, PRODUCTION p2);
// OR
// PRODUCTION operator|(PRODUCTION p1, PRODUCTION p2);
// REP
// PRODUCTION PRODUCTION::operator*(PRODUCTION p);
// OPT
// PRODUCTION PRODUCTION::operator~(PRODUCTION p);

// Ast customization (TODO)

// clang-format off
#define AST_PROP_BEGIN struct AstProp {
#define AST_PROP_END };
// clang-format on

// Embedded code definition and context access (TODO)

#define GM_CODE(code)                                                          \
    (ProductionBuilder::CODE([](CodeContext * __code_context__,                \
                                const std::vector<Token> * __matched_tokens__) \
                                 code))
#define GM_MATCHED_TOKEN(i)                   \
    (CHECK(__matched_tokens__ && (i) >= 0 &&  \
           (i) < __matched_tokens__->size()), \
     (*__matched_tokens__)[__matched_tokens__->size() - 1 - (i)])

#define GM_EXIST(i)
#define GM_GET_PROD_PROP(i, prop)
#define GM_GET_AST_PROP(i, prop)
#define GM_SET_AST_PROP(i, prop, value)
