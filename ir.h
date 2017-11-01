#pragma once

#include "symbol.h"

#include <list>
#include <string>
#include <vector>

enum EIRAddressMode
{
    OP_ADDR_invalid,
    OP_ADDR_mem,
    OP_ADDR_imm,
    OP_ADDR_label,
};
// all information needed to generate instruction
struct IRAddress
{
    EIRAddressMode mode;
    size_t width;
    union {
        uint64_t mem;
        uint64_t imm;
        StringRef *label;
    };

    std::string toString() const;

    static IRAddress imm_0()
    {
        return {OP_ADDR_imm, sizeof(int), {0}};
    }
    static IRAddress imm_1()
    {
        return {OP_ADDR_imm, sizeof(int), {1}};
    }
    static IRAddress imm_2()
    {
        return {OP_ADDR_imm, sizeof(int), {2}};
    }
    static IRAddress FromLabel(StringRef *l)
    {
        IRAddress addr = {OP_ADDR_label, sizeof(void *), {}};
        addr.label = l;
        return addr;
    }
};

enum EIROpcode
{
    IR_OPCODE_invalid,

    // storage
    IR_OPCODE_alloc,
    IR_OPCODE_free,

    // statement
    IR_OPCODE_jmp,
    IR_OPCODE_je,
    IR_OPCODE_jne,
    IR_OPCODE_jl,
    IR_OPCODE_jle,
    IR_OPCODE_jg,
    IR_OPCODE_jge,
    IR_OPCODE_jb,
    IR_OPCODE_jbe,
    IR_OPCODE_ja,
    IR_OPCODE_jae,

    // routine
    IR_OPCODE_param,
    IR_OPCODE_call,
    IR_OPCODE_ret,

    // pointer
    IR_OPCODE_ref,
    IR_OPCODE_deref,

    // integer operands
    IR_OPCODE_mov,
    IR_OPCODE_cmp,
    // bit-wise
    IR_OPCODE_or,
    IR_OPCODE_xor,
    IR_OPCODE_and,
    IR_OPCODE_not,
    IR_OPCODE_shl,
    IR_OPCODE_shr,
    IR_OPCODE_sal,  // signed shift left
    IR_OPCODE_sar,  // signed shift right
    // arithmetic
    IR_OPCODE_add,
    IR_OPCODE_sub,
    IR_OPCODE_mul,
    IR_OPCODE_div,
    IR_OPCODE_mod,
    IR_OPCODE_inc,
    IR_OPCODE_dec,
    // signedness
    IR_OPCODE_neg,

    // integer conversion
    // IR_OPCODE_s2u,
    // IR_OPCODE_u2s,
    IR_OPCODE_sx,    // signed extend
    IR_OPCODE_zx,    // unsigned extend
    IR_OPCODE_shrk,  // shrink

    // FPU
    IR_OPCODE_fld,
    IR_OPCODE_fst,  // load/store
    IR_OPCODE_fabs,
    IR_OPCODE_fchs,  // change sign
    IR_OPCODE_fadd,
    IR_OPCODE_fsub,
    IR_OPCODE_fmul,
    IR_OPCODE_fdiv,
    // floating-point conversion
    IR_OPCODE_fext,   // extend
    IR_OPCODE_fshrk,  // shrink
    IR_OPCODE_f2i,
    IR_OPCODE_i2f,
};
struct IRInstruction
{
    EIROpcode op;
    IRAddress arg1, arg2, arg3;
    StringRef *prelabel, *postlabel;  // can be jump destination

    std::string toString() const;
};
class IRInstructionBuilder
{
   public:
    static IRInstruction Cmp(IRAddress arg1, IRAddress arg2,
                             StringRef *begin = nullptr,
                             StringRef *end = nullptr)
    {
        return {IR_OPCODE_cmp, arg1, arg2, {}, begin, end};
    }
    static IRInstruction Mov(IRAddress arg1, IRAddress arg2,
                             StringRef *begin = nullptr,
                             StringRef *end = nullptr)
    {
        assert(arg1.mode != OP_ADDR_invalid && arg2.mode != OP_ADDR_invalid);
        return {IR_OPCODE_mov, arg1, arg2, {}, begin, end};
    }
    static IRInstruction Je(IRAddress arg1, StringRef *begin = nullptr,
                            StringRef *end = nullptr)
    {
        return {IR_OPCODE_je, arg1, {}, {}, begin, end};
    }
    static IRInstruction Jne(IRAddress arg1, StringRef *begin = nullptr,
                             StringRef *end = nullptr)
    {
        return {IR_OPCODE_jne, arg1, {}, {}, begin, end};
    }
    static IRInstruction Jl(IRAddress arg1, StringRef *begin = nullptr,
                            StringRef *end = nullptr)
    {
        return {IR_OPCODE_jl, arg1, {}, {}, begin, end};
    }
    static IRInstruction Jle(IRAddress arg1, StringRef *begin = nullptr,
                             StringRef *end = nullptr)
    {
        return {IR_OPCODE_jle, arg1, {}, {}, begin, end};
    }
    static IRInstruction Jg(IRAddress arg1, StringRef *begin = nullptr,
                            StringRef *end = nullptr)
    {
        return {IR_OPCODE_jg, arg1, {}, {}, begin, end};
    }
    static IRInstruction Jge(IRAddress arg1, StringRef *begin = nullptr,
                             StringRef *end = nullptr)
    {
        return {IR_OPCODE_jge, arg1, {}, {}, begin, end};
    }
    static IRInstruction Jb(IRAddress arg1, StringRef *begin = nullptr,
                            StringRef *end = nullptr)
    {
        return {IR_OPCODE_jb, arg1, {}, {}, begin, end};
    }
    static IRInstruction Jbe(IRAddress arg1, StringRef *begin = nullptr,
                             StringRef *end = nullptr)
    {
        return {IR_OPCODE_jbe, arg1, {}, {}, begin, end};
    }
    static IRInstruction Ja(IRAddress arg1, StringRef *begin = nullptr,
                            StringRef *end = nullptr)
    {
        return {IR_OPCODE_ja, arg1, {}, {}, begin, end};
    }
    static IRInstruction Jae(IRAddress arg1, StringRef *begin = nullptr,
                             StringRef *end = nullptr)
    {
        return {IR_OPCODE_jae, arg1, {}, {}, begin, end};
    }
    static IRInstruction Jmp(IRAddress arg1, StringRef *begin = nullptr,
                             StringRef *end = nullptr)
    {
        return {IR_OPCODE_jmp, arg1, {}, {}, begin, end};
    }
    static IRInstruction Inc(IRAddress arg1, StringRef *begin = nullptr,
                             StringRef *end = nullptr)
    {
        return {IR_OPCODE_inc, arg1, {}, {}, begin, end};
    }
    static IRInstruction Dec(IRAddress arg1, StringRef *begin = nullptr,
                             StringRef *end = nullptr)
    {
        return {IR_OPCODE_dec, arg1, {}, {}, begin, end};
    }
    static IRInstruction Neg(IRAddress arg1, StringRef *begin = nullptr,
                             StringRef *end = nullptr)
    {
        return {IR_OPCODE_neg, arg1, {}, {}, begin, end};
    }
    static IRInstruction Or(IRAddress arg1, IRAddress arg2, IRAddress arg3,
                            StringRef *begin = nullptr,
                            StringRef *end = nullptr)
    {
        return {IR_OPCODE_or, arg1, arg2, arg3, begin, end};
    }
    static IRInstruction Xor(IRAddress arg1, IRAddress arg2, IRAddress arg3,
                             StringRef *begin = nullptr,
                             StringRef *end = nullptr)
    {
        return {IR_OPCODE_xor, arg1, arg2, arg3, begin, end};
    }
    static IRInstruction And(IRAddress arg1, IRAddress arg2, IRAddress arg3,
                             StringRef *begin = nullptr,
                             StringRef *end = nullptr)
    {
        return {IR_OPCODE_and, arg1, arg2, arg3, begin, end};
    }
    static IRInstruction Shl(IRAddress arg1, IRAddress arg2, IRAddress arg3,
                             StringRef *begin = nullptr,
                             StringRef *end = nullptr)
    {
        return {IR_OPCODE_shl, arg1, arg2, arg3, begin, end};
    }
    static IRInstruction Sal(IRAddress arg1, IRAddress arg2, IRAddress arg3,
                             StringRef *begin = nullptr,
                             StringRef *end = nullptr)
    {
        return {IR_OPCODE_shl, arg1, arg2, arg3, begin, end};
    }
    static IRInstruction Shr(IRAddress arg1, IRAddress arg2, IRAddress arg3,
                             StringRef *begin = nullptr,
                             StringRef *end = nullptr)
    {
        return {IR_OPCODE_shr, arg1, arg2, arg3, begin, end};
    }
    static IRInstruction Sar(IRAddress arg1, IRAddress arg2, IRAddress arg3,
                             StringRef *begin = nullptr,
                             StringRef *end = nullptr)
    {
        return {IR_OPCODE_sar, arg1, arg2, arg3, begin, end};
    }
    static IRInstruction Add(IRAddress arg1, IRAddress arg2, IRAddress arg3,
                             StringRef *begin = nullptr,
                             StringRef *end = nullptr)
    {
        return {IR_OPCODE_add, arg1, arg2, arg3, begin, end};
    }
    static IRInstruction Sub(IRAddress arg1, IRAddress arg2, IRAddress arg3,
                             StringRef *begin = nullptr,
                             StringRef *end = nullptr)
    {
        return {IR_OPCODE_sub, arg1, arg2, arg3, begin, end};
    }
    static IRInstruction Mul(IRAddress arg1, IRAddress arg2, IRAddress arg3,
                             StringRef *begin = nullptr,
                             StringRef *end = nullptr)
    {
        return {IR_OPCODE_mul, arg1, arg2, arg3, begin, end};
    }
    static IRInstruction Div(IRAddress arg1, IRAddress arg2, IRAddress arg3,
                             StringRef *begin = nullptr,
                             StringRef *end = nullptr)
    {
        return {IR_OPCODE_div, arg1, arg2, arg3, begin, end};
    }
    static IRInstruction Mod(IRAddress arg1, IRAddress arg2, IRAddress arg3,
                             StringRef *begin = nullptr,
                             StringRef *end = nullptr)
    {
        return {IR_OPCODE_mod, arg1, arg2, arg3, begin, end};
    }
    static IRInstruction SX(IRAddress arg1, IRAddress arg2,
                            StringRef *begin = nullptr,
                            StringRef *end = nullptr)
    {
        return {IR_OPCODE_sx, arg1, arg2, {}, begin, end};
    }
    static IRInstruction ZX(IRAddress arg1, IRAddress arg2,
                            StringRef *begin = nullptr,
                            StringRef *end = nullptr)
    {
        return {IR_OPCODE_zx, arg1, arg2, {}, begin, end};
    }
    static IRInstruction SHRK(IRAddress arg1, IRAddress arg2,
                              StringRef *begin = nullptr,
                              StringRef *end = nullptr)
    {
        return {IR_OPCODE_shrk, arg1, arg2, {}, begin, end};
    }
    static IRInstruction FEXT(IRAddress arg1, IRAddress arg2,
                              StringRef *begin = nullptr,
                              StringRef *end = nullptr)
    {
        return {IR_OPCODE_fext, arg1, arg2, {}, begin, end};
    }
    static IRInstruction FSHRK(IRAddress arg1, IRAddress arg2,
                               StringRef *begin = nullptr,
                               StringRef *end = nullptr)
    {
        return {IR_OPCODE_fshrk, arg1, arg2, {}, begin, end};
    }
    static IRInstruction F2I(IRAddress arg1, IRAddress arg2,
                             StringRef *begin = nullptr,
                             StringRef *end = nullptr)
    {
        return {IR_OPCODE_f2i, arg1, arg2, {}, begin, end};
    }
    static IRInstruction I2F(IRAddress arg1, IRAddress arg2,
                             StringRef *begin = nullptr,
                             StringRef *end = nullptr)
    {
        return {IR_OPCODE_i2f, arg1, arg2, {}, begin, end};
    }
    static IRInstruction Ref(IRAddress arg1, IRAddress arg2,
                             StringRef *begin = nullptr,
                             StringRef *end = nullptr)
    {
        return {IR_OPCODE_ref, arg1, arg2, {}, begin, end};
    }
    static IRInstruction Deref(IRAddress arg1, IRAddress arg2,
                               StringRef *begin = nullptr,
                               StringRef *end = nullptr)
    {
        return {IR_OPCODE_deref, arg1, arg2, {}, begin, end};
    }
    static IRInstruction Ret(IRAddress arg1, StringRef *begin = nullptr,
                             StringRef *end = nullptr)
    {
        return {IR_OPCODE_ret, arg1, {}, {}, begin, end};
    }
};
// Usage:
// IRCode code;
//
// IRInstruction mov = {IR_OPCODE_mov, addr_from, addr_to, {}, nullptr}
// IRInstruction add = {IR_OPCODE_add, arg1, arg2, result, nullptr}
// code.add(mov);
// code.add(add);
//

// How to compute ir linkage:
//
// 1. symbol => ir-linkage
//          [symbol-linkage]   [storage-specifier]    ir-linkage
// obj           unique         (= auto/register)       local
// obj/func     internal           (= static)           static
// obj/func     external             (= "")             static_export
// obj/func     external           (= extern)           extern
// 2. temporary => ir-linkage
//                                             local
// 3. lex constant => ir-linkage
//     int                                     none(in instruction)
//     float                                   static_const
//     string                                  static_const

// linkage means "where to store it?"
enum EIRLinkage
{
    IR_LINKAGE_invalid,
    IR_LINKAGE_inline,         // on instruction
    IR_LINKAGE_local,          // on stack (live in scope)
    IR_LINKAGE_static,         // on static area (live in translation_unit)
    IR_LINKAGE_static_const,   // on static area (live in translation_unit)
    IR_LINKAGE_static_export,  // on static area (live in program)
    IR_LINKAGE_extern,         // on other translation_unit
};
// just bytes, the meaning is represented by operation
struct IRValue
{
    size_t size, align;
    void *init;  // from initializer_list

    int asInt32() const
    {
        assert(size == 4 && align == 4);
        return *(int *)init;
    }
};

class IRCode;

//
// linkage     name    value(as bytes)     addr(auto)
//
struct IRObject
{
    EIRLinkage linkage;
    StringRef name;  // name == "" => isTemporary()
    IRValue value;
    IRAddress addr;

    IRCode *code;  // only used by function object

    std::string toString() const;
};
class IRValueFactory
{
    static std::vector<void *> _data;

   public:
    static IRValue CreateEmpty()
    {
        return {0, 0, nullptr};
    }
    static IRValue CreateZero(size_t size, size_t align)
    {
        assert(size > 0 && align > 0);

        char *data = new char[size];
        std::fill(data, data + size, 0);
        _data.push_back(data);

        return {size, align, (void *)data};
    }
    static IRValue Create(size_t size, size_t align, const void *data)
    {
        assert(size > 0 && align > 0 && data != nullptr);

        const char *ori = (const char *)data;
        char *cpy = new char[size];
        std::copy_n(ori, size, cpy);
        _data.push_back(cpy);

        return {size, align, (void *)cpy};
    }
    static IRValue CreateString(StringRef str)
    {
        size_t size = str.size() + 1;
        size_t align = 1;

        char *cpy = new char[size];
        std::copy_n(str.data(), str.size(), cpy);
        cpy[size - 1] = '\0';
        _data.push_back(cpy);

        return {size, align, (void *)cpy};
    }
};
class IRObjectBuilder
{
    ESymbolLinkage _symbol_linkage;
    TokenType _syntax_storage;
    StringRef _name;
    TokenType _lex_type;
    IRValue _value;
    IRCode *_code;

   public:
    IRObjectBuilder()
        : _symbol_linkage(SYMBOL_LINKAGE_invalid),
          _syntax_storage(NONE),
          _name(),
          _lex_type(NONE),
          _code(nullptr)
    {
        _value = IRValueFactory::CreateEmpty();
    }
    IRObjectBuilder &withSymbolLinkage(ESymbolLinkage linkage)
    {
        _symbol_linkage = linkage;
        return (*this);
    }
    IRObjectBuilder &withSyntaxStorage(TokenType storage)
    {
        _syntax_storage = storage;
        return (*this);
    }
    IRObjectBuilder &withLexType(TokenType type)
    {
        _lex_type = type;
        return (*this);
    }
    IRObjectBuilder &withName(StringRef name)
    {
        _name = name;
        return (*this);
    }
    IRObjectBuilder &withValue(IRValue value)
    {
        _value = value;
        return (*this);
    }
    IRObjectBuilder &withCode(IRCode *code)
    {
        _code = code;
        return (*this);
    }
    IRObject build() const;
};
// IRStorage answers "tell me the IRAddress of this object!"
typedef size_t ir_handle_t;
class IRStorage
{
    std::list<IRObject> _objects;

    uint64_t _alloc;
    uint64_t _tmp_alloc, _tmp_alloc_max;
    uint64_t __alloc_aligned(size_t size, size_t align)
    {
        _alloc += size;
        if (_alloc % align != 0)
            _alloc += align - (_alloc % align);
        return _alloc;
    }
    uint64_t __tmp_alloc_aligned(size_t size, size_t align)
    {
        _tmp_alloc += _alloc;
        _tmp_alloc += size;
        if (_tmp_alloc % align != 0)
            _tmp_alloc += align - (_tmp_alloc % align);
        _tmp_alloc -= _alloc;

        _tmp_alloc_max =
            (_tmp_alloc_max < _tmp_alloc) ? _tmp_alloc : _tmp_alloc_max;

        return _tmp_alloc + _alloc;
    }
    void __compute_address(IRObject &o)
    {
        // mode, mem/imm/label
        switch (o.linkage)
        {
            case IR_LINKAGE_inline:
                o.addr.mode = OP_ADDR_imm;
                // only allow int for now
                o.addr.imm = o.value.asInt32();
                break;
            case IR_LINKAGE_local:
                o.addr.mode = OP_ADDR_mem;
                if (o.name.empty())
                    o.addr.mem =
                        __tmp_alloc_aligned(o.value.size, o.value.align);
                else
                    o.addr.mem = __alloc_aligned(o.value.size, o.value.align);
                break;
            case IR_LINKAGE_static:
            case IR_LINKAGE_static_const:
            case IR_LINKAGE_static_export:
            case IR_LINKAGE_extern:
                o.addr.mode = OP_ADDR_label;
                o.addr.label = &o.name;
                break;
            default: SyntaxError("IRStorage: unknown ir linkage."); break;
        }

        // width
        // TODO: where to get width info?
        if (o.code == nullptr)
        {
            o.addr.width = std::min(o.value.size, o.value.align);
            assert(o.addr.width >= 1 && o.addr.width <= 10);
        }
        else
        {
            o.addr.width = sizeof(void *);
        }
    }

   public:
    IRStorage() : _alloc(0), _tmp_alloc(0), _tmp_alloc_max(0) {}
    void add(IRObject o);
    IRAddress addAndGetAddress(IRObject o);
    IRAddress getAddressByName(StringRef name) const;
    void collectTemporarySpace();  // call after expression

    const std::list<IRObject> &get() const
    {
        return _objects;
    }
    std::list<IRInstruction> allocCode() const;
    std::list<IRInstruction> freeCode() const;

    std::string toString() const;
};

class IRCode
{
    std::list<IRInstruction> _code;

   public:
    void add(IRInstruction code)
    {
        _code.push_back(code);
    }
    void addLabel(StringRef *begin, StringRef *end)
    {
        assert(!_code.empty());
        _code.front().prelabel = begin;
        _code.back().postlabel = end;
    }
    void append(IRCode &code)
    {
        _code.insert(_code.end(), code._code.begin(), code._code.end());
        // _code.splice(_code.end(), code._code);
    }
    void append(std::list<IRInstruction> &&code)
    {
        _code.insert(_code.end(), code.begin(), code.end());
        // _code.splice(_code.end(), code);
    }
    size_t size() const
    {
        return _code.size();
    }
    const std::list<IRInstruction> &get() const
    {
        return _code;
    }

    std::string toString() const;
};

// Preparation:
//   1. all env.storage() ready
//   2. all code() in function_definition ready
//

class IRTranslator
{
   public:
    virtual void onObject(const IRObject &object) = 0;
    virtual void onInstruction(const IRInstruction &inst) = 0;
    virtual std::string emit() const = 0;
};

class IR_to_x64 : public IRTranslator
{
    std::string _text;
    std::string _data;
    std::string _data_const;
    std::string _data_bss;

   public:
    virtual void onObject(const IRObject &object);
    virtual void onInstruction(const IRInstruction &inst);
    virtual std::string emit() const;
};

// Execute simple IRInstruction sequence.
#include <unordered_map>
class IRSimulator
{
    static char memory[4096];
    std::unordered_map<std::string, int> label_pos;
    int get_value(const IRAddress &addr);
    int get_value_or_label(const IRAddress &addr, int i);
    char *get_addr(const IRAddress &addr);

   public:
    int run(const IRCode &code);
};

class IRUtil
{
   public:
    static std::string OpcodeToString(EIROpcode op);
    static std::string LinkageToString(EIRLinkage linkage);
    static StringRef GenerateLabel();
};
