#include <iostream>
#include <stack>
#include <string>
using namespace std;

#include "common.h"
#include "env.h"
#include "lexer.h"
#include "parser.h"
#include "convert.h"

bool IsDeclaration(TokenType t)
{
    bool decl = false;
    switch (t)
    {
        case TYPEDEF:
        case EXTERN:
        case STATIC:
        case AUTO:
        case REGISTER:
        case TYPE_VOID:
        case TYPE_CHAR:
        case TYPE_SHORT:
        case TYPE_INT:
        case TYPE_LONG:
        case TYPE_FLOAT:
        case TYPE_DOUBLE:
        case SIGNED:
        case UNSIGNED:
        case TYPE_STRUCT:
        case TYPE_ENUM:
            // TODO: other typedef names
            decl = true;
            break;
        default: break;
    }
    return decl;
}
bool IsAssignOperator(TokenType t)
{
    bool is = false;
    switch (t)
    {
        case ASSIGN:
        case ASSIGN_ADD:
        case ASSIGN_SUB:
        case ASSIGN_MUL:
        case ASSIGN_DIV:
        case ASSIGN_MOD:
        case ASSIGN_SLEFT:
        case ASSIGN_SRIGHT:
        case ASSIGN_AND:
        case ASSIGN_OR:
        case ASSIGN_XOR: is = true; break;
        default: break;
    }
    return is;
}
bool IsUnaryOperator(TokenType t)
{
    bool is = false;
    switch (t)
    {
        case BIT_AND:
        case BIT_NOT:
        case OP_MUL:
        case OP_ADD:
        case OP_SUB:
        case BOOL_NOT: is = true; break;
        default: break;
    }
    return is;
}
bool IsTypeName(TokenType t)
{
    bool is = false;
    switch (t)
    {
        // type qualifiers
        case CONST:
        case VOLATILE:
        // type specifiers
        case TYPE_VOID:
        case TYPE_CHAR:
        case TYPE_SHORT:
        case TYPE_INT:
        case TYPE_LONG:
        case TYPE_FLOAT:
        case TYPE_DOUBLE:
        case UNSIGNED:
        case SIGNED:
        case TYPE_STRUCT:
        case TYPE_ENUM: is = true; break;
        default: break;
    }
    return is;
}
bool IsExpression(TokenType t)
{
    bool is = false;
    // FIRST(expr) = ++ -- unary-op sizeof id constant string (
    switch (t)
    {
        case OP_INC:
        case OP_DEC:
        case BIT_AND:
        case BIT_NOT:
        case BOOL_NOT:
        case OP_MUL:
        case OP_ADD:
        case OP_SUB:
        case SIZEOF:
        case SYMBOL:
        case CONST_INT:
        case CONST_CHAR:
        case CONST_FLOAT:
        case STRING:
        case LP: is = true; break;
        default: break;
    }
    return is;
}

class DebugIndentHelper
{
   public:
    static string _indent;
    DebugIndentHelper()
    {
        _indent += "  ";
    }
    ~DebugIndentHelper()
    {
        _indent.pop_back();
        _indent.pop_back();
    }
};
string DebugIndentHelper::_indent;
// #define DebugParseTree(name)                             \
//     cout << DebugIndentHelper::_indent << #name << endl; \
//     DebugIndentHelper __dh
#define DebugParseTree(name)
// cout << __dh.get() << #name " at Token " << lex.peakNext()
//      << ", line " << lex.peakNext().line << endl;

SyntaxNode *TypeName::parse(Lexer &lex, Environment *env)
{
    return nullptr;
}
// no instance, only dispatch
SyntaxNode *Statement::parse(Lexer &lex, Environment *env)
{
    // DebugParseTree(Statement);
    SyntaxNode *node = nullptr;
    switch (lex.peakNext().type)
    {
        case SYMBOL:
            if (lex.peakNext(1).type == OP_COLON)
            {
                node = LabelStatement::parse(lex, env);
            }
            else
            {
                node = ExpressionStatement::parse(lex, env);
            }
            break;
        case CASE:
        case DEFAULT: node = LabelStatement::parse(lex, env); break;
        case BLK_BEGIN: node = CompoundStatement::parse(lex, env); break;
        case IF:
        case SWITCH: node = SelectionStatement::parse(lex, env); break;
        case WHILE:
        case DO:
        case FOR: node = IterationStatement::parse(lex, env); break;
        case GOTO:
        case CONTINUE:
        case BREAK:
        case RETURN: node = JumpStatement::parse(lex, env); break;
        default:
            /* TODO: check FIRST(expr) */
            node = ExpressionStatement::parse(lex, env);
            break;
    }
    return node;
}
// TODO: implement this
SyntaxNode *LabelStatement::parse(Lexer &lex, Environment *env)
{
    DebugParseTree(LabelStatement);
    SyntaxError("Unsupported feature: Label Statement");
    return nullptr;
}
SyntaxNode *CompoundStatement::parse(Lexer &lex, Environment *env,
                                     bool reuse_env)
{
    DebugParseTree(CompoundStatement);
    CompoundStatement *node = new CompoundStatement();

    if (lex.getNext().type != BLK_BEGIN)
    {
        SyntaxError("Expecting '{'");
    }

    if (reuse_env)
        node->env = env;
    else
    {
        node->env = new Environment();
        node->env->setParent(env);
    }

    // add declarations in current scope
    while (IsDeclaration(lex.peakNext().type))
        Environment::ParseLocalDeclaration(lex, node->env);

    // parse statements
    while (lex.peakNext().type != BLK_END)
    {
        node->stmts.push_back(Statement::parse(lex, node->env));
    }
    if (lex.getNext().type != BLK_END)
    {
        SyntaxError("Expecting '}'");
    }

    return node;
}
SyntaxNode *ExpressionStatement::parse(Lexer &lex, Environment *env)
{
    DebugParseTree(ExpressionStatement);
    ExpressionStatement *stmt = nullptr;
    if (lex.peakNext().type != STMT_END)
    {
        stmt = new ExpressionStatement();
        stmt->expr = CommaExpression::parse(lex, env);
    }
    EXPECT(STMT_END);

    return stmt;
}
SyntaxNode *SelectionStatement::parse(Lexer &lex, Environment *env)
{
    DebugParseTree(SelectionStatement);
    SelectionStatement *stmt = new SelectionStatement();
    stmt->stmt2 = nullptr;
    switch (lex.getNext().type)
    {
        case IF:
            if (lex.getNext().type != LP)
            {
                SyntaxError("Expect '('");
            }
            stmt->expr = CommaExpression::parse(lex, env);
            if (lex.getNext().type != RP)
            {
                SyntaxError("Expect ')'");
            }
            stmt->stmt = Statement::parse(lex, env);
            if (lex.peakNext().type == ELSE)
            {
                lex.getNext();
                stmt->stmt2 = Statement::parse(lex, env);
            }
            break;
        case SWITCH:
            if (lex.getNext().type != LP)
            {
                SyntaxError("Expect '('");
            }
            stmt->expr = CommaExpression::parse(lex, env);
            if (lex.getNext().type != RP)
            {
                SyntaxError("Expect ')'");
            }
            // TODO: switch constraits
            stmt->stmt = Statement::parse(lex, env);
            break;
        default: SyntaxError("Unexpected token"); break;
    }
    return stmt;
}
SyntaxNode *IterationStatement::parse(Lexer &lex, Environment *env)
{
    DebugParseTree(IterationStatement);
    IterationStatement *stmt = new IterationStatement();
    switch (lex.getNext().type)
    {
        case WHILE:
            stmt->type = WHILE_LOOP;
            if (lex.getNext().type != LP)
            {
                SyntaxError("Expect '('");
            }
            stmt->expr = CommaExpression::parse(lex, env);
            if (lex.getNext().type != RP)
            {
                SyntaxError("Expect ')'");
            }
            stmt->stmt = Statement::parse(lex, env);
            break;
        case DO:
            stmt->type = DO_LOOP;
            stmt->stmt = Statement::parse(lex, env);
            if (lex.getNext().type != WHILE)
            {
                SyntaxError("Expect 'while'");
            }
            if (lex.getNext().type != LP)
            {
                SyntaxError("Expect '('");
            }
            stmt->expr = CommaExpression::parse(lex, env);
            if (lex.getNext().type != RP)
            {
                SyntaxError("Expect ')'");
            }
            break;
        case FOR:
            stmt->type = FOR_LOOP;
            if (lex.getNext().type != LP)
            {
                SyntaxError("Expect '('");
            }
            stmt->expr = CommaExpression::parse(lex, env);
            EXPECT(STMT_END);
            stmt->expr2 = CommaExpression::parse(lex, env);
            EXPECT(STMT_END);
            if (lex.peakNext().type != RP)
            {
                stmt->expr3 = CommaExpression::parse(lex, env);
            }
            if (lex.getNext().type != RP)
            {
                SyntaxError("Expect ')'");
            }
            stmt->stmt = Statement::parse(lex, env);
            break;
        default: SyntaxError("Unexpected token"); break;
    }
    return stmt;
}
SyntaxNode *JumpStatement::parse(Lexer &lex, Environment *env)
{
    DebugParseTree(JumpStatement);
    JumpStatement *stmt = new JumpStatement();
    stmt->expr = nullptr;
    stmt->id = 0;
    switch (lex.getNext().type)
    {
        case GOTO:
            stmt->type = JMP_GOTO;
            SyntaxError("Unsupported feature");
            break;
        case CONTINUE:
            stmt->type = JMP_CONTINUE;
            if (lex.getNext().type != STMT_END)
            {
                SyntaxError("Expect ';'");
            }
            break;
        case BREAK: stmt->type = JMP_BREAK; break;
        case RETURN:
            stmt->type = JMP_RETURN;
            stmt->expr = CommaExpression::parse(lex, env);
            EXPECT(STMT_END);
            break;
        default: SyntaxError("Unexpected token"); break;
    }
    return stmt;
}

Expression *CommaExpression::parse(Lexer &lex, Environment *env)
{
    DebugParseTree(CommaExpression);
    if (!IsExpression(lex.peakNext().type))
        SyntaxError("Expect expression");

    Expression *e = AssignExpression::parse(lex, env);
    if (lex.peakNext().type == OP_COMMA)
    {
        lex.getNext();
        CommaExpression *expr = new CommaExpression();
        expr->curr = e;
        expr->next = CommaExpression::parse(lex, env);
        if (expr->next)
            expr->type_ = expr->next->type();
        else
            expr->type_ = e->type();
        return expr;
    }
    else
        return e;
}
// TODO: solve ambiguity, evaluate order
Expression *AssignExpression::parse(Lexer &lex, Environment *env)
{
    DebugParseTree(AssignExpression);
    AssignExpression *expr = nullptr;

    // condition or unary?
    // TODO: fix it, unary is not condition-expr
    // XXX: evaluation order?
    Expression *target = CondExpression::parse(lex, env);
    if (!IsAssignOperator(lex.peakNext().type))
        return target;

    expr = new AssignExpression();
    expr->op = lex.getNext().type;
    expr->target = target;
    expr->source = AssignExpression::parse(lex, env);

    EXPECT_TYPE_WITH(target->type(), TOp_ASSIGN);
    expr->type_ = target->type();

    return expr;
}
Expression *CondExpression::parse(Lexer &lex, Environment *env)
{
    DebugParseTree(CondExpression);
    Expression *e = OrExpression::parse(lex, env);
    if (lex.peakNext().type == OP_QMARK)
    {
        CondExpression *expr = new CondExpression();
        expr->cond = e;
        lex.getNext();
        expr->left = CommaExpression::parse(lex, env);
        EXPECT(OP_COLON);
        expr->right = CondExpression::parse(lex, env);
        return expr;
    }
    else
    {
        return e;
    }
}
Expression *OrExpression::parse(Lexer &lex, Environment *env)
{
    DebugParseTree(OrExpression);
    Expression *e = AndExpression::parse(lex, env);
    if (lex.peakNext().type == BOOL_OR)
    {
        lex.getNext();
        OrExpression *expr = new OrExpression();
        expr->left = e;
        expr->right = OrExpression::parse(lex, env);
        return expr;
    }
    else
        return e;
}
Expression *AndExpression::parse(Lexer &lex, Environment *env)
{
    DebugParseTree(AndExpression);
    Expression *e = BitOrExpression::parse(lex, env);
    if (lex.peakNext().type == BOOL_AND)
    {
        lex.getNext();
        AndExpression *expr = new AndExpression();
        expr->left = e;
        expr->right = AndExpression::parse(lex, env);
        return expr;
    }
    else
        return e;
}
Expression *BitOrExpression::parse(Lexer &lex, Environment *env)
{
    Expression *e = BitXorExpression::parse(lex, env);
    if (lex.peakNext().type == BIT_OR)
    {
        lex.getNext();
        BitOrExpression *expr = new BitOrExpression();
        expr->left = e;
        expr->right = BitOrExpression::parse(lex, env);
        return expr;
    }
    else
        return e;
}
Expression *BitXorExpression::parse(Lexer &lex, Environment *env)
{
    Expression *e = BitAndExpression::parse(lex, env);
    if (lex.peakNext().type == BIT_XOR)
    {
        lex.getNext();
        BitXorExpression *expr = new BitXorExpression();
        expr->left = e;
        expr->right = BitXorExpression::parse(lex, env);
        return expr;
    }
    else
        return e;
}
Expression *BitAndExpression::parse(Lexer &lex, Environment *env)
{
    Expression *e = EqExpression::parse(lex, env);
    if (lex.peakNext().type == BIT_AND)
    {
        lex.getNext();
        BitAndExpression *expr = new BitAndExpression();
        expr->left = e;
        expr->right = BitAndExpression::parse(lex, env);
        return expr;
    }
    else
        return e;
}
Expression *EqExpression::parse(Lexer &lex, Environment *env)
{
    Expression *e = RelExpression::parse(lex, env);
    if (lex.peakNext().type == REL_EQ || lex.peakNext().type == REL_NE)
    {
        EqExpression *expr = new EqExpression();
        expr->op = lex.getNext().type;
        expr->left = e;
        expr->right = EqExpression::parse(lex, env);
        return expr;
    }
    else
        return e;
}
Expression *RelExpression::parse(Lexer &lex, Environment *env)
{
    Expression *e = ShiftExpression::parse(lex, env);
    if (lex.peakNext().type == REL_LT || lex.peakNext().type == REL_LE ||
        lex.peakNext().type == REL_GT || lex.peakNext().type == REL_GE)
    {
        RelExpression *expr = new RelExpression();
        expr->op = lex.getNext().type;
        expr->left = e;
        expr->right = RelExpression::parse(lex, env);
        return expr;
    }
    else
        return e;
}
Expression *ShiftExpression::parse(Lexer &lex, Environment *env)
{
    Expression *e = AddExpression::parse(lex, env);
    if (lex.peakNext().type == BIT_SLEFT || lex.peakNext().type == BIT_SRIGHT)
    {
        ShiftExpression *expr = new ShiftExpression();
        expr->op = lex.getNext().type;
        expr->left = e;
        expr->right = ShiftExpression::parse(lex, env);
        return expr;
    }
    else
        return e;
}
Expression *AddExpression::parse(Lexer &lex, Environment *env, Expression *left)
{
    DebugParseTree(AddExpression);
    if (left == nullptr)
        left = MulExpression::parse(lex, env);
    if (lex.peakNext().type == OP_ADD || lex.peakNext().type == OP_SUB)
    {
        AddExpression *expr = new AddExpression();
        expr->op = lex.getNext().type;
        expr->left = left;
        expr->right = MulExpression::parse(lex, env);

        EXPECT_TYPE_WITH(left->type(), TOp_ADD);
        EXPECT_TYPE_WITH(expr->right->type(), TOp_ADD);

        expr->type_ = CommonType(expr->left->type(), expr->right->type());

        return parse(lex, env, expr);
    }
    else
        return left;
}
Expression *MulExpression::parse(Lexer &lex, Environment *env)
{
    DebugParseTree(MulExpression);
    Expression *e = CastExpression::parse(lex, env);
    if (lex.peakNext().type == OP_MUL || lex.peakNext().type == OP_DIV ||
        lex.peakNext().type == OP_MOD)
    {
        MulExpression *expr = new MulExpression();
        expr->op = lex.getNext().type;
        expr->left = e;
        expr->right = MulExpression::parse(lex, env);

        EXPECT_TYPE_WITH(expr->left->type(), TOp_MUL);
        EXPECT_TYPE_WITH(expr->right->type(), TOp_MUL);

        expr->type_ = CommonType(expr->left->type(), expr->right->type());

        return expr;
    }
    else
        return e;
}
// TODO: implement this
Expression *CastExpression::parse(Lexer &lex, Environment *env)
{
    // cast or unary
    if (lex.peakNext().type == LP && IsTypeName(lex.peakNext(1).type))
    {
        CastExpression *expr = new CastExpression();
        while (lex.peakNext().type == LP && IsTypeName(lex.peakNext(1).type))
        {
            lex.getNext();
            TypeBase *type = nullptr;  // ParseTypename(lex, env);
            expr->types.push_back(type);
            if (lex.getNext().type != RP)
            {
                SyntaxError("Expect ')'");
            }
        }
        expr->target = UnaryExpression::parse(lex, env);
        return expr;
    }
    else
    {
        return UnaryExpression::parse(lex, env);
    }
}
Expression *UnaryExpression::parse(Lexer &lex, Environment *env)
{
    DebugParseTree(UnaryExpression);
    if (IsUnaryOperator(lex.peakNext().type))
    {
        UnaryExpression *expr = new UnaryExpression();
        switch (lex.peakNext().type)
        {
            case OP_INC:
            case OP_DEC:
                expr->op = lex.getNext().type;
                expr->expr = UnaryExpression::parse(lex, env);
                break;
            case SIZEOF:
                expr->op = lex.getNext().type;
                if (lex.peakNext().type == LP)
                {
                    // expr->expr = new SyntaxNode(type = ParseTypename(lex,
                    // env));
                    if (lex.getNext().type != RP)
                    {
                        SyntaxError("Expecting ')'");
                    }
                }
                else
                    expr->expr = UnaryExpression::parse(lex, env);
                break;
            default:
                expr->op = lex.getNext().type;
                expr->expr = CastExpression::parse(lex, env);
                break;
        }
        return expr;
    }
    else
    {
        return PostfixExpression::parse(lex, env,
                                        PrimaryExpression::parse(lex, env));
    }
}
/*
 * primary-expr
 * postfix-expr '[' expr ']'
 * postfix-expr '(' [argument-expr-list] ')'
 * postfix-expr '.' identifier
 * postfix-expr '->' identifier
 * postfix-expr '++'
 * postfix-expr '--'
 */
bool __isPostfixOperator(TokenType t)
{
    bool is = false;
    switch (t)
    {
        case LSB:
        case LP:
        case REFER_TO:
        case POINT_TO:
        case OP_INC:
        case OP_DEC: is = true; break;
        default: break;
    }
    return is;
}
Expression *PostfixExpression::parse(Lexer &lex, Environment *env,
                                     Expression *target)
{
    DebugParseTree(PostfixExpression);
    if (__isPostfixOperator(lex.peakNext().type))
    {
        PostfixExpression *expr = new PostfixExpression();
        expr->target = target;
        // TODO: optional PrimaryExpression
        assert(target != nullptr);
        // assert(target->type() != nullptr);

        switch (lex.peakNext().type)
        {
            case LSB:
                EXPECT(LSB);
                EXPECT_TYPE_WITH(expr->target->type(), TOp_INDEX);
                expr->op = POSTFIX_INDEX;
                expr->index = CommaExpression::parse(lex, env);
                if (expr->index == nullptr)
                    SyntaxError("Expect expression.");
                EXPECT_TYPE_IS(expr->index->type(), TC_INT);
                if (dynamic_cast<Indexable *>(expr->target->type()))
                    expr->type_ =
                        dynamic_cast<Indexable *>(expr->target->type())
                            ->indexedType();
                EXPECT(RSB);
                break;
            case LP:
                EXPECT(LP);
                EXPECT_TYPE_IS(expr->target->type(), TC_FUNC);
                // EXPECT_TYPE_WITH(expr->target->type(), TOp_CALL);
                expr->op = POSTFIX_CALL;
                if (lex.peakNext().type != RP)
                {
                    while (true)
                    {
                        expr->params.push_back(
                            AssignExpression::parse(lex, env));
                        if (lex.peakNext().type == OP_COMMA)
                            lex.getNext();
                        else
                            break;
                    }
                }
                expr->type_ =
                    dynamic_cast<FuncType *>(expr->target->type())->rtype();
                EXPECT(RP);
                break;
            case REFER_TO:
                lex.getNext();
                EXPECT_TYPE_WITH(expr->target->type(), TOp_OFFSET);
                expr->op = POSTFIX_OBJECT_OFFSET;
                expr->member = EXPECT_GET(SYMBOL).symbol;
                break;
            case POINT_TO:
                lex.getNext();
                EXPECT_TYPE_IS(expr->target->type(), TC_POINTER);
                EXPECT_TYPE_WITH(
                    dynamic_cast<PointerType *>(expr->target->type())->target(),
                    TOp_OFFSET);
                expr->op = POSTFIX_POINTER_OFFSET;
                expr->member = EXPECT_GET(SYMBOL).symbol;
                expr->type_ = dynamic_cast<StructType *>(expr->target->type())
                                  ->getMember(expr->member)
                                  ->type;
                break;
            case OP_INC:
                lex.getNext();
                expr->op = POSTFIX_INC;
                EXPECT_TYPE_WITH(expr->target->type(), TOp_INC);
                expr->type_ = expr->target->type();
                break;
            case OP_DEC:
                lex.getNext();
                expr->op = POSTFIX_DEC;
                EXPECT_TYPE_WITH(expr->target->type(), TOp_DEC);
                expr->type_ = expr->target->type();
                break;
            default: SyntaxError("Should not reach here."); break;
        }

        Expression *next = PostfixExpression::parse(lex, env, expr);
        if (next == expr)
            return expr;
        else
        {
            dynamic_cast<PostfixExpression *>(next)->target = expr;
            return next;
        }
    }
    else
    {
        return target;
    }
}
bool __isPrimaryExpression(TokenType t)
{
    bool is = false;
    switch (t)
    {
        case SYMBOL:
        case CONST_INT:
        case CONST_FLOAT:
        case STRING:
        case LP: is = true; break;
        default: break;
    }
    return is;
}
Expression *PrimaryExpression::parse(Lexer &lex, Environment *env)
{
    DebugParseTree(PrimaryExpression);
    PrimaryExpression *p = nullptr;
    switch (lex.peakNext().type)
    {
        case CONST_INT:
            p = new PrimaryExpression();
            p->t = lex.getNext();
            p->ival = p->t.ival;
            p->type_ = env->factory.newIntegral(TYPE_INT, true);
            break;
        case STRING:
            p = new PrimaryExpression();
            p->t = lex.getNext();
            p->str = &p->t.string_;
            break;
        case SYMBOL:
            p = new PrimaryExpression();
            p->t = lex.getNext();
            p->symbol = env->find(SC_ID, p->t.symbol);
            if (p->symbol == nullptr)
                SyntaxError("Symbol '" + p->t.symbol.toString() +
                            "' not found");
            p->type_ = p->symbol->type;
            break;
        default: SyntaxErrorEx("Unsupported primary expression"); break;
    }
    return p;
}

