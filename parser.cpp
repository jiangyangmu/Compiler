#include <iostream>
#include <stack>
#include <string>
#include <vector>
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
            EXPECT(LP);
            if (lex.peakNext().type != STMT_END)
            {
                stmt->expr = CommaExpression::parse(lex, env);
            }
            EXPECT(STMT_END);
            if (lex.peakNext().type != STMT_END)
            {
                stmt->expr2 = CommaExpression::parse(lex, env);
            }
            EXPECT(STMT_END);
            if (lex.peakNext().type != RP)
            {
                stmt->expr3 = CommaExpression::parse(lex, env);
            }
            EXPECT(RP);
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
            EXPECT(STMT_END);
            break;
        case BREAK:
            stmt->type = JMP_BREAK;
            EXPECT(STMT_END);
            break;
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
        SyntaxErrorDebug("Expect expression");

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
        expr->type_ = env->factory.newIntegral(TYPE_INT, true);
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
        expr->type_ = e->type();
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
        case CONST_CHAR:
            p = new PrimaryExpression();
            p->t = lex.getNext();
            p->cval = p->t.cval;
            p->type_ = env->factory.newIntegral(TYPE_CHAR, false);
            break;
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
            p->type_ = env->factory.newStringLiteral();
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
        case LP:
            p = new PrimaryExpression();
            p->t = lex.getNext();
            p->expr = CommaExpression::parse(lex, env);
            if (p->expr == nullptr)
                SyntaxError("PrimaryExpression: expect expression");
            EXPECT(RP);
            p->type_ = p->expr->type();
            break;
        default:
            break;
    }
    if (p == nullptr)
    {
        SyntaxErrorEx("Unsupported primary expression");
    }
    return p;
}

void SyntaxNode::emit(Environment *env, EEmitGoal goal) const
{
    Emit("<emit not implemented>");
}
// LabelStatement
void CompoundStatement::emit(Environment *__not_used, EEmitGoal goal) const
{
    env->emit();
    for (const SyntaxNode *stmt : stmts)
    {
        stmt->emit(env, FOR_NOTHING);
    }
}
void ExpressionStatement::emit(Environment *env, EEmitGoal goal) const
{
    expr->emit(env, FOR_NOTHING);
}
void SelectionStatement::emit(Environment *env, EEmitGoal goal) const
{
    if (!stmt && !stmt2)
    {
        expr->emit(env, FOR_NOTHING);
        return;
    }
    else
    {
        expr->emit(env, FOR_VALUE);
        Emit("cmpq $0, %%rax");
    }

    StringRef lend = CreateLabel("Lend");
    if (stmt == nullptr)
    {
        // assert( stmt2 != nullptr)
        Emit("jne %.*s", lend.size(), lend.data());
        stmt2->emit(env, FOR_NOTHING);
    }
    else if (stmt2 == nullptr)
    {
        Emit("je %.*s", lend.size(), lend.data());
        stmt->emit(env, FOR_NOTHING);
    }
    else
    {
        StringRef lc = CreateLabel("Lc");
        Emit("je %.*s", lc.size(), lc.data());
        stmt->emit(env, FOR_NOTHING);
        Emit("jmp %.*s", lend.size(), lend.data());
        Emit("%.*s:", lc.size(), lc.data());
        stmt2->emit(env, FOR_NOTHING);
    }
    Emit("%.*s:", lend.size(), lend.data());
}
void IterationStatement::emit(Environment *env, EEmitGoal goal) const
{
    StringRef head = CreateLabel("Lhead");
    StringRef start = CreateLabel("Lstart");
    StringRef end = CreateLabel("Lend");
    env->pushLabel(start, end);
    if (type == FOR_LOOP)
    {
        if (expr)
            expr->emit(env, FOR_NOTHING);
        Emit("%.*s:", head.size(), head.data());
        if (expr2)
        {
            expr2->emit(env, FOR_VALUE);
            Emit("cmpq $0, %%rax");
            Emit("je %.*s", end.size(), end.data());
        }
        stmt->emit(env, FOR_NOTHING);
        Emit("%.*s:", start.size(), start.data());
        if (expr3)
            expr3->emit(env, FOR_NOTHING);
        Emit("jmp %.*s", head.size(), head.data());
        Emit("%.*s:", end.size(), end.data());
    }
}
void JumpStatement::emit(Environment *env, EEmitGoal goal) const
{
    if (type == JMP_RETURN)
    {
        if (expr)
            expr->emit(env, FOR_VALUE);
        Emit("leave");
        Emit("ret");
    }
    else if (type == JMP_BREAK)
    {
        Emit("jmp %.*s", env->endLabel().size(), env->endLabel().data());
    }
    else if (type == JMP_CONTINUE)
    {
        Emit("jmp %.*s", env->startLabel().size(), env->startLabel().data());
    }
    else
    {
        SyntaxError("JumpStatement: not implemented");
    }
}
void CommaExpression::emit(Environment *env, EEmitGoal goal) const
{
    if (next)
    {
        curr->emit(env, FOR_NOTHING);
        next->emit(env, FOR_VALUE);
    }
    else
        curr->emit(env, FOR_VALUE);

    if (goal == FOR_ADDRESS)
        SyntaxError("Can't get address of a rvalue");
}
void AssignExpression::emit(Environment *env, EEmitGoal goal) const
{
    // TODO: evaluate should be from left to right
    source->emit(env, FOR_VALUE);
    Emit("movq %%rax, %%rcx");
    target->emit(env, FOR_ADDRESS);
    Emit("movq %%rcx, (%%rax)");

    if (goal == FOR_VALUE)
        Emit("movq %%rcx, %%rax");
}
void CondExpression::emit(Environment *env, EEmitGoal goal) const
{
    StringRef l1 = CreateLabel("L1");
    StringRef lend = CreateLabel("Lend");
    cond->emit(env, FOR_VALUE);
    Emit("cmpq $0, %%rax");
    Emit("jz %.*s", l1.size(), l1.data());
    left->emit(env, FOR_VALUE);
    Emit("jmp %.*s", lend.size(), lend.data());
    Emit("%.*s:", l1.size(), l1.data());
    right->emit(env, FOR_VALUE);
    Emit("%.*s:", lend.size(), lend.data());

    if (goal == FOR_ADDRESS)
        SyntaxError("Can't get address of a rvalue");
}
void OrExpression::emit(Environment *env, EEmitGoal goal) const
{
    left->emit(env, FOR_VALUE);
    Emit("pushq %%rax");
    right->emit(env, FOR_VALUE);
    Emit("movq %%rax, %%rcx");
    Emit("popq %%rax");

    StringRef lfalse = CreateLabel("Lfalse");

    Emit("orq %%rcx, %%rax");
    Emit("cmpq $0, %%rax");
    Emit("je %.*s", lfalse.size(), lfalse.data());

    Emit("movq $1, %%rax");
    Emit("%.*s:", lfalse.size(), lfalse.data());

    if (goal == FOR_ADDRESS)
        SyntaxError("Can't get address of a rvalue");
}
void AndExpression::emit(Environment *env, EEmitGoal goal) const
{
    left->emit(env, FOR_VALUE);
    Emit("pushq %%rax");
    right->emit(env, FOR_VALUE);
    Emit("movq %%rax, %%rcx");
    Emit("popq %%rax");

    StringRef lfalse = CreateLabel("Lfalse");

    Emit("cmpq $0, %%rax");
    Emit("je %.*s", lfalse.size(), lfalse.data());
    Emit("cmpq $0, %%rcx");
    Emit("movq $0, %%rax");
    Emit("je %.*s", lfalse.size(), lfalse.data());

    Emit("movq $1, %%rax");
    Emit("%.*s:", lfalse.size(), lfalse.data());

    if (goal == FOR_ADDRESS)
        SyntaxError("Can't get address of a rvalue");
}
void BitOrExpression::emit(Environment *env, EEmitGoal goal) const
{
    left->emit(env, FOR_VALUE);
    Emit("pushq %%rax");
    right->emit(env, FOR_VALUE);
    Emit("movq %%rax, %%rcx");
    Emit("popq %%rax");

    Emit("orq %%rcx, %%rax");

    if (goal == FOR_ADDRESS)
        SyntaxError("Can't get address of a rvalue");
}
void BitXorExpression::emit(Environment *env, EEmitGoal goal) const
{
    left->emit(env, FOR_VALUE);
    Emit("pushq %%rax");
    right->emit(env, FOR_VALUE);
    Emit("movq %%rax, %%rcx");
    Emit("popq %%rax");

    Emit("xorq %%rcx, %%rax");

    if (goal == FOR_ADDRESS)
        SyntaxError("Can't get address of a rvalue");
}
void BitAndExpression::emit(Environment *env, EEmitGoal goal) const
{
    left->emit(env, FOR_VALUE);
    Emit("pushq %%rax");
    right->emit(env, FOR_VALUE);
    Emit("movq %%rax, %%rcx");
    Emit("popq %%rax");

    Emit("andq %%rcx, %%rax");

    if (goal == FOR_ADDRESS)
        SyntaxError("Can't get address of a rvalue");
}
// EqExpression
void EqExpression::emit(Environment *env, EEmitGoal goal) const
{
    left->emit(env, FOR_VALUE);
    Emit("pushq %%rax");
    right->emit(env, FOR_VALUE);
    Emit("movq %%rax, %%rcx");
    Emit("popq %%rax");
    Emit("cmp %%rcx, %%rax");

    StringRef lfalse = CreateLabel("Lfalse");
    StringRef lend = CreateLabel("Lend");
    switch (op)
    {
        case REL_EQ: Emit("jne %.*s", lfalse.size(), lfalse.data()); break;
        case REL_NE: Emit("je %.*s", lfalse.size(), lfalse.data()); break;
        default: break;
    }

    Emit("movq $1, %%rax");
    Emit("jmp %.*s", lend.size(), lend.data());
    Emit("%.*s:", lfalse.size(), lfalse.data());
    Emit("movq $0, %%rax");
    Emit("%.*s:", lend.size(), lend.data());

    if (goal == FOR_ADDRESS)
        SyntaxError("Can't get address of a rvalue");
}
void RelExpression::emit(Environment *env, EEmitGoal goal) const
{
    left->emit(env, FOR_VALUE);
    Emit("pushq %%rax");
    right->emit(env, FOR_VALUE);
    Emit("movq %%rax, %%rcx");
    Emit("popq %%rax");
    Emit("cmp %%rcx, %%rax");

    StringRef lfalse = CreateLabel("Lfalse");
    StringRef lend = CreateLabel("Lend");
    // TODO: signed or unsigned?
    switch (op)
    {
        case REL_LT: Emit("jge %.*s", lfalse.size(), lfalse.data()); break;
        case REL_LE: Emit("jg %.*s", lfalse.size(), lfalse.data()); break;
        case REL_GT: Emit("jle %.*s", lfalse.size(), lfalse.data()); break;
        case REL_GE: Emit("jl %.*s", lfalse.size(), lfalse.data()); break;
        default: break;
    }

    Emit("movq $1, %%rax");
    Emit("jmp %.*s", lend.size(), lend.data());
    Emit("%.*s:", lfalse.size(), lfalse.data());
    Emit("movq $0, %%rax");
    Emit("%.*s:", lend.size(), lend.data());

    if (goal == FOR_ADDRESS)
        SyntaxError("Can't get address of a rvalue");
}
void ShiftExpression::emit(Environment *env, EEmitGoal goal) const
{
    left->emit(env, FOR_VALUE);
    Emit("pushq %%rax");
    right->emit(env, FOR_VALUE);
    Emit("movq %%rax, %%rcx");
    Emit("popq %%rax");

    if (op == BIT_SLEFT)
        Emit("rol %%cl, %%rax");
    else
        Emit("ror %%cl, %%rax");

    if (goal == FOR_ADDRESS)
        SyntaxError("Can't get address of a rvalue");
}
void AddExpression::emit(Environment *env, EEmitGoal goal) const
{
    left->emit(env, FOR_VALUE);
    Emit("pushq %%rax");
    right->emit(env, FOR_VALUE);
    Emit("movq %%rax, %%rcx");
    Emit("popq %%rax");
    if (op == OP_ADD)
        Emit("addq %%rcx, %%rax");
    else
        Emit("subq %%rcx, %%rax");

    if (goal == FOR_ADDRESS)
        SyntaxError("Can't get address of a rvalue");
}
// MulExpression
// CastExpression
// UnaryExpression
void PostfixExpression::emit(Environment *env, EEmitGoal goal) const
{
    PrimaryExpression *e = dynamic_cast<PrimaryExpression *>(target);
    vector<StringRef> regs{StringRef("%rdi"), StringRef("%rsi"), StringRef("%rdx"),
                           StringRef("%rcx"), StringRef("%r8"),  StringRef("%r9")};
    int idx = 0;
    switch (op)
    {
        case POSTFIX_CALL:
            for (auto it = params.begin(); it != params.end(); ++it)
            {
                (*it)->emit(env, FOR_VALUE);
                Emit("movq %%rax, %s", regs[idx++]);
            }
            Emit("call _%s", e->symbol->name.toString().c_str());
            break;
        default:
            SyntaxError("PostfixExpression: not implemented");
            break;
    }
}
void PrimaryExpression::emit(Environment *env, EEmitGoal goal) const
{
    vector<StringRef> regs{StringRef("%rdi"), StringRef("%rsi"), StringRef("%rdx"),
                           StringRef("-8(%rbp)"), StringRef("%r8"),  StringRef("%r9")};
    StringRef l;
    switch (t.type)
    {
        case CONST_CHAR:
            if (goal == FOR_VALUE)
                Emit("movq $%d, %%rax", cval);
            else if (goal == FOR_ADDRESS)
                SyntaxError("Can't get address of a rvalue");
            break;
        case CONST_INT:
            if (goal == FOR_VALUE)
                Emit("movq $%d, %%rax", ival);
            else if (goal == FOR_ADDRESS)
                SyntaxError("Can't get address of a rvalue");
            break;
        case STRING:
            l = CreateLabel("L.str");
            EmitData("%s: .asciz \"%s\"", l.data(), str->toString().data());
            if (goal == FOR_VALUE)
                Emit("leaq %s(%%rip), %%rax", l.data());
            else if (goal == FOR_ADDRESS)
                SyntaxError("Can't get address of a rvalue");
            break;
        case SYMBOL:
            if (goal == FOR_VALUE)
            {
                if (symbol->position > 0)
                    Emit("movq %s, %%rax", regs[symbol->position - 1]);
                else
                    Emit("movq _%s(%%rip), %%rax", symbol->name.toString().data());
            }
            else if (goal == FOR_ADDRESS)
            {
                if (symbol->position > 0)
                    SyntaxError("Can't get address of a register value");
                else
                    Emit("leaq _%s(%%rip), %%rax", symbol->name.toString().data());
            }
            break;
        case LP:
            expr->emit(env, goal);
            break;
        default: Emit("<unknown PrimaryExpression>"); break;
    }
}
