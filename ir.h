#pragma once

#include <string>
#include <vector>

#include "common.h"

enum EOperationType
{
    // storage
    OP_TYPE_alloc,
    OP_TYPE_free,

    // statement
    OP_TYPE_cmp,
    OP_TYPE_jmp,
    OP_TYPE_je,
    OP_TYPE_jl,
    OP_TYPE_jle,
    OP_TYPE_jg,
    OP_TYPE_jge,
    OP_TYPE_jb,
    OP_TYPE_jbe,
    OP_TYPE_ja,
    OP_TYPE_jae,

    // expression
    OP_TYPE_mov,
    OP_TYPE_or,
    OP_TYPE_xor,
    OP_TYPE_and,
    OP_TYPE_not,  // bit-wise
    OP_TYPE_shl,
    OP_TYPE_shr,  // shift
    OP_TYPE_add,
    OP_TYPE_sub,
    OP_TYPE_mul,
    OP_TYPE_div,
    OP_TYPE_mod,
    OP_TYPE_inc,
    OP_TYPE_dec,
    OP_TYPE_neg,
    OP_TYPE_ref,
    OP_TYPE_deref,

    // routine
    OP_TYPE_param,
    OP_TYPE_call,
    OP_TYPE_ret,

    // FPU
    OP_TYPE_fld,
    OP_TYPE_fst,  // load/store
    OP_TYPE_fabs,
    OP_TYPE_fchs,  // change sign
    OP_TYPE_fadd,
    OP_TYPE_fsub,
    OP_TYPE_fmul,
    OP_TYPE_fdiv,
};

enum EIRAddressMode
{
    OP_ADDR_invalid,
    OP_ADDR_reg,
    OP_ADDR_imm,
};

struct IRAddress
{
    EIRAddressMode mode;
    uint64_t value;
};

// TODO: Stringable
// s/Operation/IROperation/
struct Operation
{
    EOperationType op;
    IRAddress arg1, arg2, arg3;

    std::string toString() const;
};

// storage management
class IRStorage : public Stringable
{
    std::vector<StringRef> _name;
    std::vector<IRAddress> _addr;
    uint64_t _alloc;

   public:
    IRStorage() : _alloc(0) {}

    void alloc(StringRef symbol, size_t size, size_t align,
               bool replace = false);
    IRAddress find(StringRef symbol) const;
    uint64_t allocatedSize() const;

    // Stringable
    virtual std::string toString() const;
};
// code management

class OperationUtil
{
   public:
    static std::string OperationTypeToString(EOperationType type);
};

// class OperationBlock
// {
//    public:
//     typedef size_t op_handle_t;

//    public:
//     //  add(arith_op, src_name, dst_name)
//     //  add(arith_op, src1_name, src2_name, dst_name)
//     //  add(jmp_op, dst_label)
//     //
//     //  add(jmp_op) -> op_handle_t
//     //  fix(op_handle_t, dst_label)
//     //
//     data_address_t getNextAddress();
//     op_handle_t addOperation(EOperationType t, data_address_t arg1 = 0,
//                              data_address_t arg2 = 0, data_address_t arg3 =
//                              0);
//     void fixOperationArgument(op_handle_t op, data_address_t addr);
// };
