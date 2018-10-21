#pragma once

#include "../symbol/symbol.h"

enum ExprType {
    EXPR_COMMA,
    EXPR_ASSIGN,
    EXPR_COND,
    EXPR_OR,
    EXPR_AND,
    EXPR_BIT_OR,
    EXPR_BIT_XOR,
    EXPR_BIT_AND,
    EXPR_EQ,
    EXPR_REL,
    EXPR_SHIFT,
    EXPR_ADD,
    EXPR_MUL,
    EXPR_CAST,
    EXPR_UNARY,
    EXPR_UNARY_INC,
    EXPR_UNARY_DEC,
    EXPR_UNARY_GET_ADDRESS,
    EXPR_UNARY_INDIRECT,
    EXPR_UNARY_POSITIVE,
    EXPR_UNARY_NEGATIVE,
    EXPR_UNARY_BIT_NOT,
    EXPR_UNARY_NOT,
    EXPR_POSTFIX,
    EXPR_POSTFIX_SUBSCRIPT,
    EXPR_POSTFIX_CALL,
    EXPR_POSTFIX_GET_MEMBER,
    EXPR_POSTFIX_INDIRECT_GET_MEMBER,
    EXPR_POSTFIX_INC,
    EXPR_POSTFIX_DEC,
    EXPR_PRIMARY,
};

std::string ExprTypeToString(ExprType et)
{
    std::string s = "";
    switch (et)
    {
        case EXPR_COMMA:                        s = "comma"; break;
        case EXPR_ASSIGN:                       s = "assign"; break;
        case EXPR_COND:                         s = "cond"; break;
        case EXPR_OR:                           s = "or"; break;
        case EXPR_AND:                          s = "and"; break;
        case EXPR_BIT_OR:                       s = "bit_or"; break;
        case EXPR_BIT_XOR:                      s = "bit_xor"; break;
        case EXPR_BIT_AND:                      s = "bit_and"; break;
        case EXPR_EQ:                           s = "eq"; break;
        case EXPR_REL:                          s = "rel"; break;
        case EXPR_SHIFT:                        s = "shift"; break;
        case EXPR_ADD:                          s = "add"; break;
        case EXPR_MUL:                          s = "mul"; break;
        case EXPR_CAST:                         s = "cast"; break;
        case EXPR_UNARY:                        s = "unary"; break;
        case EXPR_UNARY_INC:                    s = "inc"; break;
        case EXPR_UNARY_DEC:                    s = "dec"; break;
        case EXPR_UNARY_GET_ADDRESS:            s = "get_address"; break;
        case EXPR_UNARY_INDIRECT:               s = "indirect"; break;
        case EXPR_UNARY_POSITIVE:               s = "positive"; break;
        case EXPR_UNARY_NEGATIVE:               s = "negative"; break;
        case EXPR_UNARY_BIT_NOT:                s = "bit_not"; break;
        case EXPR_UNARY_NOT:                    s = "not"; break;
        case EXPR_POSTFIX:                      s = "postfix"; break;
        case EXPR_POSTFIX_SUBSCRIPT:            s = "subscript"; break;
        case EXPR_POSTFIX_CALL:                 s = "call"; break;
        case EXPR_POSTFIX_GET_MEMBER:           s = "get_member"; break;
        case EXPR_POSTFIX_INDIRECT_GET_MEMBER:  s = "ind_get_member"; break;
        case EXPR_POSTFIX_INC:                  s = "postfix inc"; break;
        case EXPR_POSTFIX_DEC:                  s = "postfix dec"; break;
        case EXPR_PRIMARY:                      s = "primary"; break;
        default:                                s = "<unknown>"; break;
    }
    return s;
}

struct ExprNode;
extern std::string ExprTreeToString(const ExprNode * root);

struct ExprNode {
    ExprType    exprType;
    Type *      objectType;

    std::string toString() const
    {
        return ExprTreeToString(this);
    }
};

struct ExprNodeList {
    ExprNode * node;
    ExprNodeList * next;
};

// comma, assign, or, and, bit-or, bit-xor, bit-and, eq, rel, shift, add, mul
struct BinaryNode : ExprNode {
    ExprNode * left;
    ExprNode * right;
};

struct CondNode : ExprNode {
    ExprNode * cond;
    ExprNode * trueResult;
    ExprNode * falseResult;
};

struct CastNode : ExprNode {
    ExprNode * origin;
};

struct UnaryNode : ExprNode {
    ExprNode * target;
};

struct PostfixNode : ExprNode {
    ExprNode * target;
    union {
        ExprNode * arraySizeExpr;
        ExprNodeList argumentList;
        StringRef * memberId;
    };
};

struct PrimaryNode : ExprNode {
    Symbol * symbol;
    Token constant;
};

std::string ExprTreeToStringImpl(const ExprNode * root, std::string indent)
{
    CHECK(root);
    std::string s = indent;
    if (root->exprType == EXPR_PRIMARY)
    {
        const PrimaryNode * p = static_cast<const PrimaryNode *>(root);
        s += p->symbol
             ? p->symbol->name.toString() + ":id"
             : p->constant.text.toString() + ":const";
        s += ":" + root->objectType->toString();
        s += "\n";
    }
    else if (root->exprType >= EXPR_UNARY_INC &&
             root->exprType <= EXPR_UNARY_NOT)
    {
        const UnaryNode * p = static_cast<const UnaryNode *>(root);
        switch (root->exprType)
        {
            case EXPR_UNARY_INC:            s = "++"; break;
            case EXPR_UNARY_DEC:            s = "--"; break;
            case EXPR_UNARY_GET_ADDRESS:    s = "&"; break;
            case EXPR_UNARY_INDIRECT:       s = "*"; break;
            case EXPR_UNARY_POSITIVE:       s = "+"; break;
            case EXPR_UNARY_NEGATIVE:       s = "-"; break;
            case EXPR_UNARY_BIT_NOT:        s = "~"; break;
            case EXPR_UNARY_NOT:            s = "!"; break;
            default:                        break;
        }
        s += "()";
        s += ":" + root->objectType->toString();
        s += "\n";
        indent += "  ";
        s += ExprTreeToStringImpl(p->target, indent);
    }
    else if (root->exprType == EXPR_POSTFIX_INC ||
             root->exprType == EXPR_POSTFIX_DEC)
    {
        const PostfixNode * p = static_cast<const PostfixNode *>(root);
        s += root->exprType == EXPR_POSTFIX_INC
             ? "()++"
             : "()--";
        s += ":" + root->objectType->toString();
        s += "\n";
        indent += "  ";
        s += ExprTreeToStringImpl(p->target, indent);
    }
    else if (root->exprType == EXPR_POSTFIX_GET_MEMBER ||
             root->exprType == EXPR_POSTFIX_INDIRECT_GET_MEMBER)
    {
        const PostfixNode * p = static_cast<const PostfixNode *>(root);
        s += root->exprType == EXPR_POSTFIX_GET_MEMBER
            ? "().()"
            : "()->()";
        s += ":" + root->objectType->toString();
        s += "\n";
        indent += "  ";
        s += ExprTreeToStringImpl(p->target, indent);
        s += indent + p->memberId->toString() + "\n";
    }
    else if (root->exprType == EXPR_POSTFIX_CALL)
    {
        const PostfixNode * p = static_cast<const PostfixNode *>(root);
        s += "call";
        s += ":" + root->objectType->toString();
        s += "\n";
        indent += "  ";
        s += ExprTreeToStringImpl(p->target, indent);
        for (const ExprNodeList *
             argumentNode = p->argumentList.next;
             argumentNode;
             argumentNode = argumentNode->next)
        {
            s += ExprTreeToStringImpl(argumentNode->node, indent);
        }
    }
    else if (root->exprType == EXPR_POSTFIX_SUBSCRIPT)
    {
        const PostfixNode * p = static_cast<const PostfixNode *>(root);
        s += "[]";
        s += ":" + root->objectType->toString();
        s += "\n";
        indent += "  ";
        s += ExprTreeToStringImpl(p->target, indent);
        s += ExprTreeToStringImpl(p->arraySizeExpr, indent);
    }
    else
    {
        s += ExprTypeToString(root->exprType);
    }

    return s;
}
std::string ExprTreeToString(const ExprNode * root)
{
    return ExprTreeToStringImpl(root, "");
}

// Type checking, cast node insertion
class ExprTreeBuilder {
public:
    static BinaryNode * Comma(ExprNode * n1, ExprNode * n2)
    {
        BinaryNode * comma = new BinaryNode;
        //comma->
        return nullptr;
    }
    static BinaryNode * Assign(ExprNode * to, ExprNode * from)
    {
        CHECK(to->objectType->isLvalue() &&
              ((to->objectType->isArithmetic()         && from->objectType->isArithmetic()) ||
               (to->objectType->getClass() == T_STRUCT && IsCompatibleType(to->objectType, from->objectType)) ||
               (to->objectType->getClass() == T_UNION  && IsCompatibleType(to->objectType, from->objectType)) ||
               (to->objectType->isPointer()            && IsAssignCompatiblePointer(to->objectType, from->objectType))));


        return nullptr;
    }
    static CondNode * Cond(ExprNode * cond, ExprNode * n1, ExprNode * n2)
    {
        CHECK(cond->objectType->isScalar() &&
              ((n1->objectType->isArithmetic()         && n2->objectType->isArithmetic()) ||
               (n1->objectType->getClass() == T_STRUCT && n2->objectType->getClass() == T_STRUCT) ||
               (n1->objectType->getClass() == T_UNION  && n2->objectType->getClass() == T_UNION) ||
               (n1->objectType->getClass() == T_VOID   && n2->objectType->getClass() == T_VOID) ||
               (n1->objectType->isPointer()            && n2->objectType->isIntegral())));


        return nullptr;
    }
    static BinaryNode * Or(ExprNode * left, ExprNode * right)
    {
        CHECK(left->objectType->isScalar() && right->objectType->isScalar());

        return nullptr;
    }
    static BinaryNode * And(ExprNode * left, ExprNode * right)
    {
        CHECK(left->objectType->isScalar() && right->objectType->isScalar());

        return nullptr;
    }
    static BinaryNode * BitOr(ExprNode * left, ExprNode * right)
    {
        CHECK(left->objectType->isIntegral() && right->objectType->isIntegral());

        return nullptr;
    }
    static BinaryNode * BitXor(ExprNode * left, ExprNode * right)
    {
        CHECK(left->objectType->isIntegral() && right->objectType->isIntegral());

        return nullptr;
    }
    static BinaryNode * BitAnd(ExprNode * left, ExprNode * right)
    {
        CHECK(left->objectType->isIntegral() && right->objectType->isIntegral());

        return nullptr;
    }
    static BinaryNode * Eq(ExprNode * left, ExprNode * right)
    {
        CHECK((left->objectType->isArithmetic() && right->objectType->isArithmetic()) ||
              (left->objectType->isPointer()    && IsCompatiblePointer(left->objectType, right->objectType)));

        return nullptr;
    }
    static BinaryNode * Rel(ExprNode * left, ExprNode * right)
    {
        CHECK((left->objectType->isArithmetic() && right->objectType->isArithmetic()) ||
              (left->objectType->isPointer()    && IsStrictCompatiblePointer(left->objectType, right->objectType)));
        return nullptr;
    }
    static BinaryNode * Shift(ExprNode * left, ExprNode * right)
    {
        CHECK(left->objectType->isIntegral() && right->objectType->isIntegral());

        return nullptr;
    }
    static BinaryNode * Add(ExprNode * left, ExprNode * right)
    {
        CHECK((left->objectType->isArithmetic()    && right->objectType->isArithmetic()) ||
              (IsPointerToObject(left->objectType) && right->objectType->isIntegral()));

        return nullptr;
    }
    static BinaryNode * Sub(ExprNode * left, ExprNode * right)
    {
        CHECK((left->objectType->isArithmetic()    && right->objectType->isArithmetic()) ||
              (IsPointerToObject(left->objectType) && right->objectType->isIntegral()) ||
              (IsPointerToObject(left->objectType) && IsCompatibleType(dynamic_cast<PointerType *>(left->objectType)->getTargetType(),
                                                                       dynamic_cast<PointerType *>(right->objectType)->getTargetType())));

        return nullptr;
    }
    static BinaryNode * Mul(ExprNode * left, ExprNode * right)
    {
        CHECK(left->objectType->isArithmetic() && right->objectType->isArithmetic());

        return nullptr;
    }
    static BinaryNode * Div(ExprNode * left, ExprNode * right)
    {
        CHECK(left->objectType->isArithmetic() && right->objectType->isArithmetic());

        return nullptr;
    }
    static BinaryNode * Mod(ExprNode * left, ExprNode * right)
    {
        CHECK(left->objectType->isIntegral() && right->objectType->isIntegral());

        return nullptr;
    }
    static CastNode * Cast(Type * type, ExprNode * origin)
    {
        CHECK(type->getClass() == T_VOID ||
              (type->isScalar() && origin->objectType->isScalar()));

        return nullptr;
    }
    static UnaryNode * PrefixInc(ExprNode * expr)
    {
        CHECK(expr->objectType->isLvalue() && expr->objectType->isScalar());
        
        UnaryNode * p   = new UnaryNode;
        p->exprType     = EXPR_UNARY_INC;
        p->objectType   = expr->objectType;
        p->target       = expr;
        return p;
    }
    static UnaryNode * PrefixDec(ExprNode * expr)
    {
        CHECK(expr->objectType->isLvalue() && expr->objectType->isScalar());

        UnaryNode * p   = new UnaryNode;
        p->exprType     = EXPR_UNARY_DEC;
        p->objectType   = expr->objectType;
        p->target       = expr;
        return p;
    }
    static UnaryNode * GetAddress(ExprNode * expr)
    {
        CHECK(expr->objectType->isFunction() ||
              (expr->objectType->isObject() &&
               expr->objectType->isLvalue()));

        PointerType * pointer = new PointerType();
        pointer->setTargetType(expr->objectType);

        UnaryNode * p   = new UnaryNode;
        p->exprType     = EXPR_UNARY_GET_ADDRESS;
        p->objectType   = pointer;
        p->target       = expr;
        return p;
    }
    static UnaryNode * Indirect(ExprNode * expr)
    {
        CHECK(expr->objectType->isPointer());

        Type * type = dynamic_cast<PointerType *>(expr->objectType)->getTargetType();

        UnaryNode * p   = new UnaryNode;
        p->exprType     = EXPR_UNARY_INDIRECT;
        p->objectType   = type;
        p->target       = expr;
        return p;
    }
    static UnaryNode * Positive(ExprNode * expr)
    {
        CHECK(expr->objectType->isArithmetic());

        UnaryNode * p   = new UnaryNode;
        p->exprType     = EXPR_UNARY_POSITIVE;
        p->objectType   = expr->objectType;
        p->target       = expr;
        return p;
    }
    static UnaryNode * Negative(ExprNode * expr)
    {
        CHECK(expr->objectType->isArithmetic());

        UnaryNode * p   = new UnaryNode;
        p->exprType     = EXPR_UNARY_NEGATIVE;
        p->objectType   = expr->objectType;
        p->target       = expr;
        return p;
    }
    static UnaryNode * BitNot(ExprNode * expr)
    {
        CHECK(expr->objectType->isIntegral());

        UnaryNode * p   = new UnaryNode;
        p->exprType     = EXPR_UNARY_BIT_NOT;
        p->objectType   = expr->objectType;
        p->target       = expr;
        return p;
    }
    static UnaryNode * Not(ExprNode * expr)
    {
        CHECK(expr->objectType->isScalar());

        UnaryNode * p   = new UnaryNode;
        p->exprType     = EXPR_UNARY_NOT;
        p->objectType   = expr->objectType;
        p->target       = expr;
        return p;
    }
    static PostfixNode * Subscript(ExprNode * pointer, ExprNode * offset)
    {
        CHECK(IsPointerToObject(pointer->objectType) &&
              offset->objectType->isIntegral());
        // TODO: support reverse pointer & offset.

        PointerType * pt = dynamic_cast<PointerType *>(pointer->objectType);
        CHECK(pt);

        PostfixNode * p     = new PostfixNode;
        p->exprType         = EXPR_POSTFIX_SUBSCRIPT;
        p->objectType       = pt->getTargetType();
        p->target           = pointer;
        p->arraySizeExpr    = offset;
        return p;
    }
    static PostfixNode * Call(ExprNode * pointer, ExprNodeList argumentList)
    {
        CHECK(IsPointerToFunction(pointer->objectType) /* && argumentList match */);

        PointerType * pt = dynamic_cast<PointerType *>(pointer->objectType);
        CHECK(pt);
        FuncType * f = dynamic_cast<FuncType *>(pt->getTargetType());
        CHECK(f);

        PostfixNode * p = new PostfixNode;
        p->exprType     = EXPR_POSTFIX_CALL;
        p->objectType   = f->getTargetType();
        p->target       = pointer;
        p->argumentList = argumentList;
        return p;
    }
    static PostfixNode * GetMember(ExprNode * agg, StringRef memberId)
    {
        CHECK(agg->objectType->getClass() == T_STRUCT &&
              dynamic_cast<StructType *>(agg->objectType)->hasMember(memberId));
        // TODO: union

        PostfixNode * p = new PostfixNode;
        p->exprType     = EXPR_POSTFIX_GET_MEMBER;
        p->objectType   = dynamic_cast<StructType *>(agg->objectType)->getMemberType(memberId);
        p->target       = agg;
        p->memberId     = new StringRef(memberId);
        return p;
    }
    static PostfixNode * IndirectGetMember(ExprNode * pointer, StringRef memberId)
    {
        CHECK(pointer->objectType->isPointer());
        Type * agg = dynamic_cast<PointerType *>(pointer->objectType)->getTargetType();
        CHECK(agg->getClass() == T_STRUCT &&
              dynamic_cast<StructType *>(agg)->hasMember(memberId));
        // TODO: union

        PostfixNode * p = new PostfixNode;
        p->exprType     = EXPR_POSTFIX_INDIRECT_GET_MEMBER;
        p->objectType   = dynamic_cast<StructType *>(agg)->getMemberType(memberId);
        p->target       = pointer;
        p->memberId     = new StringRef(memberId);
        return p;
    }
    static PostfixNode * PostfixInc(ExprNode * expr)
    {
        CHECK(expr->objectType->isLvalue() &&
              expr->objectType->isScalar());

        PostfixNode * p = new PostfixNode;
        p->exprType     = EXPR_POSTFIX_INC;
        p->objectType   = expr->objectType;
        p->target       = expr;
        return p;
    }
    static PostfixNode * PostfixDec(ExprNode * expr)
    {
        CHECK(expr->objectType->isLvalue() &&
              expr->objectType->isScalar());

        PostfixNode * p = new PostfixNode;
        p->exprType     = EXPR_POSTFIX_DEC;
        p->objectType   = expr->objectType;
        p->target       = expr;
        return p;
    }
    static PrimaryNode * Primary(Symbol * symbol)
    {
        CHECK(symbol && symbol->objectType);

        PrimaryNode * p = new PrimaryNode;
        p->exprType     = EXPR_PRIMARY;
        p->objectType   = symbol->objectType;
        p->symbol       = symbol;
        return p;
    }
    static PrimaryNode * Primary(Token constant)
    {
        CHECK(constant.type == Token::CONST_INT ||
              constant.type == Token::CONST_CHAR ||
              constant.type == Token::CONST_FLOAT ||
              constant.type == Token::STRING);

        PrimaryNode * p = new PrimaryNode;
        p->exprType     = EXPR_PRIMARY;
        p->objectType   = TypeBuilder::FromConstantToken(constant);
        p->constant     = constant;
        p->symbol       = nullptr;
        return p;
    }
};
