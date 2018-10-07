#include "parser2.h"

Ast * NewAst(AstType type)
{
    Ast * ast = new Ast;
    ast->parent = ast->leftChild = ast->rightSibling = nullptr;
    ast->type = type;
    return ast;
}
Ast * NewAst(AstType type, Token token)
{
    Ast * ast = new Ast;
    ast->parent = ast->leftChild = ast->rightSibling = nullptr;
    ast->type = type;
    ast->token = token;
    return ast;
}

// always valid assertion
#define ASSERT(e)                                                          \
    (void)((!!(e)) ||                                                      \
           (_wassert(                                                      \
                _CRT_WIDE(#e), _CRT_WIDE(__FILE__), (unsigned)(__LINE__)), \
            0))

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
bool Contains(Token::Type type,
              const Token::Type * types,size_t size)
{
    for (size_t i = 0; i < size; ++i)
    {
        if (types[i] == type) return true;
    }
    return false;
}

bool First(AstType at, const Token & token)
{
    Token::Type type = token.type;

    if (at == STORAGE_CLASS_SPECIFIER)
    {
        const Token::Type first[] = {
                                        Token::KW_TYPEDEF,
                                        Token::KW_EXTERN,
                                        Token::KW_STATIC,
                                        Token::KW_AUTO,
                                        Token::KW_REGISTER,
                                    };
        return Contains(type, first, ARRAY_SIZE(first));
    }
    else if (at == TYPE_QUALIFIER)
    {
        const Token::Type first[] = {
                                        Token::KW_CONST,
                                        Token::KW_VOLATILE,
                                    };
        return Contains(type, first, ARRAY_SIZE(first));
    }
    else if (at == TYPE_SPECIFIER)
    {
        const Token::Type first[] = {
                                        Token::KW_VOID,
                                        Token::KW_CHAR,
                                        Token::KW_SHORT,
                                        Token::KW_INT,
                                        Token::KW_LONG,
                                        Token::KW_FLOAT,
                                        Token::KW_DOUBLE,
                                        Token::KW_SIGNED,
                                        Token::KW_UNSIGNED,
                                        Token::KW_STRUCT,
                                        Token::KW_UNION,
                                        Token::KW_ENUM,
                                        //Token::ID,  // must be a type name
                                    };
        return Contains(type, first, ARRAY_SIZE(first));
    }
    else if (at == DECLARATION_SPECIFIERS)
    {
        return First(STORAGE_CLASS_SPECIFIER, token) ||
               First(TYPE_SPECIFIER, token) ||
               First(TYPE_QUALIFIER, token);
    }
    else if (at == DECLARATOR)
    {
        const Token::Type first[] = {
                                        Token::OP_MUL,
                                        Token::LP,
                                        Token::ID,
                                    };
        return Contains(type, first, ARRAY_SIZE(first));
    }
    else if (at == DECLARATION)
    {
        return First(DECLARATION_SPECIFIERS, token);
    }
    else if (at == LABELED_STMT)
    {
        const Token::Type first[] = {
                                        Token::ID,
                                        Token::KW_CASE,
                                        Token::KW_DEFAULT,
                                    };
        return Contains(type, first, ARRAY_SIZE(first));
    }
    else if (at == COMPOUND_STMT)
    {
        return token.type == Token::BLK_BEGIN;
    }
    else if (at == EXPRESSION_STMT)
    {
        return token.type == Token::STMT_END ||
               First(EXPR, token);
    }
    else if (at == SELECTION_STMT)
    {
        const Token::Type first[] = {
                                        Token::KW_IF,
                                        Token::KW_SWITCH,
                                    };
        return Contains(type, first, ARRAY_SIZE(first));
    }
    else if (at == ITERATION_STMT)
    {
        const Token::Type first[] = {
                                        Token::KW_WHILE,
                                        Token::KW_DO,
                                        Token::KW_FOR,
                                    };
        return Contains(type, first, ARRAY_SIZE(first));
    }
    else if (at == JUMP_STMT)
    {
        const Token::Type first[] = {
                                        Token::KW_GOTO,
                                        Token::KW_CONTINUE,
                                        Token::KW_BREAK,
                                        Token::KW_RETURN,
                                    };
        return Contains(type, first, ARRAY_SIZE(first));
    }
    else if (at == STMT)
    {
        return First(LABELED_STMT,      token) ||
               First(COMPOUND_STMT,     token) ||
               First(EXPRESSION_STMT,   token) ||
               First(SELECTION_STMT,    token) ||
               First(ITERATION_STMT,    token) ||
               First(JUMP_STMT,         token);
    }
    else if (at == EXPR)
    {
        const Token::Type first[] = {
                                        Token::LP,
                                        Token::OP_INC,
                                        Token::OP_DEC,
                                        Token::BIT_AND,
                                        Token::OP_MUL,
                                        Token::OP_ADD,
                                        Token::OP_SUB,
                                        Token::BIT_NOT,
                                        Token::BOOL_NOT,
                                        Token::KW_SIZEOF,
                                        Token::ID,
                                        Token::CONST_INT,
                                        Token::CONST_CHAR,
                                        Token::CONST_FLOAT,
                                        Token::STRING,
                                    };
        return Contains(type, first, ARRAY_SIZE(first));
    }
    ASSERT(false);
    return false;
}

#define NEXT() (ti.next())
#define PEEK() (ti.peek())
#define PEEK_T(n) (ti.peek().type == (n))
#define SKIP_T(n) (PEEK_T(n) ? (void)ti.next(), true : false)
#define EXPECT_T(n) (ASSERT(PEEK_T(n)), (void)ti.next())

extern Ast * ParseTranslationUnit(TokenIterator & ti);
extern Ast * ParseDeclaration(TokenIterator & ti);
extern Ast * ParseDeclaration(TokenIterator & ti, Ast * declarationSpecifiers, Ast * declarator);
extern Ast * ParseDeclarationSpecifiers(TokenIterator & ti);
extern Ast * ParseTypeSpecifier(TokenIterator & ti);
extern Ast * ParseStructDeclaration(TokenIterator & ti);
extern Ast * ParseStructDeclarator(TokenIterator & ti);
extern Ast * ParseDeclaratorInitializer(TokenIterator & ti);
extern Ast * ParseDeclarator(TokenIterator & ti);
extern Ast * ParseInitializer(TokenIterator & ti);
extern Ast * ParseFunctionDefinition(TokenIterator & ti, Ast * declarationSpecifiers, Ast * declarator);
extern Ast * ParseStatememt(TokenIterator & ti);
extern Ast * ParseCompoundStatement(TokenIterator & ti);
extern Ast * ParseExpression(TokenIterator & ti);
extern Ast * ParseConstantExpr(TokenIterator & ti);
extern Ast * ParseParameterList(TokenIterator & ti);

Ast * ParseTranslationUnit(TokenIterator & ti)
{
    Ast * translationUnit                       = NewAst(TRANSLATION_UNIT);
    Ast ** pNextChild                           = &translationUnit->leftChild;

    while (!PEEK_T(Token::FILE_END))
    {
        Ast * declarationSpecifiers             = First(DECLARATION_SPECIFIERS, PEEK())
                                                  ? ParseDeclarationSpecifiers(ti)
                                                  : nullptr;

        Ast * declarator                        = First(DECLARATOR, PEEK())
                                                  ? ParseDeclarator(ti)
                                                  : nullptr;

        *pNextChild                             = (declarator != nullptr && PEEK_T(Token::BLK_BEGIN))
                                                  ? ParseFunctionDefinition(ti, declarationSpecifiers, declarator)
                                                  : ParseDeclaration(ti, declarationSpecifiers, declarator);
        pNextChild                              = &(*pNextChild)->rightSibling;
    }
    ASSERT(translationUnit->leftChild);

    return translationUnit;
}

Ast * ParseDeclaration(TokenIterator & ti)
{
    Ast * declaration                           = NewAst(DECLARATION);
    declaration->leftChild                      = ParseDeclarationSpecifiers(ti);

    Ast ** pNextDeclaratorInitializer           = &declaration->leftChild->rightSibling;
    while (pNextDeclaratorInitializer)
    {
        *pNextDeclaratorInitializer             = SKIP_T(Token::OP_COMMA)
                                                  ? ParseDeclaratorInitializer(ti)
                                                  : nullptr;
        pNextDeclaratorInitializer              = pNextDeclaratorInitializer 
                                                  ? &(*pNextDeclaratorInitializer)->rightSibling
                                                  : nullptr;
    }

    EXPECT_T(Token::STMT_END);

    return declaration;
}

Ast * ParseDeclaration(TokenIterator & ti, Ast * declarationSpecifiers, Ast * declarator)
{
    ASSERT(declarationSpecifiers);

    Ast * declaration                           = NewAst(DECLARATION);
    Ast ** pNextChild                           = &declaration->leftChild;


    *pNextChild                                 = declarationSpecifiers;
    pNextChild                                  = &(*pNextChild)->rightSibling;

    if (declarator)
    {
        Ast * declaratorInitializer             = NewAst(DECLARATOR_INITIALIZER);
        declaratorInitializer->leftChild        = declarator;
        declaratorInitializer->leftChild->rightSibling
                                                = SKIP_T(Token::ASSIGN)
                                                  ? ParseInitializer(ti)
                                                  : nullptr;
        *pNextChild                             = declaratorInitializer;
        pNextChild                              = &(*pNextChild)->rightSibling;
        while (SKIP_T(Token::OP_COMMA))
        {
            Ast * declaratorInitializer         = NewAst(DECLARATOR_INITIALIZER);
            declaratorInitializer->leftChild    = ParseDeclarator(ti);
            declaratorInitializer->leftChild->rightSibling
                                                = SKIP_T(Token::ASSIGN)
                                                  ? ParseInitializer(ti)
                                                  : nullptr;
            *pNextChild                         = declaratorInitializer;
            pNextChild                          = &(*pNextChild)->rightSibling;
        }
    }

    EXPECT_T(Token::STMT_END);

    return declaration;
}

Ast * ParseDeclarationSpecifiers(TokenIterator & ti)
{
    Ast * declarationSpecifiers                 = NewAst(DECLARATION_SPECIFIERS);
    Ast ** pNextSpecifer                        = &declarationSpecifiers->leftChild;
    while (true)
    {
        if (First(STORAGE_CLASS_SPECIFIER, PEEK()))
        {
            Ast * storageSpecifier                  = NewAst(STORAGE_CLASS_SPECIFIER, NEXT());
            *pNextSpecifer                          = storageSpecifier;
            pNextSpecifer                           = &(*pNextSpecifer)->rightSibling;
        }
        else if (First(TYPE_QUALIFIER, PEEK()))
        {
            Ast * typeQualifier                     = NewAst(TYPE_QUALIFIER, NEXT());
            *pNextSpecifer                          = typeQualifier;
            pNextSpecifer                           = &(*pNextSpecifer)->rightSibling;
        }
        else if (First(TYPE_SPECIFIER, PEEK()))
        {
            *pNextSpecifer                          = ParseTypeSpecifier(ti);
            pNextSpecifer                           = &(*pNextSpecifer)->rightSibling;
        }
        else
        {
            break;
        }
    }
    ASSERT(declarationSpecifiers->leftChild);
    return declarationSpecifiers;
}

Ast * ParseTypeSpecifier(TokenIterator & ti)
{
    ASSERT(First(TYPE_SPECIFIER, PEEK()));

    if (PEEK_T(Token::KW_STRUCT) || PEEK_T(Token::KW_UNION))
    {
        Ast * structSpecifier               = NewAst(NEXT().type == Token::KW_STRUCT
                                                        ? STRUCT_SPECIFIER
                                                        : UNION_SPECIFIER);
        Ast ** pNextChild                   = &structSpecifier->leftChild;

        if (PEEK_T(Token::ID))
        {
            *pNextChild                     = NewAst(IDENTIFIER, NEXT());
            pNextChild                      = &(*pNextChild)->rightSibling;
        }
        else
        {
            ASSERT(PEEK_T(Token::BLK_BEGIN));
        }

        if (SKIP_T(Token::BLK_BEGIN))
        {
            do
            {
                *pNextChild                 = ParseStructDeclaration(ti);
                pNextChild                  = &(*pNextChild)->rightSibling;
            }
            while (!SKIP_T(Token::BLK_END));
        }

        return structSpecifier;
    }
    else if (SKIP_T(Token::KW_ENUM))
    {
        Ast * enumSpecifier                 = NewAst(ENUM_SPECIFIER);
        Ast ** pNextChild                   = &enumSpecifier->leftChild;
                
        if (PEEK_T(Token::ID))
        {
            *pNextChild                     = NewAst(IDENTIFIER, NEXT());
            pNextChild                      = &(*pNextChild)->rightSibling;
        }
                
        if (SKIP_T(Token::BLK_BEGIN))
        {
            ASSERT(PEEK_T(Token::ID));
            *pNextChild                     = NewAst(ENUM_CONSTANT, NEXT());
            (*pNextChild)->leftChild        = SKIP_T(Token::ASSIGN)
                                                ? ParseConstantExpr(ti)
                                                : nullptr;
            pNextChild                      = &(*pNextChild)->rightSibling;
            while (SKIP_T(Token::OP_COMMA))
            {
                *pNextChild                 = NewAst(ENUM_CONSTANT, NEXT());
                (*pNextChild)->leftChild    = SKIP_T(Token::ASSIGN)
                                                ? ParseConstantExpr(ti)
                                                : nullptr;
                pNextChild                  = &(*pNextChild)->rightSibling;
            }

            EXPECT_T(Token::BLK_END);
        }
        else
        {
            ASSERT(enumSpecifier->leftChild);
        }

        return enumSpecifier;
    }
    else if (PEEK_T(Token::ID))
    {
        return NewAst(TYPEDEF_NAME, NEXT());
    }
    else
    {
        return NewAst(TYPE_SPECIFIER, NEXT());
    }
}

Ast * ParseStructDeclaration(TokenIterator & ti)
{
    Ast * structDeclaration                         = NewAst(STRUCT_DECLARATION);
    Ast ** pNextChild                               = &structDeclaration->leftChild;

    while (true)
    {
        if (First(TYPE_SPECIFIER, PEEK()))
        {
            *pNextChild                             = ParseTypeSpecifier(ti);
            pNextChild                              = &(*pNextChild)->rightSibling;
        }
        else if (First(TYPE_QUALIFIER, PEEK()))
        {
            *pNextChild                             = NewAst(TYPE_QUALIFIER, NEXT());
            pNextChild                              = &(*pNextChild)->rightSibling;
        }
        else
        {
            break;
        }
    }
    ASSERT(structDeclaration->leftChild);

    *pNextChild                                     = ParseStructDeclarator(ti);
    pNextChild                                      = &(*pNextChild)->rightSibling;
    while (SKIP_T(Token::OP_COMMA))
    {
        *pNextChild                                 = ParseStructDeclarator(ti);
        pNextChild                                  = &(*pNextChild)->rightSibling;
    }

    EXPECT_T(Token::STMT_END);
    return structDeclaration;
}

Ast * ParseStructDeclarator(TokenIterator & ti)
{
    Ast * structDeclarator                          = NewAst(STRUCT_DECLARATOR);
    Ast ** pNextChild                               = &structDeclarator->leftChild;

    if (First(DECLARATOR, PEEK()))
    {
        *pNextChild                                 = ParseDeclarator(ti);
        pNextChild                                  = &(*pNextChild)->rightSibling;
    }

    if (SKIP_T(Token::OP_COLON))
    {
        *pNextChild                                 = ParseConstantExpr(ti);
        pNextChild                                  = &(*pNextChild)->rightSibling;
    }
    else
    {
        ASSERT(structDeclarator->leftChild);
    }

    return structDeclarator;
}

Ast * ParseDeclaratorInitializer(TokenIterator & ti)
{
    Ast * declaratorInitializer                 = NewAst(DECLARATOR_INITIALIZER);
    declaratorInitializer->leftChild            = ParseDeclarator(ti);
    declaratorInitializer->leftChild->rightSibling
                                                = ParseInitializer(ti);
    return declaratorInitializer;
}

Ast * ParseDeclarator(TokenIterator & ti)
{
    Ast * declarator                            = NewAst(DECLARATOR);

    Ast ** pNextDirectDeclarator                = &declarator->leftChild;

    if (SKIP_T(Token::OP_MUL))
    {
        Ast * pointerList                       = NewAst(POINTER_LIST);

        Ast * pointer                           = NewAst(POINTER);
        pointer->leftChild                      = (PEEK_T(Token::KW_CONST) || PEEK_T(Token::KW_VOLATILE))
                                                  ? NewAst(TYPE_QUALIFIER, NEXT())
                                                  : nullptr;

        pointerList->leftChild                  = pointer;

        while (SKIP_T(Token::OP_MUL))
        {
            pointer->rightSibling               = NewAst(POINTER);
            pointer->rightSibling->leftChild    = (PEEK_T(Token::KW_CONST) || PEEK_T(Token::KW_VOLATILE))
                                                  ? NewAst(TYPE_QUALIFIER, NEXT())
                                                  : nullptr;
            pointer                             = pointer->rightSibling;
        }

        *pNextDirectDeclarator                  = pointerList;
        pNextDirectDeclarator                   = &((*pNextDirectDeclarator)->rightSibling);
    }

    if (SKIP_T(Token::LP))
    {
        *pNextDirectDeclarator                  = ParseDeclarator(ti);
        EXPECT_T(Token::RP);
    }
    else
    {
        ASSERT(PEEK_T(Token::ID));
        *pNextDirectDeclarator                  = NewAst(IDENTIFIER, NEXT());
    }
    pNextDirectDeclarator                       = &((*pNextDirectDeclarator)->rightSibling);

    while (true)
    {
        if (SKIP_T(Token::LSB))
        {
            *pNextDirectDeclarator              = !PEEK_T(Token::RSB)
                                                  ? ParseConstantExpr(ti)
                                                  : NewAst(CONSTANT_EXPR);
            pNextDirectDeclarator               = &((*pNextDirectDeclarator)->rightSibling);
            EXPECT_T(Token::RSB);
        }
        else if (SKIP_T(Token::LP))
        {
            *pNextDirectDeclarator              = !PEEK_T(Token::RP)
                                                  ? ParseParameterList(ti)
                                                  : NewAst(PARAMETER_LIST);
            pNextDirectDeclarator               = &((*pNextDirectDeclarator)->rightSibling);
            EXPECT_T(Token::RP);
        }
        else
        {
            break;
        }
    }

    return declarator;
}

Ast * ParseInitializer(TokenIterator & ti)
{
    // TODO
    ASSERT(false);
    return nullptr;
}

Ast * ParseFunctionDefinition(TokenIterator & ti, Ast * declarationSpecifiers, Ast * declarator)
{
    Ast * functionDefinition                    = NewAst(FUNCTION_DEFINITION);

    if (declarationSpecifiers)
    {
        functionDefinition->leftChild           = declarationSpecifiers;
        functionDefinition->leftChild->rightSibling
                                                = declarator;
        functionDefinition->leftChild->rightSibling->rightSibling
                                                = ParseCompoundStatement(ti);
    }
    else
    {
        functionDefinition->leftChild           = declarator;
        functionDefinition->leftChild->rightSibling
                                                = ParseCompoundStatement(ti);
    }

    return functionDefinition;
}

Ast * ParseStatememt(TokenIterator & ti)
{
    if (First(COMPOUND_STMT, PEEK()))
    {
        Ast * compoundStatement                 = NewAst(COMPOUND_STMT);

        EXPECT_T(Token::BLK_BEGIN);
        Ast ** pNextChild                       = &compoundStatement->leftChild;
        while (!PEEK_T(Token::BLK_END) && First(DECLARATION, PEEK()))
        {
            *pNextChild                         = ParseDeclaration(ti);
            pNextChild                          = &(*pNextChild)->rightSibling;
        }
        while (!PEEK_T(Token::BLK_END))
        {
            ASSERT(First(STMT, PEEK()));
            *pNextChild                         = ParseStatememt(ti);
            pNextChild                          = &(*pNextChild)->rightSibling;
        }
        EXPECT_T(Token::BLK_END);

        return compoundStatement;
    }
    if (First(LABELED_STMT, PEEK()))
    {
        Ast * labelStatement                    = NewAst(LABELED_STMT);

        if (PEEK_T(Token::ID))
        {
            labelStatement->leftChild           = NewAst(IDENTIFIER, NEXT());
            EXPECT_T(Token::OP_COLON);
            labelStatement->leftChild->rightSibling
                                                = ParseStatememt(ti);
        }
        else if (SKIP_T(Token::KW_CASE))
        {
            labelStatement->leftChild           = ParseConstantExpr(ti);
            labelStatement->leftChild->rightSibling
                                                = ParseStatememt(ti);
        }
        else
        {
            EXPECT_T(Token::KW_DEFAULT);
            EXPECT_T(Token::OP_COLON);
            labelStatement->leftChild           = ParseStatememt(ti);
        }

        return labelStatement;
    }
    else if (First(EXPRESSION_STMT, PEEK()))
    {
        Ast * expressionStatement               = NewAst(EXPRESSION_STMT);

        expressionStatement->leftChild          = !PEEK_T(Token::STMT_END)
                                                  ? ParseExpression(ti)
                                                  : nullptr;
        EXPECT_T(Token::STMT_END);
        return expressionStatement;
    }
    else if (First(SELECTION_STMT, PEEK()))
    {
        if (SKIP_T(Token::KW_IF))
        {
            EXPECT_T(Token::LP);
            Ast * expression                    = ParseExpression(ti);
            EXPECT_T(Token::RP);

            Ast * ifBlock                       = ParseStatememt(ti);

            Ast * elseBlock                     = SKIP_T(Token::KW_ELSE)
                                                  ? ParseStatememt(ti)
                                                  : nullptr;

            Ast * ifStatement                   = NewAst(elseBlock
                                                         ? IF_ELSE_STMT
                                                         : IF_STMT);
            ifStatement->leftChild              = expression;
            ifStatement->leftChild->rightSibling= ifBlock;
            ifStatement->leftChild->rightSibling->rightSibling
                                                = elseBlock;
            return ifStatement;
        }
        else
        {
            EXPECT_T(Token::KW_SWITCH);

            Ast * switchStatement               = NewAst(SWITCH_STMT);
            EXPECT_T(Token::LP);
            switchStatement->leftChild          = ParseExpression(ti);
            EXPECT_T(Token::RP);
            switchStatement->leftChild->rightSibling
                                                = ParseStatememt(ti);
            return switchStatement;
        }
    }
    else if (First(ITERATION_STMT, PEEK()))
    {
        if (SKIP_T(Token::KW_WHILE))
        {
            Ast * whileStatement                = NewAst(WHILE_STMT);
            EXPECT_T(Token::LP);
            whileStatement->leftChild           = ParseExpression(ti);
            EXPECT_T(Token::RP);
            whileStatement->leftChild->rightSibling
                                                = ParseStatememt(ti);
            return whileStatement;
        }
        else if (SKIP_T(Token::KW_DO))
        {
            Ast * doWhileStatement              = NewAst(DO_WHILE_STMT);
            doWhileStatement->leftChild         = ParseStatememt(ti);
            EXPECT_T(Token::KW_WHILE);
            EXPECT_T(Token::LP);
            doWhileStatement->leftChild->rightSibling
                                                = ParseExpression(ti);
            EXPECT_T(Token::RP);
            EXPECT_T(Token::STMT_END);
            return doWhileStatement;
        }
        else
        {
            EXPECT_T(Token::KW_FOR);

            Ast * forStatement                  = NewAst(FOR_STMT);
            EXPECT_T(Token::LP);
            forStatement->leftChild             = !PEEK_T(Token::STMT_END)
                                                  ? ParseExpression(ti)
                                                  : NewAst(EXPR);
            EXPECT_T(Token::STMT_END);
            forStatement->leftChild->rightSibling
                                                = !PEEK_T(Token::STMT_END)
                                                  ? ParseExpression(ti)
                                                  : NewAst(EXPR);
            EXPECT_T(Token::STMT_END);
            forStatement->leftChild->rightSibling->rightSibling
                                                = !PEEK_T(Token::RP)
                                                  ? ParseExpression(ti)
                                                  : NewAst(EXPR);
            EXPECT_T(Token::RP);
            forStatement->leftChild->rightSibling->rightSibling->rightSibling
                                                = ParseStatememt(ti);
            return forStatement;
        }
    }
    else
    {
        ASSERT(First(JUMP_STMT, PEEK()));

        if (SKIP_T(Token::KW_GOTO))
        {
            Ast * gotoStatement                 = NewAst(GOTO_STMT);
            ASSERT(PEEK_T(Token::ID));
            gotoStatement->leftChild            = NewAst(IDENTIFIER, NEXT());
            EXPECT_T(Token::STMT_END);
            return gotoStatement;
        }
        else if(SKIP_T(Token::KW_CONTINUE))
        {
            return NewAst(CONTINUE_STMT);
        }
        else if (SKIP_T(Token::KW_BREAK))
        {
            return NewAst(BREAK_STMT);
        }
        else
        {
            EXPECT_T(Token::KW_RETURN);
            Ast * returnStatement               = NewAst(RETURN_STMT);
            returnStatement->leftChild          = !PEEK_T(Token::STMT_END)
                                                  ? ParseExpression(ti)
                                                  : nullptr;
            return returnStatement;
        }
    }
}

Ast * ParseCompoundStatement(TokenIterator & ti)
{
    ASSERT(PEEK_T(Token::BLK_BEGIN));
    return ParseStatememt(ti);
}

Ast * ParseExpression(TokenIterator & ti)
{
    // TODO
    return NewAst(EXPR);
}

Ast * ParseConstantExpr(TokenIterator & ti)
{
    // TODO
    ASSERT(PEEK_T(Token::CONST_INT));
    return NewAst(CONSTANT_EXPR, NEXT());
}

Ast * ParseParameterList(TokenIterator & ti)
{
    Ast * firstParameterDeclaration             = NewAst(PARAMETER_DECLARATION);
    firstParameterDeclaration->leftChild        = ParseDeclarationSpecifiers(ti);
    firstParameterDeclaration->leftChild->rightSibling
                                                = ParseDeclarator(ti); // TODO: AbstractDeclarator
    bool isVarParamaterList                     = false;

    Ast * pNextParameterDeclaration             = firstParameterDeclaration;
    while (SKIP_T(Token::OP_COMMA))
    {
        if (PEEK_T(Token::VAR_PARAM))
        {
            isVarParamaterList                          = true;
            break;
        }
        else
        {
            Ast * parameterDeclaration                  = NewAst(PARAMETER_DECLARATION);
            parameterDeclaration->leftChild             = ParseDeclarationSpecifiers(ti);
            parameterDeclaration->leftChild->rightSibling
                                                        = ParseDeclarator(ti); // TODO: AbstractDeclarator
            pNextParameterDeclaration->rightSibling     = parameterDeclaration;
            pNextParameterDeclaration                   = parameterDeclaration;
        }
    }

    Ast * parameterList                         = NewAst(isVarParamaterList
                                                         ? PARAMETER_LIST_VARLIST
                                                         : PARAMETER_LIST);
    parameterList->leftChild                    = firstParameterDeclaration;
    return parameterList;
}

void DebugPrintAstImpl(Ast * ast, size_t indent)
{
    const char * astTypeString[] = {
                                    "translation unit",
                                    "external declaration",
                                    "function definition",
                                    "declaration",
                                    "declarator initializer",
                                    "declaration list",
                                    "declarator",
                                    "direct declarator",
                                    "abstract declarator",
                                    "direct abstract declarator",
                                    "parameter list (variant)",
                                    "parameter list",
                                    "parameter",
                                    "identifier",
                                    "identifier list",
                                    "type name",
                                    "pointer",
                                    "pointer list",
                                    "declaration specifiers",
                                    "storage",
                                    "qualifier",
                                    "specifier",
                                    "type qualifier list",
                                    "const",
                                    "volatile",
                                    "specifier qualifier list",
                                    "struct specifier",
                                    "union specifier",
                                    "struct declaration",
                                    "struct declarator list",
                                    "struct declarator",
                                    "enum specifier",
                                    "enum constant",
                                    "typedef name",
                                    "initializer",
                                    "initializer list",
                                    "expr",
                                    "assign expr",
                                    "cond expr",
                                    "or expr",
                                    "and expr",
                                    "bit or expr",
                                    "bit xor expr",
                                    "bit and expr",
                                    "eq expr",
                                    "rel expr",
                                    "shift expr",
                                    "add expr",
                                    "mul expr",
                                    "cast expr",
                                    "unary expr",
                                    "postfix expr",
                                    "primary expr",
                                    "argument expr list",
                                    "constant expr",
                                    "stmt",
                                    "stmt list",
                                    "labeled stmt",
                                    "compound stmt",
                                    "expression stmt",
                                    "selection stmt",
                                    "if stmt",
                                    "if else stmt",
                                    "switch stmt",
                                    "iteration stmt",
                                    "while stmt",
                                    "do while stmt",
                                    "for stmt",
                                    "jump stmt",
                                    "goto stmt",
                                    "continue stmt",
                                    "break stmt",
                                    "return stmt",
                                 };
    if (ast)
    {
        for (size_t i = 0; i < indent; ++i)
            std::cout << ' ';
        std::cout << astTypeString[(unsigned)ast->type];
        if (ast->type == IDENTIFIER)
        {
            std::cout << ' '<< ast->token.text;
        }
        std::cout << std::endl;

        for (Ast * child = ast->leftChild; child; child = child->rightSibling)
            DebugPrintAstImpl(child, indent + 2);
    }
}

void DebugPrintAst(Ast * ast)
{
    std::cout << "Ast:" << std::endl;
    DebugPrintAstImpl(ast, 0);
}
