#pragma once

#include "Location.h"
#include "Definition.h"
#include "../Util/Integer.h"

namespace Language {

enum NodeType
{
    UNKNOWN_IR_NODE,

    BEGIN_STATEMENT,

    STMT_COMPOUND,
    STMT_EXPRESSION,
    STMT_IF,
    STMT_WHILE,
    STMT_DO_WHILE,
    STMT_FOR,
    STMT_BREAK,
    STMT_CONTINUE,
    STMT_RETURN,
    STMT_GOTO,
    STMT_LABEL,
    STMT_SWITCH,
    STMT_CASE,
    STMT_DEFAULT,

    END_STATEMENT,
    BEGIN_EXPRESSION,

    // data node
    EXPR_DATA,

    // function call
    EXPR_CALL,

    // type casting operators (change type, size, sign)
    EXPR_CVT_SI2SI, EXPR_CVT_SI2UI, EXPR_CVT_UI2SI, EXPR_CVT_UI2UI,
    EXPR_CVT_F2F,
    EXPR_CVT_SI2F, EXPR_CVT_F2SI,
    EXPR_CVT_I2B, EXPR_CVT_B2I, EXPR_CVT_F2B, EXPR_CVT_B2F,

    EXPR_CVT_REINTERP,
    EXPR_CVT_DECAY,

    // bool operators (bool x bool)
    EXPR_BOOL_NOT, EXPR_BOOL_AND, EXPR_BOOL_OR,

    // integer operators (int x int, same size, same sign)
    EXPR_IADD, EXPR_ISUB,
    EXPR_IMUL, EXPR_IDIV, EXPR_IMOD,

    EXPR_INOT,
    EXPR_IAND, EXPR_IOR, EXPR_IXOR,
    
    EXPR_ISHL, EXPR_ISHR,
    
    EXPR_IEQ, EXPR_INE, EXPR_ILT, EXPR_ILE, EXPR_IGE, EXPR_IGT,

    // float operators (float x float, same size)
    EXPR_FNEG, EXPR_FADD, EXPR_FSUB, EXPR_FMUL, EXPR_FDIV,
    EXPR_FEQ, EXPR_FNE, EXPR_FLT, EXPR_FLE, EXPR_FGE, EXPR_FGT,

    // pointer arith operators (pointer x integer, same size)
    EXPR_PADDSI, EXPR_PSUBSI,
    EXPR_PADDUI, EXPR_PSUBUI,

    // pointer operators (pointer x pointer, same size)
    EXPR_PDIFF,

    EXPR_PEQ, EXPR_PNE, EXPR_PLT, EXPR_PLE, EXPR_PGE, EXPR_PGT,

    EXPR_PIND,
    EXPR_PNEW,

    // memory operators (size)
    EXPR_MCOPY,
    EXPR_MDUP,

    EXPR_MADDSI, EXPR_MADDUI,

    // TODO: divide
    EXPR_CONDITION,
    // expression list (TODO: divide)
    EXPR_ELIST,

    END_EXPRESSION,

    EMPTY_EXPRESSION, // used by return/for/expr statements
};

struct Node
{
    Node * down, * right, * up;

    NodeType type;

    union
    {
        DefinitionContext * context;    // for compound statement
        StringRef * label;              // for goto & label statement
        u64 caseValue;                  // for case statement
    } stmt;

    struct
    {
        Type * type;
        Location loc;
    } expr;
};

// Debug

std::string NodeDebugString(Node * node);
// Build

void    AddChild(Node * parent, Node * child);
int     CountChild(Node * node);
Node *  LastChild(Node * parent);
Node *  MakeNode(NodeType type);
void    DestroyNodeTree(Node * root);

}