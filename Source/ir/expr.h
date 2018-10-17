#pragma once

#include "../symbol/symbol.h"

// 1. tree structure
struct ExprNode {
    Type * type;

    virtual std::string toString() const {
        return "?";
    }
};

enum ExprOp { ADD };

struct BinaryNode : ExprNode {
    ExprOp op;
    ExprNode * arg1;
    ExprNode * arg2;

    virtual std::string toString() const {
        return arg1->toString() + " " + arg2->toString() + " +";
    }
};

struct UnaryNode : ExprNode {
    ExprOp op;
    ExprNode * arg;

    virtual std::string toString() const {
        return "?";
    }
};

// maybe merge with UnaryNode
struct CastNode : ExprNode {
    Type * source_type;
    ExprNode * arg;

    virtual std::string toString() const {
        return arg->toString() + " <cast>";
    }
};

struct PrimaryNode : ExprNode {
    Symbol * symbol;
    // constant
    // string literal

    virtual std::string toString() const {
        return symbol->name.toString();
    }
};

bool can_cast(const Type * to, const Type * from) {
    // integral <-> integral
    return to->isIntegral() && from->isIntegral();
}
// bool need_cast(const Type * to, const Type * from);

BinaryNode * new_binary_node(Type * type,
                             ExprOp op,
                             ExprNode * arg1,
                             ExprNode * arg2) {
    BinaryNode * bn = new BinaryNode;
    bn->type = type;
    bn->op = op;
    bn->arg1 = arg1;
    bn->arg2 = arg2;
    return bn;
}

CastNode * new_cast_node(Type * type, Type * source_type, ExprNode * arg) {
    CastNode * cn = new CastNode;
    cn->type = type;
    cn->source_type = source_type;
    cn->arg = arg;
    return cn;
}

PrimaryNode * new_primary_node(Symbol * symbol) {
    PrimaryNode * pn = new PrimaryNode;
    pn->type = symbol->objectType;
    pn->symbol = symbol;
    return pn;
}

// 2. build tree (type checking, cast node insertion)

class ExprTreeBuilder {
public:
    static PrimaryNode * NODE(Symbol * symbol) {
        CHECK(symbol && symbol->objectType);
        return new_primary_node(symbol);
    }
    static BinaryNode * ADD(ExprNode * n2, ExprNode * n1) {
        // Type checking
        CHECK((n1->type->isPointer() && n2->type->isIntegral()) ||
              (n1->type->isArithmetic() && n2->type->isArithmetic()));

        // Decide result type
        Type * type = nullptr;
        if (n1->type->isArithmetic() && n2->type->isArithmetic())
        {
            type = UsualArithmeticConversion(n1->type, n2->type);
        }
        else if (n1->type->isPointer() && n2->type->isIntegral())
        {
            type = n1->type;
        }
        else if (n1->type->isIntegral() && n2->type->isPointer())
        {
            type = n2->type;
        }
        CHECK(type != nullptr);

        // Cast node insertion
        if (can_cast(type, n1->type))
        {
            n1 = new_cast_node(type, n1->type, n1);
        }
        if (can_cast(type, n2->type))
        {
            n2 = new_cast_node(type, n2->type, n2);
        }

        return new_binary_node(type, ExprOp::ADD, n1, n2);
    }
};

// 3. eval tree (code emitting)
// see ir.h
