#pragma once

// #include "codegen.h"
// #include "env.h"
#include "common.h"
#include "env.h"
#include "lexer.h"
#include "symbol.h"
#include "type.h"
#include "ast.h"

#include <iostream>

void __debugPrint(string &&s);

// Pass 0: type declaration & object definition
// Pass 1: type derivation
// Pass 2: code generation
class Parser
{
    Lexer &lex;
    sn_translation_unit *tu;
    ParserParams params;

   public:
    Parser(Lexer &l) : lex(l) {}
    void parse();
    void emit()
    {
        IR_to_x64 t;
        params.env->traverse(t);
        std::cout << t.emit() << std::endl;
    }

    void DebugPrintEnvironment()
    {
        params.env->debugPrint();
    }
    void DebugPrintSyntaxTree()
    {
        __debugPrint(tu->toString());
    }
};
