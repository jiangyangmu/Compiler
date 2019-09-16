#include "AstParser.h"

#include <iostream>

#include "../Base/common.h"

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
        return Contains(type, first, ELEMENT_COUNT(first));
    }
    else if (at == TYPE_QUALIFIER)
    {
        const Token::Type first[] = {
                                        Token::KW_CONST,
                                        Token::KW_VOLATILE,
                                    };
        return Contains(type, first, ELEMENT_COUNT(first));
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
        return Contains(type, first, ELEMENT_COUNT(first));
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
                                        Token::LPAREN,
                                        Token::ID,
                                    };
        return Contains(type, first, ELEMENT_COUNT(first));
    }
    else if (at == ABSTRACT_DECLARATOR)
    {
        const Token::Type first[] = {
                                        Token::OP_MUL,
                                        Token::LPAREN,
                                        Token::LSB,
                                    };
        return Contains(type, first, ELEMENT_COUNT(first));
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
        return Contains(type, first, ELEMENT_COUNT(first));
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
        return Contains(type, first, ELEMENT_COUNT(first));
    }
    else if (at == ITERATION_STMT)
    {
        const Token::Type first[] = {
                                        Token::KW_WHILE,
                                        Token::KW_DO,
                                        Token::KW_FOR,
                                    };
        return Contains(type, first, ELEMENT_COUNT(first));
    }
    else if (at == JUMP_STMT)
    {
        const Token::Type first[] = {
                                        Token::KW_GOTO,
                                        Token::KW_CONTINUE,
                                        Token::KW_BREAK,
                                        Token::KW_RETURN,
                                    };
        return Contains(type, first, ELEMENT_COUNT(first));
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
                                        Token::LPAREN,
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
        return Contains(type, first, ELEMENT_COUNT(first));
    }
    ASSERT(false);
    return false;
}

#define NEXT() (ti.next())
#define PEEK() (ti.peek())
#define PEEK2() (ti.peekN(1))
#define PEEK_T(n) (ti.peek().type == (n))
#define PEEK2_T(n) (ti.peekN(1).type == (n))
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
extern Ast * ParseCommaExpr(TokenIterator & ti, Ast * leftCommaExpr = nullptr);
extern Ast * ParseAssignExpr(TokenIterator & ti);
extern Ast * ParseCondExpr(TokenIterator & ti);
extern Ast * ParseOrExpr(TokenIterator & ti, Ast * leftOrExpr = nullptr);
extern Ast * ParseAndExpr(TokenIterator & ti, Ast * leftAndExpr = nullptr);
extern Ast * ParseBitOrExpr(TokenIterator & ti, Ast * leftBitOrExpr = nullptr);
extern Ast * ParseBitXorExpr(TokenIterator & ti, Ast * leftBitXorExpr = nullptr);
extern Ast * ParseBitAndExpr(TokenIterator & ti, Ast * leftBitAndExpr = nullptr);
extern Ast * ParseEqExpr(TokenIterator & ti, Ast * leftEqExpr = nullptr);
extern Ast * ParseRelExpr(TokenIterator & ti, Ast * leftRelExpr = nullptr);
extern Ast * ParseShiftExpr(TokenIterator & ti, Ast * leftShiftExpr = nullptr);
extern Ast * ParseAddExpr(TokenIterator & ti, Ast * leftAddExpr = nullptr);
extern Ast * ParseMulExpr(TokenIterator & ti, Ast * leftMulExpr = nullptr);
extern Ast * ParseCastExpr(TokenIterator & ti);
extern Ast * ParseUnaryExpr(TokenIterator & ti);
extern Ast * ParsePostfixExpr(TokenIterator & ti, Ast * leftPostfixExpr = nullptr);
extern Ast * ParsePrimaryExpr(TokenIterator & ti);
extern Ast * ParseArgumentList(TokenIterator & ti);
extern Ast * ParseConstantExpr(TokenIterator & ti);
extern Ast * ParseParameterList(TokenIterator & ti);

Ast * ParseTranslationUnit(TokenIterator & ti)
{
    Ast * translationUnit                       = NewAst(TRANSLATION_UNIT);
    Ast ** pNextChild                           = &translationUnit->leftChild;

    while (ti.has())
    {
        ASSERT(First(DECLARATION_SPECIFIERS, PEEK()));
        Ast * declarationSpecifiers             = ParseDeclarationSpecifiers(ti);

        Ast * declarator                        = First(DECLARATOR, PEEK())
                                                  ? ParseDeclarator(ti)
                                                  : nullptr;

        *pNextChild                             = (declarator != nullptr && PEEK_T(Token::BLK_BEGIN))
                                                  ? ParseFunctionDefinition(ti, declarationSpecifiers, declarator)
                                                  : ParseDeclaration(ti, declarationSpecifiers, declarator);
        pNextChild                              = &(*pNextChild)->rightSibling;
    }
    //ASSERT(translationUnit->leftChild);

    return translationUnit;
}

Ast * ParseDeclaration(TokenIterator & ti)
{
    Ast * declaration                           = NewAst(DECLARATION);
    declaration->leftChild                      = ParseDeclarationSpecifiers(ti);

    if (!PEEK_T(Token::STMT_END))
    {
        Ast ** pNextDeclaratorInitializer       = &declaration->leftChild->rightSibling;
        *pNextDeclaratorInitializer             = ParseDeclaratorInitializer(ti);
        pNextDeclaratorInitializer              = &(*pNextDeclaratorInitializer)->rightSibling;
        while (SKIP_T(Token::OP_COMMA))
        {
            *pNextDeclaratorInitializer         = ParseDeclaratorInitializer(ti);
            pNextDeclaratorInitializer          = &(*pNextDeclaratorInitializer)->rightSibling;
        }
    }

    EXPECT_T(Token::STMT_END);

    return declaration;
}

Ast * ParseDeclaration(TokenIterator & ti, Ast * declarationSpecifiers, Ast * declarator)
{
    ASSERT(declarationSpecifiers);

    Ast * declaration                           = NewAst(DECLARATION);
    declaration->leftChild                      = declarationSpecifiers;

    if (declarator)
    {
        Ast ** pNextDeclaratorInitializer = &declaration->leftChild->rightSibling;

        Ast * declaratorInitializer             = NewAst(DECLARATOR_INITIALIZER);
        declaratorInitializer->leftChild        = declarator;
        declaratorInitializer->leftChild->rightSibling
                                                = SKIP_T(Token::ASSIGN)
                                                  ? ParseInitializer(ti)
                                                  : nullptr;
        *pNextDeclaratorInitializer             = declaratorInitializer;
        pNextDeclaratorInitializer              = &(*pNextDeclaratorInitializer)->rightSibling;
        while (SKIP_T(Token::OP_COMMA))
        {
            *pNextDeclaratorInitializer = ParseDeclaratorInitializer(ti);
            pNextDeclaratorInitializer = &(*pNextDeclaratorInitializer)->rightSibling;
        }
    }

    EXPECT_T(Token::STMT_END);

    return declaration;
}

bool IsAstTypeQualifier(AstType type)
{
    return type == TYPE_QUALIFIER;
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

bool IsAstTypeSpecifier(AstType type)
{
    return type == STRUCT_SPECIFIER ||
           type == UNION_SPECIFIER ||
           type == ENUM_SPECIFIER ||
           type == TYPEDEF_NAME ||
           type == TYPE_SPECIFIER;
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
        else
        {
            ASSERT(PEEK_T(Token::BLK_BEGIN));
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
                ASSERT(PEEK_T(Token::ID));
                *pNextChild                 = NewAst(ENUM_CONSTANT, NEXT());
                (*pNextChild)->leftChild    = SKIP_T(Token::ASSIGN)
                                              ? ParseConstantExpr(ti)
                                              : nullptr;
                pNextChild                  = &(*pNextChild)->rightSibling;
            }

            EXPECT_T(Token::BLK_END);
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
    structDeclarator->leftChild                     = ParseDeclarator(ti);
    return structDeclarator;
}

Ast * ParseDeclaratorInitializer(TokenIterator & ti)
{
    Ast * declaratorInitializer                 = NewAst(DECLARATOR_INITIALIZER);
    declaratorInitializer->leftChild            = ParseDeclarator(ti);
    declaratorInitializer->leftChild->rightSibling
                                                = SKIP_T(Token::ASSIGN)
                                                  ? ParseInitializer(ti)
                                                  : nullptr;
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

    if (SKIP_T(Token::LPAREN))
    {
        *pNextDirectDeclarator                  = ParseDeclarator(ti);
        EXPECT_T(Token::RPAREN);
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
        else if (SKIP_T(Token::LPAREN))
        {
            *pNextDirectDeclarator              = !PEEK_T(Token::RPAREN)
                                                  ? ParseParameterList(ti)
                                                  : NewAst(PARAMETER_LIST);
            pNextDirectDeclarator               = &((*pNextDirectDeclarator)->rightSibling);
            EXPECT_T(Token::RPAREN);
        }
        else
        {
            break;
        }
    }

    return declarator;
}

Ast * ParseAbstractDeclarator(TokenIterator & ti)
{
    Ast * declarator                            = NewAst(ABSTRACT_DECLARATOR);

    Ast ** pNextChild                           = &declarator->leftChild;

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

        *pNextChild                             = pointerList;
        pNextChild                              = &((*pNextChild)->rightSibling);
    }

    // abstract_declarator?
    if (PEEK_T(Token::LPAREN) && First(ABSTRACT_DECLARATOR, PEEK2()))
    {
        EXPECT_T(Token::LPAREN);
        *pNextChild                             = ParseAbstractDeclarator(ti);
        EXPECT_T(Token::RPAREN);
        pNextChild                              = &((*pNextChild)->rightSibling);
    }

    // (array|func)*
    if(PEEK_T(Token::LPAREN) || PEEK_T(Token::LSB))
    {
        while (true)
        {
            if (SKIP_T(Token::LSB))
            {
                *pNextChild                     = !PEEK_T(Token::RSB)
                                                  ? ParseConstantExpr(ti)
                                                  : NewAst(CONSTANT_EXPR);
                pNextChild                      = &((*pNextChild)->rightSibling);
                EXPECT_T(Token::RSB);
            }
            else if (SKIP_T(Token::LPAREN))
            {
                *pNextChild                     = !PEEK_T(Token::RPAREN)
                                                  ? ParseParameterList(ti)
                                                  : NewAst(PARAMETER_LIST);
                pNextChild                      = &((*pNextChild)->rightSibling);
                EXPECT_T(Token::RPAREN);
            }
            else
            {
                break;
            }
        }
    }

    ASSERT(declarator->leftChild);

    return declarator;
}

Ast * ParseTypename(TokenIterator & ti)
{
    Ast * typeName                              = NewAst(TYPE_NAME);
    Ast ** pNextChild                           = &typeName->leftChild;

    while (true)
    {
        if (First(TYPE_SPECIFIER, PEEK()))
        {
            *pNextChild                         = ParseTypeSpecifier(ti);
            pNextChild                          = &(*pNextChild)->rightSibling;
        }
        else if (First(TYPE_QUALIFIER, PEEK()))
        {
            *pNextChild                         = NewAst(TYPE_QUALIFIER, NEXT());
            pNextChild                          = &(*pNextChild)->rightSibling;
        }
        else
        {
            break;
        }
    }
    ASSERT(typeName->leftChild);

    if (First(ABSTRACT_DECLARATOR, PEEK()))
    {
        *pNextChild                             = ParseAbstractDeclarator(ti);
        pNextChild                              = &(*pNextChild)->rightSibling;
    }

    return typeName;
}

Ast * ParseInitializer(TokenIterator & ti)
{
    // TODO
    ASSERT(false);
    return nullptr;
}

Ast * ParseFunctionDefinition(TokenIterator & ti, Ast * declarationSpecifiers, Ast * declarator)
{
    ASSERT(declarationSpecifiers && declarator);

    Ast * functionDefinition                    = NewAst(FUNCTION_DEFINITION);

    functionDefinition->leftChild               = declarationSpecifiers;
    functionDefinition->leftChild->rightSibling
                                                = declarator;
    functionDefinition->leftChild->rightSibling->rightSibling
                                                = ParseCompoundStatement(ti);

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
    if (PEEK_T(Token::KW_CASE) ||
        PEEK_T(Token::KW_DEFAULT) ||
        (PEEK_T(Token::ID) && PEEK2_T(Token::OP_COLON)))
    {
        if (PEEK_T(Token::ID))
        {
            // No goto/label statement
            ASSERT(false);
            return nullptr;
        }
        else if (SKIP_T(Token::KW_CASE))
        {
            Ast * caseStatement                 = NewAst(CASE_STMT);
            caseStatement->leftChild            = ParseConstantExpr(ti);
            EXPECT_T(Token::OP_COLON);
            caseStatement->leftChild->rightSibling
                                                = ParseStatememt(ti);
            return caseStatement;
        }
        else
        {
            Ast * defaultStatement              = NewAst(DEFAULT_STMT);
            EXPECT_T(Token::KW_DEFAULT);
            EXPECT_T(Token::OP_COLON);
            defaultStatement->leftChild         = ParseStatememt(ti);
            return defaultStatement;
        }
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
            EXPECT_T(Token::LPAREN);
            Ast * expression                    = ParseExpression(ti);
            EXPECT_T(Token::RPAREN);

            Ast * ifBlock                       = ParseStatememt(ti);

            Ast * elseBlock                     = SKIP_T(Token::KW_ELSE)
                                                  ? ParseStatememt(ti)
                                                  : nullptr;

            Ast * ifStatement                   = NewAst(IF_ELSE_STMT);
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
            EXPECT_T(Token::LPAREN);
            switchStatement->leftChild          = ParseExpression(ti);
            EXPECT_T(Token::RPAREN);
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
            EXPECT_T(Token::LPAREN);
            whileStatement->leftChild           = ParseExpression(ti);
            EXPECT_T(Token::RPAREN);
            whileStatement->leftChild->rightSibling
                                                = ParseStatememt(ti);
            return whileStatement;
        }
        else if (SKIP_T(Token::KW_DO))
        {
            Ast * doWhileStatement              = NewAst(DO_WHILE_STMT);
            doWhileStatement->leftChild         = ParseStatememt(ti);
            EXPECT_T(Token::KW_WHILE);
            EXPECT_T(Token::LPAREN);
            doWhileStatement->leftChild->rightSibling
                                                = ParseExpression(ti);
            EXPECT_T(Token::RPAREN);
            EXPECT_T(Token::STMT_END);
            return doWhileStatement;
        }
        else
        {
            EXPECT_T(Token::KW_FOR);

            Ast * forStatement                  = NewAst(FOR_STMT);
            EXPECT_T(Token::LPAREN);
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
                                                = !PEEK_T(Token::RPAREN)
                                                  ? ParseExpression(ti)
                                                  : NewAst(EXPR);
            EXPECT_T(Token::RPAREN);
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
            // No goto/label statement
            ASSERT(false);
            return nullptr;
        }
        else if(SKIP_T(Token::KW_CONTINUE))
        {
            EXPECT_T(Token::STMT_END);
            return NewAst(CONTINUE_STMT);
        }
        else if (SKIP_T(Token::KW_BREAK))
        {
            EXPECT_T(Token::STMT_END);
            return NewAst(BREAK_STMT);
        }
        else
        {
            EXPECT_T(Token::KW_RETURN);
            Ast * returnStatement               = NewAst(RETURN_STMT);
            returnStatement->leftChild          = !PEEK_T(Token::STMT_END)
                                                  ? ParseExpression(ti)
                                                  : nullptr;
            EXPECT_T(Token::STMT_END);
            return returnStatement;
        }
    }
}

Ast * ParseCompoundStatement(TokenIterator & ti)
{
    ASSERT(PEEK_T(Token::BLK_BEGIN));
    return ParseStatememt(ti);
}

// TODO
Ast * ParseExpression(TokenIterator & ti)
{
    return ParseCommaExpr(ti);
}
Ast * ParseCommaExpr(TokenIterator & ti, Ast * leftCommaExpr /* = nullptr */)
{
    Ast * left                                  = leftCommaExpr
                                                  ? leftCommaExpr
                                                  : ParseAssignExpr(ti);
    if (SKIP_T(Token::OP_COMMA))
    {
        Ast * commaExpr                         = NewAst(COMMA_EXPR);
        commaExpr->leftChild                    = left;
        commaExpr->leftChild->rightSibling      = ParseAssignExpr(ti);
        return ParseCommaExpr(ti, commaExpr);
    }
    else
        return left;
}
Ast * ParseAssignExpr(TokenIterator & ti)
{
    Ast * condExpr                              = ParseCondExpr(ti);
    if (IsAssignOp(PEEK().type))
    {
        Ast * assignExpr                        = NewAst(ASSIGN_EXPR);
        assignExpr->token                       = NEXT();
        assignExpr->leftChild                   = condExpr;
        assignExpr->leftChild->rightSibling     = ParseAssignExpr(ti);
        return assignExpr;
    }
    else
        return condExpr;
}
Ast * ParseCondExpr(TokenIterator & ti)
{
    Ast * orExpr                                = ParseOrExpr(ti);
    if (SKIP_T(Token::OP_QMARK))
    {
        Ast * condExpr                          = NewAst(COND_EXPR);
        condExpr->leftChild                     = orExpr;
        condExpr->leftChild->rightSibling       = ParseExpression(ti);
        EXPECT_T(Token::OP_COLON);
        condExpr->leftChild->rightSibling->rightSibling
                                                = ParseCondExpr(ti);
        return condExpr;
    }
    else
        return orExpr;
}
Ast * ParseOrExpr(TokenIterator & ti, Ast * leftOrExpr /* = nullptr */)
{
    Ast * left                                  = leftOrExpr
                                                  ? leftOrExpr
                                                  : ParseAndExpr(ti);
    if (SKIP_T(Token::BOOL_OR))
    {
        Ast * orExpr                            = NewAst(OR_EXPR);
        orExpr->leftChild                       = left;
        orExpr->leftChild->rightSibling         = ParseAndExpr(ti);
        return ParseOrExpr(ti, orExpr);
    }
    else
        return left;
}
Ast * ParseAndExpr(TokenIterator & ti, Ast * leftAndExpr /* = nullptr */)
{
    Ast * left                                  = leftAndExpr
                                                  ? leftAndExpr
                                                  : ParseBitOrExpr(ti);
    if (SKIP_T(Token::BOOL_AND))
    {
        Ast * andExpr                           = NewAst(AND_EXPR);
        andExpr->leftChild                      = left;
        andExpr->leftChild->rightSibling        = ParseBitOrExpr(ti);
        return ParseAndExpr(ti, andExpr);
    }
    else
        return left;
}
Ast * ParseBitOrExpr(TokenIterator & ti, Ast * leftBitOrExpr /* = nullptr */)
{
    Ast * left                                  = leftBitOrExpr
                                                  ? leftBitOrExpr
                                                  : ParseBitXorExpr(ti);
    if (SKIP_T(Token::BIT_OR))
    {
        Ast * bitOrExpr                         = NewAst(BIT_OR_EXPR);
        bitOrExpr->leftChild                    = left;
        bitOrExpr->leftChild->rightSibling      = ParseBitXorExpr(ti);
        return ParseBitOrExpr(ti, bitOrExpr);
    }
    else
        return left;
}
Ast * ParseBitXorExpr(TokenIterator & ti, Ast * leftBitXorExpr /* = nullptr */)
{
    Ast * left                                  = leftBitXorExpr
                                                  ? leftBitXorExpr
                                                  : ParseBitAndExpr(ti);
    if (SKIP_T(Token::BIT_XOR))
    {
        Ast * bitXorExpr                        = NewAst(BIT_XOR_EXPR);
        bitXorExpr->leftChild                   = left;
        bitXorExpr->leftChild->rightSibling     = ParseBitAndExpr(ti);
        return ParseBitXorExpr(ti, bitXorExpr);
    }
    else
        return left;
}
Ast * ParseBitAndExpr(TokenIterator & ti, Ast * leftBitAndExpr /* = nullptr */)
{
    Ast * left                                  = leftBitAndExpr
                                                  ? leftBitAndExpr
                                                  : ParseEqExpr(ti);
    if (SKIP_T(Token::BIT_AND))
    {
        Ast * bitAndExpr                        = NewAst(BIT_AND_EXPR);
        bitAndExpr->leftChild                   = left;
        bitAndExpr->leftChild->rightSibling     = ParseEqExpr(ti);
        return ParseBitAndExpr(ti, bitAndExpr);
    }
    else
        return left;
}
Ast * ParseEqExpr(TokenIterator & ti, Ast * leftEqExpr/* = nullptr */)
{
    Ast * left                                  = leftEqExpr
                                                  ? leftEqExpr
                                                  : ParseRelExpr(ti);
    if (IsEqOp(PEEK().type))
    {
        Ast * eqExpr                            = NewAst(EQ_EXPR);
        eqExpr->leftChild                       = left;
        eqExpr->token                           = NEXT();
        eqExpr->leftChild->rightSibling         = ParseRelExpr(ti);
        return ParseEqExpr(ti, eqExpr);
    }
    else
        return left;
}
Ast * ParseRelExpr(TokenIterator & ti, Ast * leftRelExpr /* = nullptr */)
{
    Ast * left                                  = leftRelExpr
                                                  ? leftRelExpr
                                                  : ParseShiftExpr(ti);
    if (IsRelOp(PEEK().type))
    {
        Ast * relExpr                           = NewAst(REL_EXPR);
        relExpr->leftChild                      = left;
        relExpr->token                          = NEXT();
        relExpr->leftChild->rightSibling        = ParseShiftExpr(ti);
        return ParseRelExpr(ti, relExpr);
    }
    else
        return left;
}
Ast * ParseShiftExpr(TokenIterator & ti, Ast * leftShiftExpr /* = nullptr */)
{
    Ast * left                                  = leftShiftExpr
                                                  ? leftShiftExpr
                                                  : ParseAddExpr(ti);
    if (IsShiftOp(PEEK().type))
    {
        Ast * shiftExpr                         = NewAst(SHIFT_EXPR);
        shiftExpr->leftChild                    = left;
        shiftExpr->token                        = NEXT();
        shiftExpr->leftChild->rightSibling      = ParseAddExpr(ti);
        return ParseShiftExpr(ti, shiftExpr);
    }
    else
        return left;
}
Ast * ParseAddExpr(TokenIterator & ti, Ast * leftAddExpr /* = nullptr */)
{
    Ast * left                                  = leftAddExpr
                                                  ? leftAddExpr
                                                  : ParseMulExpr(ti);
    if (IsAddOp(PEEK().type))
    {
        Ast * addExpr                           = NewAst(ADD_EXPR);
        addExpr->leftChild                      = left;
        addExpr->token                          = NEXT();
        addExpr->leftChild->rightSibling        = ParseMulExpr(ti);
        return ParseAddExpr(ti, addExpr);
    }
    else
        return left;
}
Ast * ParseMulExpr(TokenIterator & ti, Ast * leftMulExpr /* = nullptr */)
{
    Ast * left                                  = leftMulExpr
                                                  ? leftMulExpr
                                                  : ParseCastExpr(ti);
    if (IsMulOp(PEEK().type))
    {
        Ast * mulExpr                           = NewAst(MUL_EXPR);
        mulExpr->leftChild                      = left;
        mulExpr->token                          = NEXT();
        mulExpr->leftChild->rightSibling        = ParseCastExpr(ti);
        return ParseMulExpr(ti, mulExpr);
    }
    else
        return left;
}
Ast * ParseCastExpr(TokenIterator & ti)
{
    if (PEEK_T(Token::LPAREN) && (First(TYPE_SPECIFIER, PEEK2()) || First(TYPE_QUALIFIER, PEEK2())))
    {
        Ast * castExpr                          = NewAst(CAST_EXPR);
        EXPECT_T(Token::LPAREN);
        castExpr->leftChild                     = ParseTypename(ti);
        EXPECT_T(Token::RPAREN);
        castExpr->leftChild->rightSibling       = ParseCastExpr(ti);
        return castExpr;
    }
    else
    {
        return ParseUnaryExpr(ti);
    }
}
Ast * ParseUnaryExpr(TokenIterator & ti)
{
    if (PEEK_T(Token::OP_INC) || PEEK_T(Token::OP_DEC))
    {
        Ast * unaryExpr                         = NewAst(UNARY_EXPR);
        unaryExpr->token                        = NEXT();
        unaryExpr->leftChild                    = ParseUnaryExpr(ti);
        return unaryExpr;
    }
    else if (IsUnaryOp(PEEK().type))
    {
        Ast * unaryExpr                         = NewAst(UNARY_EXPR);
        unaryExpr->token                        = NEXT();
        unaryExpr->leftChild                    = ParseCastExpr(ti);
        return unaryExpr;
    }
    else if (PEEK_T(Token::KW_SIZEOF))
    {
        Ast * unaryExpr                         = NewAst(UNARY_EXPR);
        unaryExpr->token                        = NEXT();

        if (PEEK_T(Token::LPAREN) && (First(TYPE_SPECIFIER, PEEK2()) || First(TYPE_QUALIFIER, PEEK2())))
        {
            EXPECT_T(Token::LPAREN);
            unaryExpr->leftChild                = ParseTypename(ti);
            EXPECT_T(Token::RPAREN);
        }
        else
        {
            unaryExpr->leftChild                = ParseUnaryExpr(ti);
        }

        return unaryExpr;
    }
    else
    {
        return ParsePostfixExpr(ti);
    }
}
Ast * ParsePostfixExpr(TokenIterator & ti, Ast * leftPostfixExpr /* = nullptr*/)
{
    Ast * left                                  = leftPostfixExpr
                                                  ? leftPostfixExpr
                                                  : ParsePrimaryExpr(ti);
    if (SKIP_T(Token::LSB))
    {
        Ast * postfixExpr                       = NewAst(POSTFIX_EXPR);
        postfixExpr->leftChild                  = left;
        postfixExpr->leftChild->rightSibling    = ParseExpression(ti);
        EXPECT_T(Token::RSB);
        return ParsePostfixExpr(ti, postfixExpr);
    }
    else if (SKIP_T(Token::LPAREN))
    {
        Ast * postfixExpr                       = NewAst(POSTFIX_EXPR);
        postfixExpr->leftChild                  = left;
        postfixExpr->leftChild->rightSibling    = !PEEK_T(Token::RPAREN)
                                                  ? ParseArgumentList(ti)
                                                  : NewAst(ARGUMENT_EXPR_LIST);
        EXPECT_T(Token::RPAREN);
        return ParsePostfixExpr(ti, postfixExpr);
    }
    else if (PEEK_T(Token::OP_DOT)|| PEEK_T(Token::OP_POINTTO))
    {
        Ast * postfixExpr                       = NewAst(POSTFIX_EXPR);
        postfixExpr->leftChild                  = left;
        postfixExpr->token                      = NEXT();
        ASSERT(PEEK_T(Token::ID));
        postfixExpr->leftChild->rightSibling    = NewAst(IDENTIFIER, NEXT());
        return ParsePostfixExpr(ti, postfixExpr);
    }
    else if (PEEK_T(Token::OP_INC) || PEEK_T(Token::OP_DEC))
    {
        Ast * postfixExpr                       = NewAst(POSTFIX_EXPR);
        postfixExpr->leftChild                  = left;
        postfixExpr->token                      = NEXT();
        return ParsePostfixExpr(ti, postfixExpr);
    }
    else
    {
        return left;
    }
}
Ast * ParsePrimaryExpr(TokenIterator & ti)
{
    if (PEEK_T(Token::ID) ||
        PEEK_T(Token::CONST_INT) ||
        PEEK_T(Token::CONST_CHAR) ||
        PEEK_T(Token::CONST_FLOAT) ||
        PEEK_T(Token::STRING))
    {
        return NewAst(PRIMARY_EXPR, NEXT());
    }
    else
    {
        EXPECT_T(Token::LPAREN);
        Ast * expr                              = ParseExpression(ti);
        EXPECT_T(Token::RPAREN);
        return expr;
    }
}
Ast * ParseArgumentList(TokenIterator & ti)
{
    Ast * argumentList                          = NewAst(ARGUMENT_EXPR_LIST);
    Ast ** pNextArgument                        = &argumentList->leftChild;

    *pNextArgument                              = ParseAssignExpr(ti);
    pNextArgument                               = &(*pNextArgument)->rightSibling;
    while (SKIP_T(Token::OP_COMMA))
    {
        *pNextArgument                          = ParseAssignExpr(ti);
        pNextArgument                           = &(*pNextArgument)->rightSibling;
    }

    return argumentList;
}

Ast * ParseConstantExpr(TokenIterator & ti)
{
    // TODO
    Ast * constExpr;

    ASSERT(PEEK_T(Token::CONST_INT));
    constExpr = ParsePrimaryExpr(ti);
    constExpr->type = CONSTANT_EXPR;

    return constExpr;
}

Ast * ParseParameterList(TokenIterator & ti)
{
    Ast * parameterList                         = NewAst(PARAMETER_LIST);

    Ast * pNextParameterDeclaration             = NewAst(PARAMETER_DECLARATION);
    pNextParameterDeclaration->leftChild        = ParseDeclarationSpecifiers(ti);
    pNextParameterDeclaration->leftChild->rightSibling
                                                = ParseDeclarator(ti); // TODO: AbstractDeclarator

    parameterList->leftChild                    = pNextParameterDeclaration;

    while (SKIP_T(Token::OP_COMMA))
    {
        if (SKIP_T(Token::VAR_PARAM))
        {
            pNextParameterDeclaration->rightSibling     = NewAst(PARAMETER_VAR_LIST);
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


    return parameterList;
}

bool IsEqualAst(Ast * a1, Ast * a2)
{
    if (a1 && a2)
    {
        return
            (a1->type == a2->type) &&
            IsEqualAst(a1->leftChild, a2->leftChild) &&
            IsEqualAst(a1->rightSibling, a2->rightSibling);
    }
    else if (!a1 && !a2)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void DebugPrintAstImpl(Ast * ast, size_t indent)
{
    const char * astTypeString[] = {
                                    "translation unit",
                                    "external declaration",
                                    "function definition",
                                    "declaration",
                                    "declarator initializer",
                                    "declarator",
                                    "direct declarator",
                                    "abstract declarator",
                                    "direct abstract declarator",
                                    "parameter list",
                                    "var list",
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
                                    "comma expr",
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
                                    "case",
                                    "default",
                                    "compound stmt",
                                    "expression stmt",
                                    "selection stmt",
                                    "if else stmt",
                                    "switch stmt",
                                    "iteration stmt",
                                    "while stmt",
                                    "do while stmt",
                                    "for stmt",
                                    "jump stmt",
                                    "continue stmt",
                                    "break stmt",
                                    "return stmt",
                                 };
    if (ast)
    {
        for (size_t i = 0; i < indent; ++i)
            std::cout << ' ';
        ASSERT((unsigned)ast->type < sizeof(astTypeString) / sizeof(void *));
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
    DebugPrintAstImpl(ast, 0);
}
