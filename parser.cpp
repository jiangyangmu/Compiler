#include <iostream>
#include <stack>
using namespace std;

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

void CheckTokens(Lexer &lex)
{
    string s;
    while (cin >> s && lex.hasNext())
    {
        if (s == "n")
        {
            cout << "Token " << lex.peakNext()
                 << " at line " << lex.peakNext().line << endl;
            lex.getNext();
        }
        else
        {
            break;
        }
    }
}
#define SyntaxError(msg)                                       \
    do                                                         \
    {                                                          \
        cout << "Syntax: " << msg << " at " << __FILE__ << ':' \
             << to_string(__LINE__) << endl;                   \
        CheckTokens(lex);                                      \
        exit(1);                                               \
    } while (0)

#define DebugParseTree(name)                                     \
    do                                                           \
    {                                                            \
        cout << #name " at Token " << lex.peakNext() << ", line " \
             << lex.peakNext().line << endl;                     \
    } while (0)

TFunction * TFunction::tryParse(Lexer &lex)
{
    TFunction * f = nullptr;
    if (lex.peakNext().type == LP)
    {
        lex.getNext();
        f = new TFunction();
        while (lex.peakNext().type != RP)
        {
            Symbol s;
            s.specifier = Specifier::tryParse(lex);
            if (s.specifier != nullptr)
            {
                s.declarator = Declarator::tryParse(lex);
                f->params.push_back(s);
                if (lex.peakNext().type == OP_COMMA)
                {
                    lex.getNext();
                }
            }
        }
        if (lex.getNext().type != RP)
        {
            SyntaxError("Expect ')'");
        }
    }
    return f;
}
TFunction * TFunction::parse(Lexer &lex)
{
    TFunction * f = tryParse(lex);
    if (f == nullptr)
    {
        SyntaxError("Expect function parameters");
    }
    return f;
}
TArray * TArray::tryParse(Lexer &lex)
{
    TArray * array = nullptr;
    if (lex.peakNext().type == LSB)
    {
        lex.getNext();
        array = new TArray();
        // TODO: finish this
        //array->length = ConstExpression::eval(CondExpression::parse(lex));
        lex.getNext();
        if (lex.getNext().type != RSB)
        {
            SyntaxError("Expect ']'");
        }
        array->etype = TArray::tryParse(lex);
    }
    return array;
}
TArray * TArray::parse(Lexer &lex)
{
    TArray * array = tryParse(lex);
    if (array == nullptr)
    {
        SyntaxError("Expect array");
    }
    return array;
}
TPointer * TPointer::tryParse(Lexer &lex)
{
    if (lex.peakNext().type != OP_MUL)
    {
        return nullptr;
    }

    stack<PointerType> ptrs;
    while (lex.peakNext().type == OP_MUL)
    {
        lex.getNext();
        if (lex.peakNext().type == CONST)
        {
            lex.getNext();
            ptrs.push(PTR_CONST);
        }
        else
        {
            ptrs.push(PTR_NORMAL);
        }
    }

    TPointer * p = new TPointer();
    p->type = ptrs.top();
    p->etype = nullptr;
    ptrs.pop();
    TPointer * curr = p;
    while (!ptrs.empty())
    {
        TPointer * next = new TPointer();
        next->type = ptrs.top();
        ptrs.pop();
        curr->etype = next;
        curr = next;
    }
    return p;
}
Specifier * Specifier::parse(Lexer &lex)
{
    Specifier * s = new Specifier();
    s->type = nullptr;
    s->storage = TS_AUTO;

    bool finish = false;
    while (!finish)
    {
        switch (lex.peakNext().type)
        {
            case CONST: s->isconst = true; break;
            // case TYPE_VOID: s->type = new TVoid(); break;
            case TYPE_CHAR: s->type = new TChar(); break;
            // case TYPE_SHORT: s->type = new TShort(); break;
            case TYPE_INT: s->type = new TInt(); break;
            // case TYPE_LONG: s->type = new TLong(); break;
            // case TYPE_FLOAT: s->type = new TFloat(); break;
            // case TYPE_DOUBLE: s->type = new TDouble(); break;
            case SIGNED: s->isunsigned = false; break;
            case UNSIGNED: s->isunsigned = true; break;
            default:
                // TODO: support struct, enum, typedef-name
                // TODO: support type storage
                finish = true;
                break;
        }
        if (!finish) lex.getNext();
    }
    if (s->type == nullptr)
    {
        SyntaxError("Invalid specifier, missed type");
    }

    return s;
}
Specifier * Specifier::tryParse(Lexer &lex)
{
    Specifier * s = nullptr;
    if (IsTypeName(lex.peakNext().type))
    {
        s = parse(lex);
    }
    return s;
}
Declarator * Declarator::tryParse(Lexer &lex)
{
    if (lex.peakNext().type != OP_MUL && lex.peakNext().type != LP &&
        lex.peakNext().type != SYMBOL)
    {
        return nullptr;
    }

    Declarator * d = new Declarator();

    d->array = nullptr;
    d->child = nullptr;
    d->pointer = TPointer::tryParse(lex);

    if (lex.peakNext().type == LP)
    {
        lex.getNext();
        d->child = Declarator::parse(lex);
        if (lex.getNext().type != RP)
        {
            SyntaxError("Expect ')'");
        }
        if (lex.peakNext().type == LSB)
        {
            d->type = DT_ARRAY;
            d->array = TArray::parse(lex);
        }
        else if (lex.peakNext().type == LP)
        {
            d->type = DT_FUNCTION;
            d->function = TFunction::parse(lex);
        }
    }
    else if (lex.peakNext().type == SYMBOL)
    {
        Token t = lex.getNext();
        Declarator * d_symbol = d;
        // id
        if (lex.peakNext().type == LSB)
        {
            d->type = DT_ARRAY;
            d->array = TArray::parse(lex);
            d_symbol = new Declarator();
            d->child = d_symbol;
        }
        else if (lex.peakNext().type == LP)
        {
            d->type = DT_FUNCTION;
            d->function = TFunction::parse(lex);
            d_symbol = new Declarator();
            d->child = d_symbol;
        }
        d_symbol->child = nullptr;
        d_symbol->type = DT_SYMBOL;
        d_symbol->id = t.symid;
    }
    else
    {
        SyntaxError("Expect symbol or '('");
    }

    return d;
}
Declarator * Declarator::parse(Lexer &lex)
{
    Declarator * d = tryParse(lex);
    if (d == nullptr)
    {
        SyntaxError("Expect declarator");
    }
    return d;
}
SymbolTable * gGlobalTable = nullptr;
SymbolTable * gCurrentTable = nullptr;
void SymbolTable::AddTable(SymbolTable * table)
{
    // assert( gCurrentTable != NULL )
    table->parent = gCurrentTable;
    gCurrentTable = table;
}
void SymbolTable::RemoveTable()
{
    gCurrentTable = gCurrentTable->parent;
    // assert( gCurrentTable != NULL )
}
SymbolTable * SymbolTable::tryParse(Lexer &lex)
{
    if (!IsDeclaration(lex.peakNext().type))
    {
        return nullptr;
    }

    SymbolTable * table = new SymbolTable();
    table->parent = nullptr;
    while (IsDeclaration(lex.peakNext().type))
    {
        Specifier * specifier = Specifier::parse(lex);
        while (true)
        {
            Symbol s;
            s.specifier = specifier;
            s.declarator = Declarator::parse(lex);
            table->symbols.push_back(s);
            if (lex.peakNext().type == OP_COMMA)
            {
                lex.getNext();
            }
            else if (lex.peakNext().type == STMT_END)
            {
                lex.getNext();
                break;
            }
            else
            {
                SyntaxError("Unexpected token");
            }
        }
    }
    return table;
}

SyntaxNode *TranslationUnit::parse(Lexer &lex)
{
    if (lex.getNext().type != TYPE_INT)
    {
        SyntaxError("Expect 'int'");
    }
    if (lex.getNext().type != SYMBOL)
    {
        SyntaxError("Expect 'main'");
    }
    if (lex.getNext().type != LP)
    {
        SyntaxError("Expect '('");
    }
    if (lex.getNext().type != RP)
    {
        SyntaxError("Expect ')'");
    }
    return CompoundStatement::parse(lex);
}

SyntaxNode *TypeName::parse(Lexer &lex) { return nullptr; }
// no instance, only dispatch
SyntaxNode *Statement::parse(Lexer &lex)
{
    DebugParseTree(Statement);
    SyntaxNode *node = nullptr;
    switch (lex.peakNext().type)
    {
        case SYMBOL:
            if (lex.peakNext(1).type == OP_COLON)
            {
                node = LabelStatement::parse(lex);
            }
            else
            {
                node = ExpressionStatement::parse(lex);
            }
            break;
        case CASE:
        case DEFAULT:
            node = LabelStatement::parse(lex);
            break;
        case BLK_BEGIN:
            node = CompoundStatement::parse(lex);
            break;
        case IF:
        case SWITCH:
            node = SelectionStatement::parse(lex);
            break;
        case WHILE:
        case DO:
        case FOR:
            node = IterationStatement::parse(lex);
            break;
        case GOTO:
        case CONTINUE:
        case BREAK:
        case RETURN:
            node = JumpStatement::parse(lex);
            break;
        default:
            /* TODO: check FIRST(expr) */
            node = ExpressionStatement::parse(lex);
            break;
    }
    return node;
}
SyntaxNode *LabelStatement::parse(Lexer &lex)
{
    DebugParseTree(LabelStatement);
    SyntaxError("Unsupported feature: Label Statement");
    return nullptr;
}
SyntaxNode *CompoundStatement::parse(Lexer &lex)
{
    DebugParseTree(CompoundStatement);
    CompoundStatement *node = new CompoundStatement();

    if (lex.getNext().type != BLK_BEGIN)
    {
        SyntaxError("Expecting '{'");
    }

    // add declarations in current scope
    node->table = SymbolTable::tryParse(lex);
    if (node->table != nullptr)
    {
        SymbolTable::AddTable(node->table);
        node->table->debugPrint(lex);
    }

    // parse statements
    while (lex.peakNext().type != BLK_END)
    {
        node->stmts.push_back(Statement::parse(lex));
    }
    if (lex.getNext().type != BLK_END)
    {
        SyntaxError("Expecting '}'");
    }

    // remove declarations in current scope
    if (node->table != nullptr)
    {
        SymbolTable::RemoveTable();
    }

    return node;
}
SyntaxNode *ExpressionStatement::parse(Lexer &lex)
{
    DebugParseTree(ExpressionStatement);
    if (lex.peakNext().type == STMT_END)
    {
        lex.getNext();
        return nullptr;
    }
    else
    {
        SyntaxNode *node = Expression::parse(lex);
        if (lex.getNext().type != STMT_END)
            SyntaxError("Expecting ';'");
        return node;
    }
}
SyntaxNode *SelectionStatement::parse(Lexer &lex)
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
            stmt->expr = Expression::parse(lex);
            if (lex.getNext().type != RP)
            {
                SyntaxError("Expect ')'");
            }
            stmt->stmt = Statement::parse(lex);
            if (lex.peakNext().type == ELSE)
            {
                lex.getNext();
                stmt->stmt2 = Statement::parse(lex);
            }
            break;
        case SWITCH:
            if (lex.getNext().type != LP)
            {
                SyntaxError("Expect '('");
            }
            stmt->expr = Expression::parse(lex);
            if (lex.getNext().type != RP)
            {
                SyntaxError("Expect ')'");
            }
            // TODO: switch constraits
            stmt->stmt = Statement::parse(lex);
            break;
        default:
            SyntaxError("Unexpected token");
            break;
    }
    return stmt;
}
SyntaxNode *IterationStatement::parse(Lexer &lex)
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
            stmt->expr = Expression::parse(lex);
            if (lex.getNext().type != RP)
            {
                SyntaxError("Expect ')'");
            }
            stmt->stmt = Statement::parse(lex);
            break;
        case DO:
            stmt->type = DO_LOOP;
            stmt->stmt = Statement::parse(lex);
            if (lex.getNext().type != WHILE)
            {
                SyntaxError("Expect 'while'");
            }
            if (lex.getNext().type != LP)
            {
                SyntaxError("Expect '('");
            }
            stmt->expr = Expression::parse(lex);
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
            stmt->expr = ExpressionStatement::parse(lex);
            stmt->expr2 = ExpressionStatement::parse(lex);
            if (lex.peakNext().type != RP)
            {
                stmt->expr3 = Expression::parse(lex);
            }
            if (lex.getNext().type != RP)
            {
                SyntaxError("Expect ')'");
            }
            stmt->stmt = Statement::parse(lex);
            break;
        default:
            SyntaxError("Unexpected token");
            break;
    }
    return stmt;
}
SyntaxNode *JumpStatement::parse(Lexer &lex)
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
            stmt->expr = ExpressionStatement::parse(lex);
            break;
        default:
            SyntaxError("Unexpected token");
            break;
    }
    return stmt;
}

SyntaxNode *Expression::parse(Lexer &lex)
{
    DebugParseTree(Expression);
    Expression *expr = nullptr;
    SyntaxNode *e;

    while (true)
    {
        switch (lex.peakNext().type)
        {
            // FIRST(expr) = ++ -- unary-op sizeof id constant string (
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
            case CONST_ENUM:
            case STRING:
            case LP:
                e = AssignExpression::parse(lex);
                break;
            default:
                break;
        }
        if (expr)
        {
            expr->exprs.push_back(e);
            if (lex.peakNext().type != OP_COMMA)
                return expr;
            else
                lex.getNext();
        }
        else
        {
            if (lex.peakNext().type != OP_COMMA)
            {
                return e;
            }
            else
            {
                expr = new Expression();
                expr->exprs.push_back(e);
            }
        }
    }
}
SyntaxNode *AssignExpression::parse(Lexer &lex)
{
    DebugParseTree(AssignExpression);
    AssignExpression *expr = new AssignExpression();

    // condition or unary?
    SyntaxNode *e;

    // TODO: fix it, unary is not condition-expr
    while (true)
    {
        e = CondExpression::parse(lex);
        if (IsAssignOperator(lex.peakNext().type))
        {
            lex.getNext();
            expr->targets.push_back(e);
        }
        else
        {
            expr->source = e;
            break;
        }
    }

    return expr;
}
SyntaxNode *CondExpression::parse(Lexer &lex)
{
    DebugParseTree(CondExpression);
    SyntaxNode *e = OrExpression::parse(lex);
    if (lex.peakNext().type == OP_QMARK)
    {
        CondExpression *expr = new CondExpression();
        expr->cond = e;
        lex.getNext();
        expr->left = Expression::parse(lex);
        if (lex.getNext().type != OP_COLON)
        {
            SyntaxError("Expect ':'");
        }
        expr->right = CondExpression::parse(lex);
        return expr;
    }
    else
    {
        return e;
    }
}
SyntaxNode *OrExpression::parse(Lexer &lex)
{
    DebugParseTree(OrExpression);
    SyntaxNode *e = AndExpression::parse(lex);
    if (lex.peakNext().type == BOOL_OR)
    {
        OrExpression *expr = new OrExpression();
        expr->exprs.push_back(e);
        while (lex.peakNext().type == BOOL_OR)
        {
            lex.getNext();
            expr->exprs.push_back(AndExpression::parse(lex));
        }
        return expr;
    }
    else
    {
        return e;
    }
}
SyntaxNode *AndExpression::parse(Lexer &lex)
{
    DebugParseTree(AndExpression);
    SyntaxNode *e = BitOrExpression::parse(lex);
    if (lex.peakNext().type == BOOL_AND)
    {
        AndExpression *expr = new AndExpression();
        expr->exprs.push_back(e);
        while (lex.peakNext().type == BOOL_AND)
        {
            lex.getNext();
            expr->exprs.push_back(BitOrExpression::parse(lex));
        }
        return expr;
    }
    else
    {
        return e;
    }
}
SyntaxNode *BitOrExpression::parse(Lexer &lex)
{

    SyntaxNode *e = BitXorExpression::parse(lex);
    if (lex.peakNext().type == BIT_OR)
    {
        BitOrExpression *expr = new BitOrExpression();
        expr->exprs.push_back(e);
        while (lex.peakNext().type == BIT_OR)
        {
            lex.getNext();
            expr->exprs.push_back(BitXorExpression::parse(lex));
        }
        return expr;
    }
    else
    {
        return e;
    }
}
SyntaxNode *BitXorExpression::parse(Lexer &lex)
{
    SyntaxNode *e = BitAndExpression::parse(lex);
    if (lex.peakNext().type == BIT_XOR)
    {
        BitXorExpression *expr = new BitXorExpression();
        expr->exprs.push_back(e);
        while (lex.peakNext().type == BIT_XOR)
        {
            lex.getNext();
            expr->exprs.push_back(BitAndExpression::parse(lex));
        }
        return expr;
    }
    else
    {
        return e;
    }
}
SyntaxNode *BitAndExpression::parse(Lexer &lex)
{
    SyntaxNode *e = EqExpression::parse(lex);
    if (lex.peakNext().type == BIT_AND)
    {
        BitAndExpression *expr = new BitAndExpression();
        expr->exprs.push_back(e);
        while (lex.peakNext().type == BIT_AND)
        {
            lex.getNext();
            expr->exprs.push_back(EqExpression::parse(lex));
        }
        return expr;
    }
    else
    {
        return e;
    }
}
SyntaxNode *EqExpression::parse(Lexer &lex)
{
    SyntaxNode *e = RelExpression::parse(lex);
    if (lex.peakNext().type == REL_EQ || lex.peakNext().type == REL_NE)
    {
        EqExpression *expr = new EqExpression();
        expr->exprs.push_back(e);
        while (lex.peakNext().type == REL_EQ || lex.peakNext().type == REL_NE)
        {
            expr->ops.push_back(lex.getNext().type);
            expr->exprs.push_back(RelExpression::parse(lex));
        }
        return expr;
    }
    else
    {
        return e;
    }
}
SyntaxNode *RelExpression::parse(Lexer &lex)
{
    SyntaxNode *e = ShiftExpression::parse(lex);
    if (lex.peakNext().type == REL_LT || lex.peakNext().type == REL_LE ||
        lex.peakNext().type == REL_GT || lex.peakNext().type == REL_GE)
    {
        RelExpression *expr = new RelExpression();
        expr->exprs.push_back(e);
        while (lex.peakNext().type == REL_LT || lex.peakNext().type == REL_LE ||
               lex.peakNext().type == REL_GT || lex.peakNext().type == REL_GE)
        {
            expr->ops.push_back(lex.getNext().type);
            expr->exprs.push_back(ShiftExpression::parse(lex));
        }
        return expr;
    }
    else
    {
        return e;
    }
}
SyntaxNode *ShiftExpression::parse(Lexer &lex)
{
    SyntaxNode *e = AddExpression::parse(lex);
    if (lex.peakNext().type == BIT_SLEFT || lex.peakNext().type == BIT_SRIGHT)
    {
        ShiftExpression *expr = new ShiftExpression();
        expr->exprs.push_back(e);
        while (lex.peakNext().type == BIT_SLEFT ||
               lex.peakNext().type == BIT_SRIGHT)
        {
            expr->ops.push_back(lex.getNext().type);
            expr->exprs.push_back(AddExpression::parse(lex));
        }
        return expr;
    }
    else
    {
        return e;
    }
}
SyntaxNode *AddExpression::parse(Lexer &lex)
{
    DebugParseTree(AddExpression);
    SyntaxNode *e = MulExpression::parse(lex);
    if (lex.peakNext().type == OP_ADD || lex.peakNext().type == OP_SUB)
    {
        AddExpression *expr = new AddExpression();
        expr->exprs.push_back(e);
        while (lex.peakNext().type == OP_ADD || lex.peakNext().type == OP_SUB)
        {
            expr->ops.push_back(lex.getNext().type);
            expr->exprs.push_back(MulExpression::parse(lex));
        }
        return expr;
    }
    else
    {
        return e;
    }
}
SyntaxNode *MulExpression::parse(Lexer &lex)
{
    DebugParseTree(MulExpression);
    SyntaxNode *e = CastExpression::parse(lex);
    if (lex.peakNext().type == OP_MUL || lex.peakNext().type == OP_DIV ||
        lex.peakNext().type == OP_MOD)
    {
        MulExpression *expr = new MulExpression();
        expr->exprs.push_back(e);
        while (lex.peakNext().type == OP_MUL || lex.peakNext().type == OP_DIV ||
               lex.peakNext().type == OP_MOD)
        {
            expr->ops.push_back(lex.getNext().type);
            expr->exprs.push_back(CastExpression::parse(lex));
        }
        return expr;
    }
    else
    {
        return e;
    }
}
SyntaxNode *CastExpression::parse(Lexer &lex)
{
    // cast or unary
    if (lex.peakNext().type == LP &&
        IsTypeName(lex.peakNext(1).type))
    {
        CastExpression *expr = new CastExpression();
        while (lex.peakNext().type == LP &&
               IsTypeName(lex.peakNext(1).type))
        {
            if (lex.getNext().type != LP)
            {
                SyntaxError("Expect '('");
            }
            expr->types.push_back(TypeName::parse(lex));
            if (lex.getNext().type != RP)
            {
                SyntaxError("Expect ')'");
            }
        }
        expr->target = UnaryExpression::parse(lex);
        return expr;
    }
    else
    {
        return UnaryExpression::parse(lex);
    }
}
SyntaxNode *UnaryExpression::parse(Lexer &lex)
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
                expr->expr = UnaryExpression::parse(lex);
                break;
            case SIZEOF:
                expr->op = lex.getNext().type;
                if (lex.peakNext().type == LP)
                {
                    expr->expr = TypeName::parse(lex);
                }
                else
                {
                    expr->expr = UnaryExpression::parse(lex);
                }
                break;
            default:
                expr->op = lex.getNext().type;
                expr->expr = CastExpression::parse(lex);
                break;
        }
        return expr;
    }
    else
    {
        return PostfixExpression::parse(lex);
    }
}
SyntaxNode *PostfixExpression::parse(Lexer &lex)
{
    DebugParseTree(PostfixExpression);
    return PrimaryExpression::parse(lex);
}
SyntaxNode *PrimaryExpression::parse(Lexer &lex)
{
    DebugParseTree(PrimaryExpression);
    PrimaryExpression * p = nullptr;
    switch (lex.peakNext().type)
    {
        case SYMBOL:
        case CONST_INT:
            p = new PrimaryExpression();
            p->t = lex.getNext();
            break;
        default:
            SyntaxError("Unsupported primary expression");
            break;
    }
    return p;
}
