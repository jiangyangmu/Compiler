#pragma once

#include <string>
#include <vector>

#include "common.h"

// s/Operation/Instruction

// s/EOperationType/IROperationType
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

enum IRType
{
    IR_TYPE_invalid,
    IR_TYPE_int,
    IR_TYPE_uint,
    IR_TYPE_float,
    IR_TYPE_array,
    IR_TYPE_string,
};

// s/EIRAddressMode/IRAddressMode
enum EIRAddressMode
{
    OP_ADDR_invalid,
    OP_ADDR_mem,
    OP_ADDR_imm,
    OP_ADDR_label,
};

struct IRAddress
{
    EIRAddressMode mode;
    uint64_t value;
};

struct IRObject
{
    IRAddress addr;
    IRType type;

    size_t size;
    size_t align;
    void *value; // uninitialized if: value == nullptr

    size_t element_size; // used by array, size % element_size == 0

    std::string toString() const;
};

struct IRConstant
{
    TokenType type;
    union {
        float fval;
        StringRef sval;
    };
};

struct IROperation
{
    EOperationType op;
    IRAddress arg1, arg2, arg3;

    std::string toString() const;
};

// storage management
// 1. named: global, local
// 2. un-named: temporary
// 3. constant
class IRStorage : public Stringable
{
    std::vector<StringRef> _name;
    std::vector<IRObject> _obj;
    // named local
    uint64_t _alloc;

    std::vector<IRObject> _unnamed_obj;
    uint64_t _unnamed_alloc, _unnamed_alloc_max;

    // std::vector<float> _flt_const;
    // std::vector<StringRef> _str_const;

    uint64_t __aligned_alloc(uint64_t &alloc, size_t size, size_t align)
    {
        alloc += size;
        if (alloc % align != 0)
            alloc += align - (alloc % align);
        return alloc;
    }

   public:
    IRStorage() : _alloc(0) {}

    // named
    // TODO: don't use replace, let Environment has a callback to iterate objects
    IRAddress alloc(StringRef label, IRObject object, bool replace);
    IRAddress find(StringRef label) const;

    // un-named
    IRAddress allocUnnamed(IRObject object);
    void freeAllUnnamed();

    // TODO: constant pool (float, string)
    // IRAddress alloc(IRConstant c);

    // code for storage allocation
    std::vector<IROperation> generateCode() const;

    // Stringable
    virtual std::string toString() const;
};
// code management

class Type;

class IRUtil
{
   public:
    static IRObject TypeToIRObject(const Type *type, ESymbolLinkage linkage);
    static std::string OperationTypeToString(EOperationType type);
};
