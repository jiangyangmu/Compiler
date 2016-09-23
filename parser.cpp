#include <stack>
#include <iostream>
using namespace std;

#include "lexer.h"
#include "parser.h"

bool IsDeclaration(TokenType t)
{
    bool decl = false;
    switch (t)
    {
        case TYPEDEF: case EXTERN: case STATIC: case AUTO: case REGISTER:
        case TYPE_VOID: case TYPE_CHAR: case TYPE_SHORT: case TYPE_INT:
        case TYPE_LONG: case TYPE_FLOAT: case TYPE_DOUBLE: case SIGNED:
        case UNSIGNED: case TYPE_STRUCT: case TYPE_ENUM:
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
        case ASSIGN: case ASSIGN_ADD: case ASSIGN_SUB: case ASSIGN_MUL:
        case ASSIGN_DIV: case ASSIGN_MOD:case ASSIGN_SLEFT:
        case ASSIGN_SRIGHT: case ASSIGN_AND: case ASSIGN_OR:
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
        case BIT_AND: case BIT_NOT:
        case OP_MUL: case OP_ADD: case OP_SUB: case BOOL_NOT:
            is = true;
            break;
        default:
            break;
    }
    return is;
}

/*
void SyntaxError(const char * msg)
{
    cout << "Syntax: " << msg << endl;
    exit(1);
}
*/
#define SyntaxError(msg) do { \
    cout << "Syntax: " << msg << " at " << __FILE__ << ':' << to_string(__LINE__) << endl; \
    exit(1); \
    } while (0)

Specifier * Specifier::ParseSpecifier(Lexer &lex)
{
    Specifier * sp = new Specifier();
    sp->isconst = sp->isstatic = sp->issigned = false;
    sp->stype = SPEC_NONE;

    bool finish = false;
    while (!finish)
    {
        switch (lex.peakNext().type)
        {
            // storage specifiers
            case STATIC: sp->isstatic = true; break;
            case TYPEDEF: case EXTERN: case AUTO: case REGISTER: break;
            // type qualifiers
            case CONST: sp->isconst = true; break;
            case VOLATILE: break;
            // type specifiers
            case TYPE_VOID: sp->stype = SPEC_VOID; break;
            case TYPE_CHAR: sp->stype = SPEC_CHAR; break;
            case TYPE_SHORT: sp->stype = SPEC_SHORT; break;
            case TYPE_INT: sp->stype = SPEC_INT; break;
            case TYPE_LONG: sp->stype = SPEC_LONG; break;
            case TYPE_FLOAT: sp->stype = SPEC_FLOAT; break;
            case TYPE_DOUBLE: sp->stype = SPEC_DOUBLE; break;
            // TODO: struct, enum, ...
            default: finish = true; break;
        }
        if (!finish) lex.getNext();
    }

    if (sp->stype == SPEC_NONE)
    {
        SyntaxError("Expect type specifier");
    }
    // TODO: other constraits
    return sp;
}

bool Specifier::MaybeTypeName(TokenType t)
{
    bool is = false;
    switch (t)
    {
        // type qualifiers
        case CONST: case VOLATILE:
        // type specifiers
        case TYPE_VOID: case TYPE_CHAR: case TYPE_SHORT: case TYPE_INT:
        case TYPE_LONG: case TYPE_FLOAT: case TYPE_DOUBLE:
        case UNSIGNED: case SIGNED: case TYPE_STRUCT: case TYPE_ENUM:
            is = true;
            break;
        default:
            break;
    }
    return is;
}

void Specifier::print()
{
    cout << '[';
    if (isconst) cout << "CONST,";
    if (isstatic) cout << "STATIC,";
    if (issigned) cout << "SIGNED,";
    switch (stype)
    {
        case SPEC_VOID: cout << "void"; break;
        case SPEC_CHAR: cout << "char"; break;
        case SPEC_SHORT: cout << "short"; break;
        case SPEC_INT: cout << "int"; break;
        case SPEC_LONG: cout << "long"; break;
        case SPEC_FLOAT: cout << "float"; break;
        case SPEC_DOUBLE: cout << "double"; break;
        default: cout << "unknown"; break;
    }
    cout << ']';
}

Pointer Pointer::ParsePointer(Lexer &lex)
{
    Pointer p;
    int ptype;
    while (lex.peakNext().type == OP_MUL)
    {
        lex.getNext();
        ptype = 0;
        while (true)
        {
            if (lex.peakNext().type == CONST)
            {
                ptype |= PTR_CONST;
                lex.getNext();
            }
            else if (lex.peakNext().type == VOLATILE)
            {
                lex.getNext();
            }
            else break;
        }
        p.ptypes.push_back(ptype);
    }
    if (p.ptypes.empty())
    {
        SyntaxError("Expect pointer");
    }
    return p;
}

void Pointer::print()
{
    if (ptypes.empty()) return;
    for (int i = ptypes.size() - 1; i >= 0; --i)
    {
        switch (ptypes[i])
        {
            case 0: cout << "pointer of "; break;
            case 1: cout << "const pointer of "; break;
            default: break;
        }
    }
}

Function Function::ParseFunction(Lexer &lex)
{
    Function f;
    if (lex.getNext().type != LP)
    {
        SyntaxError("Expect '('");
    }
    while (lex.peakNext().type != RP)
    {
        f.specifiers.push_back(Specifier::ParseSpecifier(lex));
        f.decls.push_back(Declarator::ParseDeclarator(lex));
    }
    if (lex.getNext().type != RP)
    {
        SyntaxError("Expect ')'");
    }
    return f;
}

Array Array::ParseArray(Lexer &lex)
{
    Array a;
    if (lex.getNext().type != LSB)
    {
        SyntaxError("Expect '['");
    }
    // TODO: add EvalType
    //a.length = CondExpression::parse(lex).eval();
    if (lex.getNext().type != RSB)
    {
        SyntaxError("Expect ']'");
    }
    return a;
}

Declarator * Declarator::ParseDeclarator(Lexer &lex)
{
    Declarator * d = new Declarator();
    d->child = nullptr;
    d->pointer = nullptr;
    d->array = nullptr;
    if (lex.peakNext().type == OP_MUL)
    {
        d->pointer = new Pointer(Pointer::ParsePointer(lex));
    }

    int id;
    switch (lex.peakNext().type)
    {
        case LP:
            lex.getNext();
            d->child = ParseDeclarator(lex);
            if (lex.getNext().type != RP)
            {
                SyntaxError("Expect ')'");
            }
            // handle d type
            if (lex.peakNext().type == LSB)
            {
                d->dtype = DT_ARRAY;
                d->array = new Array(Array::ParseArray(lex));
            }
            else if (lex.peakNext().type == LP)
            {
                d->dtype = DT_FUNCTION;
                d->function = new Function(Function::ParseFunction(lex));
            }
            break;
        case SYMBOL:
            id = lex.getNext().symid;
            if (lex.peakNext().type == STMT_END
             || lex.peakNext().type == RP)
            {
                d->dtype = DT_ID;
                d->id = id;
            }
            else
            {
                d->child = new Declarator();
                d->child->child = nullptr;
                d->child->pointer = nullptr;
                d->child->id = id;
                d->child->dtype = DT_ID;
                if (lex.peakNext().type == LSB)
                {
                    d->dtype = DT_ARRAY;
                    d->array = new Array(Array::ParseArray(lex));
                }
                else if (lex.peakNext().type == LP)
                {
                    d->dtype = DT_FUNCTION;
                    d->function = new Function(Function::ParseFunction(lex));
                }
                else
                {
                    SyntaxError("Expect '[' or '('");
                }
            }
            break;
        default: SyntaxError("Expect symbol or '('"); break;
    }
    return d;
}

void Declarator::print()
{
    if (child) child->print();
    switch (dtype)
    {
        case DT_ID: cout << "ID " << id << " is "; break;
        case DT_ARRAY: cout << "array of "; break;
        case DT_FUNCTION: cout << "function returns "; break;
        case DT_NONE: cout << "unknown "; break;
    }
    if (pointer) pointer->print();
}

void SymbolDecl::print()
{
    declarator->print();
    specifier->print();
    cout << endl;
}
void SymbolTable::add_symbol(Lexer &lex)
{
    Specifier * specifier = Specifier::ParseSpecifier(lex);
    while (true)
    {
        SymbolDecl symbol;
        symbol.specifier = specifier;
        symbol.declarator = Declarator::ParseDeclarator(lex);
        symbol.print();
        symbols.push_back(symbol);
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
bool SymbolTable::search(StringBuf &name)
{
    return false;
}

stack<SymbolTable> gTables;

SymbolTable * AddSymbolTable()
{
    cout << "SymbolTable: new()" << endl;
    gTables.push(SymbolTable());
    return &gTables.top();
}

void RemoveSymbolTable()
{
    cout << "SymbolTable: delete()" << endl;
    gTables.pop();
}

SyntaxNode * TranslationUnit::parse(Lexer &lex)
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

SyntaxNode * TypeName::parse(Lexer &lex)
{
    return nullptr;
}

// no instance, only dispatch
SyntaxNode * Statement::parse(Lexer &lex)
{
    SyntaxNode * node = nullptr;
    switch (lex.peakNext().type)
    {
        case SYMBOL:
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
SyntaxNode * LabelStatement::parse(Lexer &lex)
{
    SyntaxError("Unsupported feature: Label Statement");
    return nullptr;
}
SyntaxNode * CompoundStatement::parse(Lexer &lex)
{
    CompoundStatement * node = new CompoundStatement();

    SymbolTable * table = nullptr;
    if (lex.getNext().type != BLK_BEGIN)
    {
        SyntaxError("Expecting '{'");
    }
    if (IsDeclaration(lex.peakNext().type))
    {
        table = AddSymbolTable();
        while (IsDeclaration(lex.peakNext().type))
        {
            table->add_symbol(lex);
        }
    }
    while (lex.getNext().type != BLK_END)
    {
        node->stmts.push_back(Statement::parse(lex));
    }
    if (lex.getNext().type != BLK_END)
    {
        SyntaxError("Expecting '}'");
    }
    if (table)
    {
        RemoveSymbolTable();
    }

    return node;
}
SyntaxNode * ExpressionStatement::parse(Lexer &lex)
{
    if (lex.peakNext().type == STMT_END)
    {
        lex.getNext();
        return nullptr;
    }
    else
    {
        SyntaxNode * node = Expression::parse(lex);
        if (lex.getNext().type != STMT_END)
            SyntaxError("Expecting ';'");
        return node;
    }
}
SyntaxNode * SelectionStatement::parse(Lexer &lex)
{
    SelectionStatement * stmt = new SelectionStatement();
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
SyntaxNode * IterationStatement::parse(Lexer &lex)
{
    IterationStatement * stmt = new IterationStatement();
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
SyntaxNode * JumpStatement::parse(Lexer &lex)
{
    JumpStatement * stmt = new JumpStatement();
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

SyntaxNode * Expression::parse(Lexer &lex)
{
    Expression * expr = nullptr;
    SyntaxNode * e;

    while (true)
    {
        switch (lex.peakNext().type)
        {
            // FIRST(expr) = ++ -- unary-op sizeof id constant string (
            case OP_INC: case OP_DEC:
            case BIT_AND: case BIT_NOT: case BOOL_NOT:
            case OP_MUL: case OP_ADD: case OP_SUB:
            case SIZEOF: case SYMBOL:
            case CONST_INT: case CONST_CHAR: case CONST_FLOAT:
            case CONST_ENUM: case STRING: case LP:
                e = AssignExpression::parse(lex);
                break;
            default:
                break;
        }
        if (expr)
        {
            expr->exprs.push_back(e);
            if (lex.peakNext().type != OP_COMMA) return expr;
            else lex.getNext();
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
SyntaxNode * AssignExpression::parse(Lexer &lex)
{
    AssignExpression * expr = new AssignExpression();

    // condition or unary?
    SyntaxNode * e;

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
SyntaxNode * CondExpression::parse(Lexer &lex)
{
    SyntaxNode * e = OrExpression::parse(lex);
    if (lex.peakNext().type == OP_QMARK)
    {
        CondExpression * expr = new CondExpression();
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
SyntaxNode * OrExpression::parse(Lexer &lex)
{
    SyntaxNode * e = AndExpression::parse(lex);
    if (lex.peakNext().type == BOOL_OR)
    {
        OrExpression * expr = new OrExpression();
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
SyntaxNode * AndExpression::parse(Lexer &lex)
{
    SyntaxNode * e = BitOrExpression::parse(lex);
    if (lex.peakNext().type == BOOL_AND)
    {
        AndExpression * expr = new AndExpression();
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
SyntaxNode * BitOrExpression::parse(Lexer &lex)
{
    SyntaxNode * e = BitXorExpression::parse(lex);
    if (lex.peakNext().type == BIT_OR)
    {
        BitOrExpression * expr = new BitOrExpression();
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
SyntaxNode * BitXorExpression::parse(Lexer &lex)
{
    SyntaxNode * e = BitAndExpression::parse(lex);
    if (lex.peakNext().type == BIT_XOR)
    {
        BitXorExpression * expr = new BitXorExpression();
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
SyntaxNode * BitAndExpression::parse(Lexer &lex)
{
    SyntaxNode * e = EqExpression::parse(lex);
    if (lex.peakNext().type == BIT_AND)
    {
        BitAndExpression * expr = new BitAndExpression();
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
SyntaxNode * EqExpression::parse(Lexer &lex)
{
    SyntaxNode * e = RelExpression::parse(lex);
    if (lex.peakNext().type == REL_EQ || lex.peakNext().type == REL_NE)
    {
        EqExpression * expr = new EqExpression();
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
SyntaxNode * RelExpression::parse(Lexer &lex)
{
    SyntaxNode * e = ShiftExpression::parse(lex);
    if (lex.peakNext().type == REL_LT
     || lex.peakNext().type == REL_LE
     || lex.peakNext().type == REL_GT
     || lex.peakNext().type == REL_GE)
    {
        RelExpression * expr = new RelExpression();
        expr->exprs.push_back(e);
        while (lex.peakNext().type == REL_LT
            || lex.peakNext().type == REL_LE
            || lex.peakNext().type == REL_GT
            || lex.peakNext().type == REL_GE)
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
SyntaxNode * ShiftExpression::parse(Lexer &lex)
{
    SyntaxNode * e = AddExpression::parse(lex);
    if (lex.peakNext().type == BIT_SLEFT || lex.peakNext().type == BIT_SRIGHT)
    {
        ShiftExpression * expr = new ShiftExpression();
        expr->exprs.push_back(e);
        while (lex.peakNext().type == BIT_SLEFT || lex.peakNext().type == BIT_SRIGHT)
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
SyntaxNode * AddExpression::parse(Lexer &lex)
{
    SyntaxNode * e = MulExpression::parse(lex);
    if (lex.peakNext().type == OP_ADD || lex.peakNext().type == OP_SUB)
    {
        AddExpression * expr = new AddExpression();
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
SyntaxNode * MulExpression::parse(Lexer &lex)
{
    SyntaxNode * e = CastExpression::parse(lex);
    if (lex.peakNext().type == OP_MUL
     || lex.peakNext().type == OP_DIV
     || lex.peakNext().type == OP_MOD)
    {
        MulExpression * expr = new MulExpression();
        expr->exprs.push_back(e);
        while (lex.peakNext().type == OP_MUL
            || lex.peakNext().type == OP_DIV
            || lex.peakNext().type == OP_MOD)
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
SyntaxNode * CastExpression::parse(Lexer &lex)
{
    // cast or unary
    if (lex.peakNext().type == LP
     && Specifier::MaybeTypeName(lex.peakNext(1).type))
    {
        CastExpression * expr = new CastExpression();
        while (lex.peakNext().type == LP
            && Specifier::MaybeTypeName(lex.peakNext(1).type))
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
SyntaxNode * UnaryExpression::parse(Lexer &lex)
{
    if (IsUnaryOperator(lex.peakNext().type))
    {
        return nullptr;
    }
    else
    {
        return PostfixExpression::parse(lex);
    }
}
SyntaxNode * PostfixExpression::parse(Lexer &lex)
{
    return nullptr;
}
SyntaxNode * PrimaryExpression::parse(Lexer &lex)
{
    return nullptr;
}
