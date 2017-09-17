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
    IR_TYPE_data_block, // data block
    IR_TYPE_routine,
};

// s/EIRAddressMode/IRAddressMode
enum EIRAddressMode
{
    OP_ADDR_invalid,
    OP_ADDR_mem,
    OP_ADDR_imm,
    OP_ADDR_label,
    // OP_ADDR_extern,
};

struct IRAddress
{
    EIRAddressMode mode;
    uint64_t value;  // used to store value when mode == imm
};

struct IRObject
{
    IRAddress addr;
    IRType type;

    size_t size;
    size_t align;
    void *value;  // uninitialized if: value == nullptr

    size_t element_size;  // used by array, size % element_size == 0

    std::string toString() const;
};

struct IROperation
{
    EOperationType op;
    IRAddress arg1, arg2, arg3;

    std::string toString() const;
};

typedef size_t IRObjectHandle;
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

    uint64_t __aligned_alloc(uint64_t &alloc, size_t size, size_t align)
    {
        alloc += size;
        if (alloc % align != 0)
            alloc += align - (alloc % align);
        return alloc;
    }

   public:
    IRStorage() : _alloc(0), _unnamed_alloc(0), _unnamed_alloc_max(0) {}

    // location: global, local
    // addr.mode indicates where to put
    // imm: if float or string: act as label
    // label:
    // mem: change _alloc or _tmp_alloc
    IRObjectHandle put(IRObject object);
    IRObjectHandle putWithName(IRObject object, StringRef label, bool merge);
    IRAddress find(IRObjectHandle h);
    IRAddress findByName(StringRef label);

    // code for local storage allocation
    std::vector<IROperation> generateCode() const;

    // Stringable
    virtual std::string toString() const;
};

struct Token;
struct Symbol;
class Type;

class IRObjectBuilder
{
   public:
    // temporary
    static IRObject FromType(const Type *type);
    // constant
    static IRObject FromTokenWithType(const Token *token, const Type *type);
    // named object
    static IRObject FromSymbol(const Symbol *symbol);
};

class IRUtil
{
   public:
    static std::string OperationTypeToString(EOperationType type);
};
