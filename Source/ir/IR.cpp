#include "IR.h"

#include "CallingConvention.h"
#include "../Util/Bit.h"

namespace Language {

void AddChild(Node * parent, Node * child)
{
    ASSERT(parent && child && !child->up);
    
    child->up = parent;
    
    if (!parent->down)
    {
        parent->down = child;
    }
    else
    {
        Node * last = parent->down;
        while (last->right)
            last = last->right;
        last->right = child;
    }
}

int CountChild(Node * node)
{
    ASSERT(node);
    int n = 0;
    Node * next = node->down;
    while (next)
    {
        ++n;
        next = next->right;
    }
    return n;
}

Node * LastChild(Node * parent)
{
    ASSERT(parent);

    Node * lastChild = parent->down;
    
    if (lastChild)
    {
        while (lastChild->right)
        {
            lastChild = lastChild->right;
        }
    }

    return lastChild;
}

Node * MakeNode(NodeType type)
{
    Node * node = new Node;
    node->down = node->right = node->up = nullptr;
    node->type = type;

    if (type > BEGIN_STATEMENT && type < END_STATEMENT)
    {
        node->stmt.context = nullptr;
    }
    else if ((type > BEGIN_EXPRESSION && type < END_EXPRESSION) ||
             type == EMPTY_EXPRESSION)
    {
        node->expr.type = nullptr;
        node->expr.loc.type = NO_WHERE;
    }

    return node;
}

void DestroyNodeTree(Node * root)
{
    if (root)
    {
        Node * child = root->down;
        while (child)
        {
            Node * nextChild = child->right;
            DestroyNodeTree(child);
            child = nextChild;
        }
        delete root;
    }
}

std::string NodeDebugString(Node * node)
{
    std::string s;
    switch (node->type)
    {
        case STMT_COMPOUND:     s = "compound"; break;
        case STMT_EXPRESSION:   s = "expression"; break;
        case STMT_IF:           s = "if"; break;
        case STMT_WHILE:        s = "while"; break;
        case STMT_DO_WHILE:     s = "do_while"; break;
        case STMT_FOR:          s = "for"; break;
        case STMT_BREAK:        s = "break"; break;
        case STMT_CONTINUE:     s = "continue"; break;
        case STMT_RETURN:       s = "return"; break;
        case STMT_GOTO:         s = "goto"; break;
        case STMT_LABEL:        s = "label"; break;
        case STMT_SWITCH:       s = "switch"; break;
        case STMT_CASE:         s = "case"; break;
        case STMT_DEFAULT:      s = "default"; break;
        case EXPR_DATA:         s = "data"; break;
        case EXPR_CALL:         s = "call"; break;
        case EXPR_CVT_SI2SI:    s = "cvt_si2si"; break;
        case EXPR_CVT_SI2UI:    s = "cvt_si2ui"; break;
        case EXPR_CVT_UI2SI:    s = "cvt_ui2si"; break;
        case EXPR_CVT_UI2UI:    s = "cvt_ui2ui"; break;
        case EXPR_CVT_F2F:      s = "cvt_f2f"; break;
        case EXPR_CVT_SI2F:     s = "cvt_si2f"; break;
        case EXPR_CVT_F2SI:     s = "cvt_f2si"; break;
        case EXPR_CVT_I2B:      s = "cvt_i2b"; break;
        case EXPR_CVT_B2I:      s = "cvt_b2i"; break;
        case EXPR_CVT_F2B:      s = "cvt_f2b"; break;
        case EXPR_CVT_B2F:      s = "cvt_b2f"; break;
        case EXPR_CVT_REINTERP: s = "cvt_reinterp"; break;
        case EXPR_BOOL_NOT:     s = "bool_not"; break;
        case EXPR_BOOL_AND:     s = "bool_and"; break;
        case EXPR_BOOL_OR:      s = "bool_or"; break;
        case EXPR_IADD:         s = "iadd"; break;
        case EXPR_ISUB:         s = "isub"; break;
        case EXPR_IMUL:         s = "imul"; break;
        case EXPR_IDIV:         s = "idiv"; break;
        case EXPR_IMOD:         s = "imod"; break;
        case EXPR_INOT:         s = "inot"; break;
        case EXPR_IAND:         s = "iand"; break;
        case EXPR_IOR:          s = "ior"; break;
        case EXPR_IXOR:         s = "ixor"; break;
        case EXPR_ISHL:         s = "ishl"; break;
        case EXPR_ISHR:         s = "ishr"; break;
        case EXPR_IEQ:          s = "ieq"; break;
        case EXPR_INE:          s = "ine"; break;
        case EXPR_ILT:          s = "ilt"; break;
        case EXPR_ILE:          s = "ile"; break;
        case EXPR_IGE:          s = "ige"; break;
        case EXPR_IGT:          s = "igt"; break;
        case EXPR_FNEG:         s = "fneg"; break;
        case EXPR_FADD:         s = "fadd"; break;
        case EXPR_FSUB:         s = "fsub"; break;
        case EXPR_FMUL:         s = "fmul"; break;
        case EXPR_FDIV:         s = "fdiv"; break;
        case EXPR_FEQ:          s = "feq"; break;
        case EXPR_FNE:          s = "fne"; break;
        case EXPR_FLT:          s = "flt"; break;
        case EXPR_FLE:          s = "fle"; break;
        case EXPR_FGE:          s = "fge"; break;
        case EXPR_FGT:          s = "fgt"; break;
        case EXPR_PADDSI:       s = "paddsi"; break;
        case EXPR_PADDUI:       s = "paddui"; break;
        case EXPR_PSUBSI:       s = "psubsi"; break;
        case EXPR_PSUBUI:       s = "psubui"; break;
        case EXPR_PDIFF:        s = "pdiff"; break;
        case EXPR_PEQ:          s = "peq"; break;
        case EXPR_PNE:          s = "pne"; break;
        case EXPR_PLT:          s = "plt"; break;
        case EXPR_PLE:          s = "ple"; break;
        case EXPR_PGE:          s = "pge"; break;
        case EXPR_PGT:          s = "pgt"; break;
        case EXPR_PIND:         s = "pind"; break;
        case EXPR_PNEW:         s = "pnew"; break;
        case EXPR_MCOPY:        s = "mcopy"; break;
        case EXPR_MDUP:         s = "mdup"; break;
        case EXPR_MADDUI:       s = "maddui"; break;
        case EXPR_MADDSI:       s = "maddsi"; break;
        case EXPR_CONDITION:    s = "condition"; break;
        case EXPR_ELIST:        s = "elist"; break;
        case EMPTY_EXPRESSION:  s = "empty_expr"; break;
        default: ASSERT(false); break;
    }

    return s;
}

}