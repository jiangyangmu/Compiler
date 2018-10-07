#pragma once

#include "expr.h"
#include "stmt.h"
// 1. Representation of instruction + storage

// Data
//      int
//      float
//      data-block
//
// Instruction
//      icode <op> <op>
//
// Routine
//      label
//      instruction[]
//

enum irEncoding {
    IR_INT,
    IR_UINT,
    IR_FLOAT,
    IR_MEM,
};
struct irData {
    size_t mem_size;
    size_t mem_align;
    irEncoding encoding;
};

struct irInstruction {};
struct irRoutine {
    StringRef label;
    std::vector<irInstruction> instructions;
};

irData SymbolToIRData(Symbol * symbol);
irRoutine SymbolToIRRoutine(Symbol * symbol);

// Expression tree -> IR instructions
// Statement tree -> IR instructions
class irRoutineBuilder {
public:
    enum EvalType { VOID, VALU, ADDR };

    // statement
    void EVAL(Stmt * stmt);

    // expression
    void EVAL(ExprNode * expr, EvalType et = EvalType::VOID);
};

void eval_if_stmt(IfStmt * if_stmt) {
    // eval_expr(if_stmt->cond_expr);
    //
    // emit(TEST());
    // jmp_if_false = emit(JMP_IF_FALSE(0));
    //
    // eval_stmt(if_stmt->if_stmt);
    // jmp = emit(JMP(0));
    //
    // fill_jmp(jmp_if_false, ip());
    // eval_stmt(if_stmt->else_stmt);
    //
    // fill_jmp(jmp, ip());
}
void eval_switch_stmt(SwitchStmt * switch_stmt) {
    // eval_expr(switch_stmt->cond_expr);
    //
    // cases = find_free_cases(switch_stmt->stmt);
    // for (case : cases)
    //      emit(CMP())
    //      emit(JE())
    // defalut = find_free_default(switch_stmt->stmt);
    // if (default)
    //      emit(JMP())
    // emit(JMP())
    //
    // eval_stmt(switch_stmt->stmt);
}

// 2. Mapping to x64 assembly
// check: https://gcc.godbolt.org/
