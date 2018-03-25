#pragma once

#include <vector>

struct Node;
struct ComputeTree;

enum NodeType
{
    NT_CONTROL,
    NT_COMPUTE,
    NT_BEGIN_GUARD,
    NT_END_GUARD
};

struct ControlNode
{
    // test operand: compute node ptr
    struct Node *test;
    // jump operator: ret, jmp, jmpIfTrue, jmpIfFalse
    // jump to: if ret, it is compute node(has return value) or nullptr
    struct Node *dest;
};

struct ComputeNode
{
    // compute tree ptr
    struct ComputeTree *ct;
};

// All operations allowed in C types
enum ComputeType
{
    COMPUTE_unknown,
    COMPUTE_comma,
    COMPUTE_assign,
    COMPUTE_add_assign,
    COMPUTE_sub_assign,
    COMPUTE_mul_assign,
    COMPUTE_div_assign,
    COMPUTE_mod_assign,
    COMPUTE_shl_assign,
    COMPUTE_shr_assign,
    COMPUTE_and_assign,
    COMPUTE_or_assign,
    COMPUTE_xor_assign,
    COMPUTE_cond,
    COMPUTE_or,
    COMPUTE_and,
    COMPUTE_bit_or,
    COMPUTE_bit_xor,
    COMPUTE_bit_and,
    COMPUTE_eq,
    COMPUTE_ne,
    COMPUTE_gt,
    COMPUTE_ge,
    COMPUTE_lt,
    COMPUTE_le,
    COMPUTE_shl,
    COMPUTE_shr,
    COMPUTE_add,
    COMPUTE_sub,
    COMPUTE_mul,
    COMPUTE_div,
    COMPUTE_mod,
    COMPUTE_cast, // TODO: expand to concret cast, like i32_to_f32
    COMPUTE_inc,
    COMPUTE_dec,
    COMPUTE_address,
    COMPUTE_indirect,
    COMPUTE_pos,
    COMPUTE_neg,
    COMPUTE_bit_not,
    COMPUTE_not,
    COMPUTE_sizeof,  // temporary, become integer constant after type derivation
    COMPUTE_subscript,
    COMPUTE_call,
    COMPUTE_member_ref,
    COMPUTE_member_ind,
    COMPUTE_post_inc,
    COMPUTE_post_dec,
};
struct ComputeTree
{
    // type
    union {
        // internal node
        struct
        {
            ComputeType op;
            struct ComputeTree *left, *right;
        };
        // leaf node (id, string, constant)
        Token t;
    };

    // for type derivation
    Type *ctype;
    // for codegen: storage location
};

struct Node
{
    struct Node *prev, *next;  // for next/prev location fix
    // type
    // id
    // label

    union {
        struct ControlNode ctrl;
        struct ComputeNode comp;
    }

    // bool isGuard() const;
};
// TODO
class NodeBuilder
{
   public:
    Node *Jump(Node *dest);
    // Cmp
    // JumpIfEq, JumpIfTrue, JumpIfFalse
    // Return, ReturnWith
    // ComputeNode
    // ComputeOp, ComputeLeaf
};

class NodeList
{
    // init with guard nodes
    // Node* begin()
    // Node* end()

    // add(Node *)
    // append(NodeList &)
};

using Ast = SyntaxNode;

// TODO: List<T>
class CodeAst
{
   public:
    void generate(const Ast *ast);

   private:
    NodeList nodes;
    std::vector<ComputeTree *> trees;
};
