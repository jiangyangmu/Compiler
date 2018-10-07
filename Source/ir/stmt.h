#pragma once

#include "expr.h"
#include "../symbol/symbol.h"

enum StmtType {
    IF,
    SWITCH,
    WHILE,
    COMPOUND,
};

struct Stmt {
    StmtType type;

    virtual std::string toString() const {
        return "?";
    }
};

struct IfStmt : Stmt {
    ExprNode * cond_expr;
    Stmt * if_stmt;
    Stmt * else_stmt;

    virtual std::string toString() const {
        return "IF (" + cond_expr->toString() + ") THEN {" +
            if_stmt->toString() + "}" +
            (else_stmt ? " ELSE {" + else_stmt->toString() + "}" : "");
    }
};

struct SwitchStmt : Stmt {
    ExprNode * cond_expr;
    Stmt * stmt;

    virtual std::string toString() const {
        return "SWITCH (" + cond_expr->toString() + ") {" + stmt->toString() +
            "}";
    }
};

struct WhileStmt : Stmt {
    ExprNode * cond_expr;
    Stmt * stmt;
};

struct ExpressionStmt : Stmt {
    ExprNode * expr;

    virtual std::string toString() const {
        return "EXPR {" + expr->toString() + "}";
    }
};

struct CompoundStmt : Stmt {
    // env
    std::vector<Stmt *> stmts;
};

struct LabelStmt : Stmt {
    Symbol * label;
    Stmt * stmt;

    virtual std::string toString() const {
        return "LABEL(" + label->toString() + ") {" + stmt->toString() + "}";
    }
};

struct CaseStmt : Stmt {
    // value
    Stmt * stmt;
};

struct GotoStmt : Stmt {
    StringRef label;
};
struct ContinueStmt : Stmt {};

class StmtBuilder {
public:
    static Stmt * LABEL(Symbol * label, Stmt * stmt) {
        CHECK(stmt);
        LabelStmt * label_stmt = new LabelStmt;
        label_stmt->label = label;
        label_stmt->stmt = stmt;
        return label_stmt;
    }
    static Stmt * IF(ExprNode * cond_expr, Stmt * if_stmt, Stmt * else_stmt) {
        CHECK(cond_expr);
        CHECK(if_stmt);
        IfStmt * ifstmt = new IfStmt;
        ifstmt->cond_expr = cond_expr;
        ifstmt->if_stmt = if_stmt;
        ifstmt->else_stmt = else_stmt;
        return ifstmt;
    }
    static Stmt * EXPR(ExprNode * expr) {
        ExpressionStmt * expr_stmt = new ExpressionStmt;
        expr_stmt->expr = expr;
        return expr_stmt;
    }
};