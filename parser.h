#pragma once

#include <iostream>
using namespace std;

#include "lexer.h"

// helplers
bool IsDeclaration(TokenType t)
{
    bool decl = false;
    switch (t)
    {
        case TYPEDEF: case EXTERN: case STATIC: case AUTO: case REGISTER:
        case VOID: case CHAR: case SHORT: case INT: case LONG: case FLOAT:
        case DOUBLE: case SIGNED: case UNSIGNED: case STRUCT: case ENUM:
            // TODO: other typedef names
            decl = true;
            break;
        default:
            break;
    }
    return decl;
}

typedef enum SpecType
{
    SPEC_NONE, SPEC_VOID, SPEC_CHAR, SPEC_SHORT, SPEC_INT, SPEC_LONG, SPEC_FLOAT, SPEC_DOUBLE
} SpecType;
struct Specifier
{
    bool isconst, isstatic, issigned;
    SpecType stype;
    static Specifier ParseSpecifier(Lexer &lex)
    {
        Specifier sp;
        sp.isconst = sp.isstatic = sp.issigned = false;
        sp.stype = SPEC_NONE;

        bool finish = false;
        while (!finish)
        {
            switch (lex.peakNext().type)
            {
                // storage specifiers
                case STATIC: sp.isstatic = true; break;
                case TYPEDEF: case EXTERN: case AUTO: case REGISTER: break;
                // type qualifiers
                case CONST: sp.isconst = true; break;
                case VOLATILE: break;
                // type specifiers
                case TYPE_VOID: sp.stype = SPEC_VOID; break;
                case TYPE_CHAR: sp.stype = SPEC_CHAR; break;
                case TYPE_SHORT: sp.stype = SPEC_SHORT; break;
                case TYPE_INT: sp.stype = SPEC_INT; break;
                case TYPE_LONG: sp.stype = SPEC_LONG; break;
                case TYPE_FLOAT: sp.stype = SPEC_FLOAT; break;
                case TYPE_DOUBLE: sp.stype = SPEC_DOUBLE; break;
                default: finish = true; break;
            }
            if (!finish) lex.getNext();
        }

        if (sp.type == SPEC_NONE)
        {
            SyntexError("Expect type specifier");
        }
        // TODO: other constraits
        return sp;
    }

    static bool MaybeTypeName(TokenType t)
    {
        bool maybe = false;
        switch (t)
        {
            // type qualifiers
            case CONST: case VOLATILE:
            // type specifiers
            case TYPE_VOID: case TYPE_CHAR: case TYPE_SHORT: case TYPE_INT:
            case TYPE_LONG: case TYPE_FLOAT: case TYPE_DOUBLE:
                maybe = true;
                break;
            default:
                break;
        }
        return maybe;
    }
};

typedef enum PointerType
{
    PTR_NORMAL = 0, PTR_CONST = 1 // , PTR_VOLATILE = 2
} PointerType;
struct Pointer
{
    stack<int> ptypes;
    static Pointer ParsePointer(Lexer &lex)
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
            p.ptypes.push(ptype);
        }
        if (p.ptypes.empty())
        {
            SyntexError("Expect pointer");
        }
        return p;
    }
};

struct Function
{
    vector<Specifier> specifiers;
    vector<Declarator *> decls;
    static Function ParseFunction(Lexer &lex)
    {
        Function f;
        if (lex.getNext().type != LB)
        {
            SyntexError("Expect '('");
        }
        f.specifiers.push_back(Specifier::ParseSpecifier(lex));
        f.decls.push_back(Declarator::ParseDeclarator(lex));
        if (lex.getNext().type != RB)
        {
            SyntexError("Expect ')'");
        }
        return f;
    }
};

struct Array
{
    int length;
    static Array ParseArray(Lexer &lex)
    {
        Array a;
        if (lex.getNext().type != LSB)
        {
            SyntexError("Expect '['");
        }
        // TODO: add EvalType
        a.length = CondExpression(lex).eval();
        if (lex.getNext().type != RSB)
        {
            SyntexError("Expect ']'");
        }
        return a;
    }
};

typedef enum DeclType
{
    DT_NONE, DT_ID, DT_ARRAY, DT_FUNCTION
} DeclType;
struct Declarator
{
    Declarator * child;
    Pointer * pointer;
    DeclType dtype;
    union
    {
        Array * array;
        Function * function;
        int id; /* lex.symid */
    };
    static Declarator * ParseDeclarator(Lexer &lex)
    {
        Declarator * d = new Declarator();
        d->child = d->pointer = d->array = nullptr;
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
                    SyntexError("Expect ')'");
                }
                // handle d type
                if (lex.peakNext().type == LSB)
                {
                    d->dtype = DT_ARRAY;
                    d->array = new Array(ParseArray(lex));
                }
                else (lex.peakNext().type == LP)
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
                    d->id = id;
                }
                else
                {
                    d->child = new Declarator();
                    d->child->child = d->child->pointer = nullptr;
                    d->child->id = id;
                    if (lex.peakNext().type == LSB)
                    {
                        d->dtype = DT_ARRAY;
                        d->array = new Array(ParseArray(lex));
                    }
                    else if (lex.peakNext().type == LP)
                    {
                        d->dtype = DT_FUNCTION;
                        d->function = new Function(Function::ParseFunction(lex));
                    }
                    else
                    {
                        SyntexError("Expect '[' or '('");
                    }
                }
                break;
            default: SyntexError("Expect symbol or '('"); break;
        }
        return d;
    }
};

struct SymbolDecl
{
    // L: sepcifier+pointer
    // M: id|(decl)
    // R: array | function
    // D -> P(MR)
    // M -> (D)
    // <Pointer, Array|Function|Id>
    Specifier specifier;
    Declarator declarator;

    // calculated from specifier & declarator
    int mem_size;
    StringRef name;

    void print()
    {
    }

    // operations: every type should have their own operation set
};

class SymbolTable
{
    vector<SymbolDecl> symbols;
public:
    void add_symbol(Lexer &Lex)
    {
        Specifier specifier = Specifier::ParseSpecifier(lex);
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
            else (lex.peakNext().type == STMT_END)
            {
                lex.getNext();
                break;
            }
            else
            {
                SyntexError("Unexpected token");
            }
        }
    }
    bool search(StringBuf &name)
    {
        return false;
    }
};
stack<SymbolTable> gTables;

SymbolTable * AddSymbolTable()
{
    gTables.push(SymbolTable());
    return &gTables.top();
}

void RemoveSymbolTable()
{
    gTables.pop();
}

class SyntaxNode
{
public:
    //virtual SyntaxNode * parse(Lexer &) = 0;
};

// no instance, only dispatch
class Statement : public SyntaxNode
{
public:
    static SyntaxNode * parse(Lexer &lex)
    {
        SyntaxNode * node = nullptr;
        switch (lex.peakNext().type)
        {
            case SYMBOL:
            case CASE:
            case DEFAULT:
                node = LabelStatement::parse(lex);
                break;
            case LSB:
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
};
class LabelStatement : public SyntaxNode
{
public:
    static SyntaxNode * parse(Lexer &lex)
    {
        SyntexError("Unsupported feature: Label Statement");
        return nullptr;
    }
};
class CompoundStatement : public SyntaxNode
{
    vector<SyntaxNode *> stmts;
public:
    static SyntaxNode * parse(Lexer &lex)
    {
        CompoundStatement * node = new CompoundStatement();

        SymbolTable * table = nullptr;
        if (lex.getNext().type != LSB)
        {
            SyntexError("Expecting '{'");
        }
        if (IsDeclaration(lex.peakNext().type))
        {
            table = AddSymbolTable();
            while (IsDeclaration(lex.peakNext().type))
            {
                table->add_symbol(lex);
            }
        }
        while (lex.getNext().type != RSB)
        {
            node->stmts.push_back(Statement::parse(lex));
        }
        if (lex.getNext().type != RSB)
        {
            SyntexError("Expecting '}'");
        }
        if (table)
        {
            RemoveSymbolTable();
        }

        return node;
    }
};
// no instance, dispatch only
class ExpressionStatement : public SyntaxNode
{
public:
    static SyntaxNode * parse(Lexer &lex)
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
                SyntexError("Expecting ';'");
            return node;
        }
    }
};
class SelectionStatement : public SyntaxNode
{
    SyntaxNode * expr;
    SyntaxNode * stmt;
    SyntaxNode * stmt2;
public:
    static SyntaxNode * parse(Lexer &lex)
    {
        SelectionStatement * stmt = new SelectionStatement();
        stmt->stmt2 = nullptr;
        switch (lex.getNext().type)
        {
            case IF:
                if (lex.getNext().type != LP)
                {
                    SyntexError("Expect '('");
                }
                stmt->expr = Expression::parse(lex);
                if (lex.getNext().type != RP)
                {
                    SyntexError("Expect ')'");
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
                    SyntexError("Expect '('");
                }
                stmt->expr = Expression::parse(lex);
                if (lex.getNext().type != RP)
                {
                    SyntexError("Expect ')'");
                }
                // TODO: switch constraits
                stmt->stmt = Statement::parse(lex);
                break;
            default:
                SyntexError("Unexpected token");
                break;
        }
        return stmt;
    }
};
class IterationStatement : public SyntaxNode
{
    typedef enum IterationType { WHILE_LOOP, DO_LOOP, FOR_LOOP } IterationType;

    IterationType type;
    SyntaxNode * expr;
    SyntaxNode * expr2;
    SyntaxNode * expr3;
    SyntaxNode * stmt;
public:
    static SyntaxNode * parse(Lexer &lex)
    {
        IterationStatement * stmt = new IterationStatement();
        switch (lex.getNext().type)
        {
            case WHILE:
                stmt->type = WHILE_LOOP;
                if (lex.getNext().type != LP)
                {
                    SyntexError("Expect '('");
                }
                stmt->expr = Expression::parse(lex);
                if (lex.getNext().type != RP)
                {
                    SyntexError("Expect ')'");
                }
                stmt->stmt = Statement::parse(lex);
                break;
            case DO:
                stmt->type = DO_LOOP;
                stmt->stmt = Statement::parse(lex);
                if (lex.getNext().type != WHILE)
                {
                    SyntexError("Expect 'while'");
                }
                if (lex.getNext().type != LP)
                {
                    SyntexError("Expect '('");
                }
                stmt->expr = Expression::parse(lex);
                if (lex.getNext().type != RP)
                {
                    SyntexError("Expect ')'");
                }
                break;
            case FOR:
                stmt->type = FOR_LOOP;
                if (lex.getNext().type != LP)
                {
                    SyntexError("Expect '('");
                }
                stmt->expr = ExpressionStatement::parse(lex);
                stmt->expr2 = ExpressionStatement::parse(lex);
                if (lex.peakNext().type != RP)
                {
                    stmt->expr3 = Expression::parse(lex);
                }
                if (lex.getNext().type != RP)
                {
                    SyntexError("Expect ')'");
                }
                stmt->stmt = Statement::parse(lex);
                break;
            default:
                SyntexError("Unexpected token");
                break;
        }
        return stmt;
    }
};
class JumpStatement : public SyntaxNode
{
    typedef enum JumpType { JMP_GOTO, JMP_CONTINUE, JMP_BREAK, JMP_RETURN } JumpType;

    JumpType type;
    SyntaxNode * expr;
    int id; // for goto Label
public:
    static SyntaxNode * parse(Lexer &lex)
    {
        JumpStatement stmt = new JumpStatement();
        stmt->expr = nullptr;
        stmt->id = 0;
        switch (lex.getNext().type)
        {
            case GOTO:
                stmt->type = JMP_GOTO;
                SyntexError("Unsupported feature");
                break;
            case CONTINUE:
                stmt->type = JMP_CONTINUE;
                if (lex.getNext().type != STMT_END)
                {
                    SyntexError("Expect ';'");
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
                SyntexError("Unexpected token");
                break;
        }
        return stmt;
    }
};

class Expression : public SyntaxNode
{
    vector<AssignExpression *> exprs;
public:
    static SyntaxNode * parse(Lexer &lex)
    {
        Expression * e = new Expression();
        bool first = true, finish = false;
        while (!finish)
        {
            if (!first && lex.getNext().type != OP_COMMA)
            {
                SyntexError("Expect ','");
            }
            switch (lex.peakNext().type)
            {
                // FIRST(expr) = ++ -- unary-op sizeof id constant string (
                case OP_INC: case OP_DEC:
                case BIT_AND: case BIT_NOT: case BOOL_NOT:
                case OP_MUL: case OP_ADD: case OP_DEC:
                case SIZEOF: case SYMBOL:
                case CONST_INT: case CONST_CHAR: case CONST_FLOAT:
                case CONST_ENUM: case STRING: case LP:
                    e->exprs.push_back(AssignExpression::parse(lex));
                    break;
                default:
                    finish = true;
                    break;
            }
            first = false;
        }
        return e;
    }
};
class AssignExpression : public SyntaxNode
{
    vector<UnaryExpression *> targets;
    CondExpression * source;
public:
    static SyntaxNode * parse(Lexer &lex)
    {
        // condition or unary?
        // if '(' type-name -> must be condition
        // else just be unary, check assign-op

        // if unary
        //    if LP+typename -> condition
        //    otherwise unary
        // else
        //    condition
        if (lex.peakNext().type == LP
         && Specifier::MaybeTypeName(lex.peakNext(1).type))
        {
            // condition & break
        }
        else
        {
            // unary + assign-op & contineu
        }
    }
};
class CondExpression : public SyntaxNode
{
public:
    static SyntaxNode * parse(Lexer &lex)
    {
    }
};
class OrExpression : public SyntaxNode
{
public:
    static SyntaxNode * parse(Lexer &lex)
    {
    }
};
class AndExpression : public SyntaxNode
{
public:
    static SyntaxNode * parse(Lexer &lex)
    {
    }
};
class BitOrExpression : public SyntaxNode
{
public:
    static SyntaxNode * parse(Lexer &lex)
    {
    }
};
class BitXorExpression : public SyntaxNode
{
public:
    static SyntaxNode * parse(Lexer &lex)
    {
    }
};
class BitAndExpression : public SyntaxNode
{
public:
    static SyntaxNode * parse(Lexer &lex)
    {
    }
};
class EqExpression : public SyntaxNode
{
public:
    static SyntaxNode * parse(Lexer &lex)
    {
    }
};
class RelExpression : public SyntaxNode
{
public:
    static SyntaxNode * parse(Lexer &lex)
    {
    }
};
class ShiftExpression : public SyntaxNode
{
public:
    static SyntaxNode * parse(Lexer &lex)
    {
    }
};
class AddExpression : public SyntaxNode
{
public:
    static SyntaxNode * parse(Lexer &lex)
    {
    }
};
class MulExpression : public SyntaxNode
{
public:
    static SyntaxNode * parse(Lexer &lex)
    {
    }
};
class CastExpression : public SyntaxNode
{
public:
    static SyntaxNode * parse(Lexer &lex)
    {
    }
};
class UnaryExpression : public SyntaxNode
{
public:
    static SyntaxNode * parse(Lexer &lex)
    {
    }
};
class PostfixExpression : public SyntaxNode
{
public:
    static SyntaxNode * parse(Lexer &lex)
    {
    }
};
class PrimaryExpression : public SyntaxNode
{
public:
    static SyntaxNode * parse(Lexer &lex)
    {
    }
};

class Parser
{
};
