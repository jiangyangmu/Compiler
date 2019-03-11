#pragma once

namespace x64 {

// x64 Program
// 1. add global object
// 2. add proc
class X64Program
{
public:
    void AddObject(StringRef label, size_t size, bool isZeroInitialized, void * initValue);
    void AddProc(StringRef label, int type, StringRef *code);
};

void AddFunction(X64Program * program,
                 const Language::FunctionDefinition & funcDef);
void AddObject(X64Program * program,
               const Language::ObjectDefinition & objDef);
struct ConstantContext;
void AddConstant(X64Program * program,
                 ConstantContext * constantContext);

// data locator

// string/float => label
struct ConstantContext
{
    std::map<int, StringRef> stringHashToLabel;
    std::map<int, StringRef> floatHashToLabel;
};

Location LocateString(ConstantContext * context,
                      StringRef strValue);
Location LocateFloat(ConstantContext * context,
                     float fltValue);

// {interface, offset, type} => Location
Location LocateParameter(Language::FunctionType * funcType,
                         size_t paramIndex,
                         Type * paramType);



// Statement / Expression IR

// build statements (handle jump destination linkage)
// AST -> IR Tree
// context: break point, continue point, current switch, IR tree
// ExprStmt(IRNode * expr)
// BeginIf, IfExpr, IfBody , ElseBody, EndIf
// BeginWhile, WhileExpr, WhileBody, EndWhile
// BeginDoWhile, DoWhileBody, DoWhileExpr, EndDoWhile
// BeginFor, ForInitExpr, ForCondExpr, ForEndExpr, EndFor
// Break / Continue / Goto / Return
// BeginSwitch, SwitchExpr, SwitchCase(value), SwitchDefault, SwitchBody, SwitchEnd

struct IRNode;


struct StatementContext
{
    std::vector<IRNode *> breakPoint;
    std::vector<IRNode *> continuePoint;
    std::vector<IRNode *> currentSwitch;
    std::vector<IRNode *> buildStack;
};

void PushStatement(StatementContext * context, IRNode * statement);
IRNode * PopStatement(StatementContext * context);

// build expressions (handle refine, storage locate)
}