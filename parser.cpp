#include <iostream>
#include <stack>
#include <string>
using namespace std;

#include "common.h"
#include "env.h"
#include "lexer.h"
#include "parser.h"

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
        default:
            break;
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
        case ASSIGN_XOR:
            is = true;
            break;
        default:
            break;
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
        case BOOL_NOT:
            is = true;
            break;
        default:
            break;
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
        case TYPE_ENUM:
            is = true;
            break;
        default:
            break;
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
        case LP:
            is = true;
            break;
        default:
            break;
    }
    return is;
}

class DebugIndentHelper
{
   public:
    static string _indent;
    DebugIndentHelper() { _indent += "    "; }
    ~DebugIndentHelper()
    {
        _indent.pop_back();
        _indent.pop_back();
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

SyntaxNode *TypeName::parse(Lexer &lex, Environment *env) { return nullptr; }
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
        case DEFAULT:
            node = LabelStatement::parse(lex, env);
            break;
        case BLK_BEGIN:
            node = CompoundStatement::parse(lex, env);
            break;
        case IF:
        case SWITCH:
            node = SelectionStatement::parse(lex, env);
            break;
        case WHILE:
        case DO:
        case FOR:
            node = IterationStatement::parse(lex, env);
            break;
        case GOTO:
        case CONTINUE:
        case BREAK:
        case RETURN:
            node = JumpStatement::parse(lex, env);
            break;
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
SyntaxNode *CompoundStatement::parse(Lexer &lex, Environment *env, bool reuse_env)
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
    SyntaxNode *node = nullptr;
    if (lex.peakNext().type != STMT_END)
        node = Expression::parse(lex, env);
    if (lex.getNext().type != STMT_END) {
        SyntaxError("Expecting ';'");
        assert( false );
    }
    return node;
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
            stmt->expr = Expression::parse(lex, env);
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
            stmt->expr = Expression::parse(lex, env);
            if (lex.getNext().type != RP)
            {
                SyntaxError("Expect ')'");
            }
            // TODO: switch constraits
            stmt->stmt = Statement::parse(lex, env);
            break;
        default:
            SyntaxError("Unexpected token");
            break;
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
            stmt->expr = Expression::parse(lex, env);
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
            stmt->expr = Expression::parse(lex, env);
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
            stmt->expr = ExpressionStatement::parse(lex, env);
            stmt->expr2 = ExpressionStatement::parse(lex, env);
            if (lex.peakNext().type != RP)
            {
                stmt->expr3 = Expression::parse(lex, env);
            }
            if (lex.getNext().type != RP)
            {
                SyntaxError("Expect ')'");
            }
            stmt->stmt = Statement::parse(lex, env);
            break;
        default:
            SyntaxError("Unexpected token");
            break;
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
        case BREAK:
            stmt->type = JMP_BREAK;
            break;
        case RETURN:
            stmt->type = JMP_RETURN;
            stmt->expr = ExpressionStatement::parse(lex, env);
            break;
        default:
            SyntaxError("Unexpected token");
            break;
    }
    return stmt;
}

SyntaxNode *Expression::parse(Lexer &lex, Environment *env)
{
    DebugParseTree(Expression);
    Expression *expr = nullptr;
    SyntaxNode *e;

    while (true)
    {
        if (IsExpression(lex.peakNext().type))
        {
            e = AssignExpression::parse(lex, env);
        }
        if (expr)
        {
            expr->exprs.push_back(e);
            if (lex.peakNext().type != OP_COMMA)
                return expr;
        }
        else
        {
            if (lex.peakNext().type != OP_COMMA)
                return e;
            else
            {
                expr = new Expression();
                expr->exprs.push_back(e);
            }
        }
        lex.getNext();
    }
}
// TODO: solve ambiguity, evaluate order
SyntaxNode *AssignExpression::parse(Lexer &lex, Environment *env)
{
    DebugParseTree(AssignExpression);
    AssignExpression *expr = nullptr;

    // condition or unary?
    vector<SyntaxNode *> targets;
    SyntaxNode *e = nullptr;

    // TODO: fix it, unary is not condition-expr
    // XXX: evaluation order?
    while (true)
    {
        e = CondExpression::parse(lex, env);
        if (IsAssignOperator(lex.peakNext().type))
        {
            lex.getNext();
            targets.push_back(e);
        }
        else
            break;
    }

    if (targets.empty())
        return e;
    else
    {
        expr = new AssignExpression();
        expr->source = e;
        expr->targets = targets;
        return expr;
    }
}
SyntaxNode *CondExpression::parse(Lexer &lex, Environment *env)
{
    DebugParseTree(CondExpression);
    SyntaxNode *e = OrExpression::parse(lex, env);
    if (lex.peakNext().type == OP_QMARK)
    {
        CondExpression *expr = new CondExpression();
        expr->cond = e;
        lex.getNext();
        expr->left = Expression::parse(lex, env);
        if (lex.getNext().type != OP_COLON)
        {
            SyntaxError("Expect ':'");
        }
        expr->right = CondExpression::parse(lex, env);
        return expr;
    }
    else
    {
        return e;
    }
}
SyntaxNode *OrExpression::parse(Lexer &lex, Environment *env)
{
    DebugParseTree(OrExpression);
    SyntaxNode *e = AndExpression::parse(lex, env);
    if (lex.peakNext().type == BOOL_OR)
    {
        OrExpression *expr = new OrExpression();
        expr->exprs.push_back(e);
        while (lex.peakNext().type == BOOL_OR)
        {
            lex.getNext();
            expr->exprs.push_back(AndExpression::parse(lex, env));
        }
        return expr;
    }
    else
    {
        return e;
    }
}
SyntaxNode *AndExpression::parse(Lexer &lex, Environment *env)
{
    DebugParseTree(AndExpression);
    SyntaxNode *e = BitOrExpression::parse(lex, env);
    if (lex.peakNext().type == BOOL_AND)
    {
        AndExpression *expr = new AndExpression();
        expr->exprs.push_back(e);
        while (lex.peakNext().type == BOOL_AND)
        {
            lex.getNext();
            expr->exprs.push_back(BitOrExpression::parse(lex, env));
        }
        return expr;
    }
    else
    {
        return e;
    }
}
SyntaxNode *BitOrExpression::parse(Lexer &lex, Environment *env)
{
    SyntaxNode *e = BitXorExpression::parse(lex, env);
    if (lex.peakNext().type == BIT_OR)
    {
        BitOrExpression *expr = new BitOrExpression();
        expr->exprs.push_back(e);
        while (lex.peakNext().type == BIT_OR)
        {
            lex.getNext();
            expr->exprs.push_back(BitXorExpression::parse(lex, env));
        }
        return expr;
    }
    else
    {
        return e;
    }
}
SyntaxNode *BitXorExpression::parse(Lexer &lex, Environment *env)
{
    SyntaxNode *e = BitAndExpression::parse(lex, env);
    if (lex.peakNext().type == BIT_XOR)
    {
        BitXorExpression *expr = new BitXorExpression();
        expr->exprs.push_back(e);
        while (lex.peakNext().type == BIT_XOR)
        {
            lex.getNext();
            expr->exprs.push_back(BitAndExpression::parse(lex, env));
        }
        return expr;
    }
    else
    {
        return e;
    }
}
SyntaxNode *BitAndExpression::parse(Lexer &lex, Environment *env)
{
    SyntaxNode *e = EqExpression::parse(lex, env);
    if (lex.peakNext().type == BIT_AND)
    {
        BitAndExpression *expr = new BitAndExpression();
        expr->exprs.push_back(e);
        while (lex.peakNext().type == BIT_AND)
        {
            lex.getNext();
            expr->exprs.push_back(EqExpression::parse(lex, env));
        }
        return expr;
    }
    else
    {
        return e;
    }
}
SyntaxNode *EqExpression::parse(Lexer &lex, Environment *env)
{
    SyntaxNode *e = RelExpression::parse(lex, env);
    if (lex.peakNext().type == REL_EQ || lex.peakNext().type == REL_NE)
    {
        EqExpression *expr = new EqExpression();
        expr->exprs.push_back(e);
        while (lex.peakNext().type == REL_EQ || lex.peakNext().type == REL_NE)
        {
            expr->ops.push_back(lex.getNext().type);
            expr->exprs.push_back(RelExpression::parse(lex, env));
        }
        return expr;
    }
    else
    {
        return e;
    }
}
SyntaxNode *RelExpression::parse(Lexer &lex, Environment *env)
{
    SyntaxNode *e = ShiftExpression::parse(lex, env);
    if (lex.peakNext().type == REL_LT || lex.peakNext().type == REL_LE ||
        lex.peakNext().type == REL_GT || lex.peakNext().type == REL_GE)
    {
        RelExpression *expr = new RelExpression();
        expr->exprs.push_back(e);
        while (lex.peakNext().type == REL_LT || lex.peakNext().type == REL_LE ||
               lex.peakNext().type == REL_GT || lex.peakNext().type == REL_GE)
        {
            expr->ops.push_back(lex.getNext().type);
            expr->exprs.push_back(ShiftExpression::parse(lex, env));
        }
        return expr;
    }
    else
    {
        return e;
    }
}
SyntaxNode *ShiftExpression::parse(Lexer &lex, Environment *env)
{
    SyntaxNode *e = AddExpression::parse(lex, env);
    if (lex.peakNext().type == BIT_SLEFT || lex.peakNext().type == BIT_SRIGHT)
    {
        ShiftExpression *expr = new ShiftExpression();
        expr->exprs.push_back(e);
        while (lex.peakNext().type == BIT_SLEFT ||
               lex.peakNext().type == BIT_SRIGHT)
        {
            expr->ops.push_back(lex.getNext().type);
            expr->exprs.push_back(AddExpression::parse(lex, env));
        }
        return expr;
    }
    else
    {
        return e;
    }
}
SyntaxNode *AddExpression::parse(Lexer &lex, Environment *env)
{
    DebugParseTree(AddExpression);
    SyntaxNode *e = MulExpression::parse(lex, env);
    if (lex.peakNext().type == OP_ADD || lex.peakNext().type == OP_SUB)
    {
        AddExpression *expr = new AddExpression();
        expr->exprs.push_back(e);
        while (lex.peakNext().type == OP_ADD || lex.peakNext().type == OP_SUB)
        {
            expr->ops.push_back(lex.getNext().type);
            expr->exprs.push_back(MulExpression::parse(lex, env));
        }
        return expr;
    }
    else
    {
        return e;
    }
}
SyntaxNode *MulExpression::parse(Lexer &lex, Environment *env)
{
    DebugParseTree(MulExpression);
    SyntaxNode *e = CastExpression::parse(lex, env);
    if (lex.peakNext().type == OP_MUL || lex.peakNext().type == OP_DIV ||
        lex.peakNext().type == OP_MOD)
    {
        MulExpression *expr = new MulExpression();
        expr->exprs.push_back(e);
        while (lex.peakNext().type == OP_MUL || lex.peakNext().type == OP_DIV ||
               lex.peakNext().type == OP_MOD)
        {
            expr->ops.push_back(lex.getNext().type);
            expr->exprs.push_back(CastExpression::parse(lex, env));
        }
        return expr;
    }
    else
    {
        return e;
    }
}
// TODO: implement this
SyntaxNode *CastExpression::parse(Lexer &lex, Environment *env)
{
    // cast or unary
    if (lex.peakNext().type == LP && IsTypeName(lex.peakNext(1).type))
    {
        CastExpression *expr = new CastExpression();
        while (lex.peakNext().type == LP && IsTypeName(lex.peakNext(1).type))
        {
            lex.getNext();
            TypeBase *type = nullptr; // ParseTypename(lex, env);
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
SyntaxNode *UnaryExpression::parse(Lexer &lex, Environment *env)
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
                    // expr->expr = new SyntaxNode(type = ParseTypename(lex, env));
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
        return PostfixExpression::parse(lex, env);
    }
}
// TODO: implement this
SyntaxNode *PostfixExpression::parse(Lexer &lex, Environment *env)
{
    DebugParseTree(PostfixExpression);
    return PrimaryExpression::parse(lex, env);
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
        case LP:
            is = true;
            break;
        default:
            break;
    }
    return is;
}
SyntaxNode *PrimaryExpression::parse(Lexer &lex, Environment *env)
{
    DebugParseTree(PrimaryExpression);
    PrimaryExpression *p = nullptr;
    switch (lex.peakNext().type)
    {
        case CONST_INT:
            p = new PrimaryExpression();
            p->t = lex.getNext();
            p->ival = p->t.ival;
            break;
        case SYMBOL:
            p = new PrimaryExpression();
            p->t = lex.getNext();
            p->symbol = env->find(SC_ID, lex.symbolName(p->t.symid));
            if (p->symbol == nullptr)
                SyntaxError("Symbol '" + lex.symbolName(p->t.symid).toString() + "' not found");
            break;
        default:
            SyntaxError("Unsupported primary expression");
            break;
    }
    return p;
}
