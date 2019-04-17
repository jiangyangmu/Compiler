#pragma once

#include "Location.h"
#include "Definition.h"
#include "../util/Integer.h"

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
    EXPR_ID, EXPR_CONSTANT,

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
    EXPR_SINEG,

    EXPR_IINC, EXPR_IDEC,

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

struct ConstantContext
{
    struct StringConstant
    {
        StringRef label; // Label to pointer-to-string object. TODO: remove this hack once EXPR_PVAL works
        StringRef stringLabel;
        StringRef stringValue;
    };
    struct FloatConstant
    {
        StringRef label;
        float value;
    };

    int nextStringLabel;
    int nextFloatLabel;

    std::map<int, StringConstant> hashToStringConstant;
    std::map<int, FloatConstant> hashToFloatConstant;
};

ConstantContext *   CreateConstantContext();

void                LocateString(ConstantContext * context,
                                 StringRef strValue,
                                 Location * strLocation,
                                 Location * strPtrLocation);
Location            LocateFloat(ConstantContext * context, float fltValue);

enum ExpressionIntention
{
    WANT_VALUE,
    WANT_ADDRESS,
    WANT_NOTHING,
};

#define R12_MASK (1)
#define R13_MASK (1 << 1)
#define R14_MASK (1 << 2)
#define R15_MASK (1 << 3)
#define RDI_MASK (1 << 4)
#define RSI_MASK (1 << 5)
#define RBX_MASK (1 << 6)
#define RBP_MASK (1 << 7)
#define RSP_MASK (1 << 8)
#define XMM6_MASK (1 << 9)
#define XMM7_MASK (1 << 10)
#define XMM8_MASK (1 << 11)
#define XMM9_MASK (1 << 12)
#define XMM10_MASK (1 << 13)
#define XMM11_MASK (1 << 14)
#define XMM12_MASK (1 << 15)
#define XMM13_MASK (1 << 16)
#define XMM14_MASK (1 << 17)
#define XMM15_MASK (1 << 18)

struct FunctionContext
{
    // Shared build state

    DefinitionContext * functionDefinitionContext;
    std::vector<DefinitionContext *> currentDefinitionContext;

    FunctionType * functionType; // used in id expression, to figure out argument object location

    // Statement tree build state

    // label generation
    int nextUniqueLabel;

    // label check
    // check used goto label are defined
    std::map<std::string, bool> isLabelDefined;
    // check break/continue has target, case/default in switch
    std::vector<Node *> currentBreakTarget; // can be while*/for/switch
    std::vector<Node *> currentContinueTarget; // can be while*/for
    std::vector<Node *> currentSwitch; // can be switch

    // label binding
    // while/do-while/for   -> (continue label, break label)
    // switch               -> (default label, break label, case labels...)
    std::map<Node *, std::vector<std::string>> targetToLabels;
    // continue     -> (while/do-while/for, 0)
    // break        -> (while/do-while/for/switch, 1)
    // case         -> (switch, 2...n)
    // default      -> (switch, 0)
    std::map<Node *, std::pair<Node *, int>> nodeToTarget;
    // switch       -> (default, case...)
    std::map<Node *, std::vector<Node *>> switchToChildren;

    // Expression tree build state

    ConstantContext * constantContext;
    TypeContext * typeContext;
    // add in pre-order AST traversal.
    std::vector<ExpressionIntention> currentIntention;

    // Code generation state

    std::string functionName;
    
    // stack allocation
    // layout:
    //      returnAddress                   (8-byte aligned)
    //      saved rbp                       (8-byte aligned) <-- rbp
    //      nonVolatileReg                  (8-byte aligned)
    //      localZoneSize
    //      gap                             (8-byte aligned)
    //      maxTempZoneSize
    //      gap                             (8-byte aligned)
    //      maxSpillZoneSize                (8-byte aligned)
    //      gap
    //      maxCallZoneSize                 (16-byte aligned) <-- rsp
    // allocation zone
    size_t localZoneSize;
    size_t maxTempZoneSize;
    size_t maxSpillZoneSize;
    size_t maxCallZoneSize;
    size_t stackAllocSize; // local zone + temp zone + spill zone + call zone + gaps
    size_t stackFrameSize;
    // allocation offset - "offset to previous stack frame"
    size_t registerZoneEndOffset;   // for debug
    size_t localZoneEndOffset;      // for debug
    size_t tempZoneBeginOffset;     // for temp var location computation
    size_t spillZoneEndOffset;      // needed by some expression node (e.g. EXPR_PVAL, EXPR_CVT_F2B)
    size_t callZoneEndOffset;
    std::map<Definition *, size_t> localObjectOffsets;

    // non-volatile register mask (also used in stack allocation)
    size_t dirtyRegisters;

    // Final tree

    Node * functionBody;
};

FunctionContext * CreateFunctionContext(DefinitionContext * functionDefinitionContext,
                                        FunctionType * functionType,
                                        ConstantContext * constantContext,
                                        TypeContext * typeContext,
                                        StringRef functionName);

Location GetSpillLocation(FunctionContext * functionContext, size_t i);

// allocate stack, fill locals/temps
void    PrepareStack(FunctionContext * context);

bool    IsRegisterNode(Node * node);

// Debug

std::string NodeDebugString(Node * node);
// after PrepareStack
std::string StackLayoutDebugString(FunctionContext * context);
void PrintFunctionContext(FunctionContext * context);

// Build

void    AddChild(Node * parent, Node * child);
Node *  LastChild(Node * parent);

// Expression construction front end, fit C syntax
// Construct tree base on intention, check children type, refer self type, insert cast node.
// Eliminate enum, treat them as constant node. (sizeof need delete node tree)

Node * EmptyExpression(FunctionContext * context);

Node * IdExpression(FunctionContext * context, StringRef id);
Node * ConstantExpression(FunctionContext * context, int value);
Node * ConstantExpression(FunctionContext * context, size_t value);
Node * ConstantExpression(FunctionContext * context, float value);
Node * ConstantExpression(FunctionContext * context, StringRef value);

Node * IncExpression(FunctionContext * context, Node * expr);
Node * DecExpression(FunctionContext * context, Node * expr);
Node * PostIncExpression(FunctionContext * context, Node * expr);
Node * PostDecExpression(FunctionContext * context, Node * expr);

Node * MemberOfExpression(FunctionContext * context, Node * structOrUnion, StringRef memberName);
Node * IndirectMemberOfExpression(FunctionContext * context, Node * pointerToStructOrUnion, StringRef memberName);

Node * CallExpression(FunctionContext * context, Node * func, std::vector<Node *> params);

Node * SubscriptExpression(FunctionContext * context, Node * a, Node * b);

Node * GetAddressExpression(FunctionContext * context, Node * expr);
Node * IndirectExpression(FunctionContext * context, Node * expr);

Node * PositiveExpression(FunctionContext * context, Node * expr);
Node * NegativeExpression(FunctionContext * context, Node * expr);

Node * SizeOfExpression(FunctionContext * context, Node * expr);

Node * CastExpression(Node * expr, Type * type);

Node * AddExpression(FunctionContext * context, Node * a, Node * b);
Node * SubExpression(FunctionContext * context, Node * a, Node * b);
Node * MulExpression(FunctionContext * context, Node * a, Node * b);
Node * DivExpression(FunctionContext * context, Node * a, Node * b);
Node * ModExpression(FunctionContext * context, Node * a, Node * b);

Node * BitNotExpression(FunctionContext * context, Node * expr);
Node * BitAndExpression(FunctionContext * context, Node * a, Node * b);
Node * BitXorExpression(FunctionContext * context, Node * a, Node * b);
Node * BitOrExpression(FunctionContext * context, Node * a, Node * b);
Node * ShiftLeftExpression(FunctionContext * context, Node * a, Node * b);
Node * ShiftRightExpression(FunctionContext * context, Node * a, Node * b);

Node * BoolNotExpression(FunctionContext * context, Node * expr);
Node * BoolAndExpression(FunctionContext * context, Node * a, Node * b);
Node * BoolOrExpression(FunctionContext * context, Node * a, Node * b);

Node * EqExpression(FunctionContext * context, Node * a, Node * b);
Node * NeExpression(FunctionContext * context, Node * a, Node * b);
Node * LtExpression(FunctionContext * context, Node * a, Node * b);
Node * LeExpression(FunctionContext * context, Node * a, Node * b);
Node * GeExpression(FunctionContext * context, Node * a, Node * b);
Node * GtExpression(FunctionContext * context, Node * a, Node * b);

Node * AssignExpression(FunctionContext * context, Node * a, Node * b);
Node * ConditionExpression(FunctionContext * context, Node * a, Node * b, Node * c);
Node * CommaExpression(std::vector<Node *> & exprs);

// Statement construction front end, fit C syntax
// Construct flow-control tree, check and create labels (goto label defined, break/continue has target, case/default in switch).

void   Function_Begin(FunctionContext * context);
void   Function_End(FunctionContext * context);

Node * CompoundStatement_Begin(FunctionContext * context, DefinitionContext * definitionContext);
void   CompoundStatement_AddStatement(Node * compoundStmt, Node * stmt);
void   CompoundStatement_End(FunctionContext * context);

Node * ExpressionStatement(Node * expr);

Node * ReturnStatement(Node * expr);

Node * IfStatement_Begin();
void   IfStatement_SetExpr(Node * ifStmt, Node * expr);
void   IfStatement_SetIfBody(Node * ifStmt, Node * ifBody);
void   IfStatement_SetElseBody(Node * ifStmt, Node * elseBody);
void   IfStatement_End(Node * ifStmt);

Node * WhileStatement_Begin(FunctionContext * context);
void   WhileStatement_SetExpr(Node * whileStmt, Node * expr);
void   WhileStatement_SetBody(Node * whileStmt, Node * body);
void   WhileStatement_End(FunctionContext * context);

Node * DoWhileStatement_Begin(FunctionContext * context);
void   DoWhileStatement_SetBody(Node * doWhileStmt, Node * body);
void   DoWhileStatement_SetExpr(Node * doWhileStmt, Node * expr);
void   DoWhileStatement_End(FunctionContext * context);

Node * ForStatement_Begin(FunctionContext * context);
void   ForStatement_SetPreExpr(Node * forStmt, Node * preExpr);
void   ForStatement_SetLoopExpr(Node * forStmt, Node * loopExpr);
void   ForStatement_SetPostExpr(Node * forStmt, Node * postExpr);
void   ForStatement_SetBody(Node * forStmt, Node * body);
void   ForStatement_End(FunctionContext * context);

Node * GotoStatement(FunctionContext * context, StringRef label);
Node * LabelStatement(FunctionContext * context, StringRef label);

Node * BreakStatement(FunctionContext * context);
Node * ContinueStatement(FunctionContext * context);

Node * SwitchStatement_Begin(FunctionContext * context);
void   SwitchStatement_SetExpr(Node * switchStmt, Node * expr);
void   SwitchStatement_SetBody(Node * switchStmt, Node * body);
void   SwitchStatement_End(FunctionContext * context);

Node * CaseStatement(FunctionContext * context, u64 caseValue);
Node * DefaultStatement(FunctionContext * context);

}