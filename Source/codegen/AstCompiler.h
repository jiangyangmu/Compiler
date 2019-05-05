#pragma once

#include "../IR/Ast.h"
#include "../IR/Definition.h"
#include "../IR/Type.h"
#include "../Codegen/FunctionCompiler.h"

enum ASTScope {
    IN_FILE,
    IN_PARAM_LIST,
    IN_COMPOUND_STATEMENT,
    //IN_STRUCT_BODY,
};

class TypeSpecifiersBuilder;

struct AstCompileContext
{
    // Shared build state

    // scope
    std::vector<ASTScope> scope;

    // Type build state

    // type
    std::vector<Language::Type *> types;
    // id, array, function, pointer
    std::vector<StringRef> ids;
    // storage specifier, type qualifier, type specifier
    std::vector<Token::Type> storageTokens;
    std::vector<std::vector<Token::Type>> allowedStorageTokens;
    std::vector<TypeSpecifiersBuilder *> typeSpecifierBuilders;
    std::vector<int> typeQualifiers;

    // Constant build state

    // Definition build state
    std::vector<Language::DefinitionContext *>  definitionContexts;
    Language::DefinitionContext * currentFunctionDefinitionContext;

    // Function build state

    Language::FunctionContext * currentFunctionContext;
    std::vector<Language::Node *> irNodes;

    // Final output

    Language::TypeContext *                     typeContext;
    Language::ConstantContext *                 constantContext;
    Language::DefinitionContext *               globalDefinitionContext;
    std::vector<Language::FunctionContext *>    functionContexts;
};

extern AstCompileContext * CreateAstCompileContext();

extern void CompileAst(AstCompileContext * context, Ast * ast);