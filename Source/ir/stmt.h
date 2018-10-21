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
        return "IF (" + (cond_expr ? cond_expr->toString() : "") +
            ") THEN {" + if_stmt->toString() + "}" +
            (else_stmt ? " ELSE {" + else_stmt->toString() + "}" : "");
    }
};

struct SwitchStmt : Stmt {
    ExprNode * cond_expr;
    Stmt * stmt;

    virtual std::string toString() const {
        return "SWITCH (" + (cond_expr ? cond_expr->toString() : "") +
            ") {" + stmt->toString() + "}";
    }
};

struct WhileStmt : Stmt {
    ExprNode * cond_expr;
    Stmt * stmt;

    virtual std::string toString() const {
        return "WHILE (" + (cond_expr ? cond_expr->toString() : "") +
            ") {" + stmt->toString() + "}";
    }
};

struct DoWhileStmt : Stmt {
    ExprNode * cond_expr;
    Stmt * stmt;

    virtual std::string toString() const {
        return "DO {" + stmt->toString() + "} WHILE(" +
            (cond_expr ? cond_expr->toString() : "") + ")";
    }
};

struct ForStmt : Stmt {
    ExprNode * init_expr;
    ExprNode * loop_expr;
    ExprNode * tail_expr;
    Stmt * stmt;

    virtual std::string toString() const {
        return "FOR (" +
            (init_expr ? init_expr->toString() : "") + ";" +
            (loop_expr ? loop_expr->toString() : "") + ";" +
            (tail_expr ? tail_expr->toString() : "") + ") {" +
            stmt->toString() + "}";
    }
};

struct ExpressionStmt : Stmt {
    ExprNode * expr;

    virtual std::string toString() const {
        return "EXPR (" + (expr ? expr->toString() : "") + ")";
    }
};

struct CompoundStmt : Stmt {
    Environment * env;
    std::vector<Stmt *> stmts;

    virtual std::string toString() const {
        std::string s;
        s += "COMPOUND {";
        for (Stmt * stmt : stmts)
            s += stmt->toString() + ";";
        s += "}";
        return s;
    }
};

struct LabelStmt : Stmt {
    StringRef label;
    Stmt * stmt;

    virtual std::string toString() const {
        return "LABEL (" + label.toString() + ") {" + stmt->toString() + "}";
    }
};

struct CaseStmt : Stmt {
    ExprNode * expr;
    Stmt * stmt;
    virtual std::string toString() const {
        return "CASE (" + (expr ? expr->toString() : "") +
            "): {" + stmt->toString() + "}";
    }
};

struct DefaultStmt : Stmt {
    Stmt * stmt;

    virtual std::string toString() const {
        return "DEFAULT {" + stmt->toString() + "}";
    }
};

struct GotoStmt : Stmt {
    StringRef label;

    virtual std::string toString() const {
        return "GOTO (" + label.toString() + ")";
    }
};

struct ContinueStmt : Stmt {
    virtual std::string toString() const {
        return "CONTINUE";
    }
};

struct BreakStmt : Stmt {
    virtual std::string toString() const {
        return "BREAK";
    }
};

struct ReturnStmt : Stmt {
    ExprNode * expr;

    virtual std::string toString() const {
        return "RETURN (" + (expr ? expr->toString() : "") + ")";
    }
};

std::string StatementToString(Stmt * stmt)
{
    assert(stmt);
    std::string raw = stmt->toString();
    std::string fmt;
    std::string indent;
    for (char c : raw)
    {
        if (c == '{')
        {
            fmt += "\n" + indent + "{";
            indent += "  ";
            fmt += "\n" + indent;
        }
        else if (c == '}')
        {
            indent.pop_back(), indent.pop_back();
            //if (fmt.back() != '\n')
            fmt += "\n" + indent + "}";
            fmt += "\n" + indent;
        }
        else if (c == ';')
            fmt  += "\n" + indent;
        else
            fmt += c;
    }
    return fmt;
}
