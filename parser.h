#pragma once

#include <iostream>
#include <sstream>
#include <stack>
using namespace std;

#include "lexer.h"
#include "env.h"

// XXX: all tryParse() functions return 'nullptr' on failure!!!
// XXX: all parse() functions raise SyntaxError on failure!!!

// helplers
bool IsDeclaration(TokenType t);
bool IsAssignOperator(TokenType t);
bool IsUnaryOperator(TokenType t);

class SyntaxNode
{
   public:
    virtual string debugString() { return ""; }
};

// TODO: implement 'abstract-decl' part
class TypeName : public SyntaxNode
{
   public:
    static SyntaxNode *parse(Lexer &lex, Environment *env);
};

// no instance, only dispatch
class Statement : public SyntaxNode
{
   public:
    static SyntaxNode *parse(Lexer &lex, Environment *env);
};
class LabelStatement : public SyntaxNode
{
   public:
    static SyntaxNode *parse(Lexer &lex, Environment *env);
};
class CompoundStatement : public SyntaxNode
{
    vector<SyntaxNode *> stmts;
    Environment *env;

   public:
    static SyntaxNode *parse(Lexer &lex, Environment *env, bool reuse_env = false);
    virtual string debugString()
    {
        string s = "{>\n";
        for (SyntaxNode *stmt: stmts)
        {
            s += stmt->debugString();
        }
        s += "<}\n";
        return s;
    }
};
// no instance, dispatch only
class ExpressionStatement : public SyntaxNode
{
   public:
    static SyntaxNode *parse(Lexer &lex, Environment *env);
};
class SelectionStatement : public SyntaxNode
{
    SyntaxNode *expr;
    SyntaxNode *stmt;
    SyntaxNode *stmt2;

   public:
    static SyntaxNode *parse(Lexer &lex, Environment *env);
    virtual string debugString()
    {
        string s = "Selection>\n";
        s += expr->debugString();
        s += stmt->debugString();
        if (stmt2) s += stmt2->debugString();
        s += "<\n";
        return s;
    }
};
class IterationStatement : public SyntaxNode
{
    typedef enum IterationType { WHILE_LOOP, DO_LOOP, FOR_LOOP } IterationType;

    IterationType type;
    SyntaxNode *expr;
    SyntaxNode *expr2;
    SyntaxNode *expr3;
    SyntaxNode *stmt;

   public:
    static SyntaxNode *parse(Lexer &lex, Environment *env);
    virtual string debugString()
    {
        string s;
        switch (type)
        {
            case WHILE_LOOP: s += "while"; break;
            case DO_LOOP: s += "do-while"; break;
            case FOR_LOOP: s += "for"; break;
            default: break;
        }
        s += ">\n";
        if (expr) s += expr->debugString();
        if (expr2) s += expr2->debugString();
        if (expr3) s += expr3->debugString();
        s += "<\n";
        return s;
    }
};
class JumpStatement : public SyntaxNode
{
    typedef enum JumpType {
        JMP_GOTO,
        JMP_CONTINUE,
        JMP_BREAK,
        JMP_RETURN
    } JumpType;

    JumpType type;
    SyntaxNode *expr;
    int id;  // for goto Label
   public:
    static SyntaxNode *parse(Lexer &lex, Environment *env);
    virtual string debugString()
    {
        string s;
        switch (type)
        {
            case JMP_BREAK: s += "break"; break;
            case JMP_CONTINUE: s += "continue"; break;
            case JMP_GOTO: s += "goto"; break;
            case JMP_RETURN: s += "return"; break;
        }
        s += ">\n";
        if (expr) s += expr->debugString();
        s += "<\n";
        return s;
    }
};

class Expression : public SyntaxNode
{
    vector<SyntaxNode *> exprs;

   public:
    static SyntaxNode *parse(Lexer &lex, Environment *env);
    virtual string debugString()
    {
        string s = "expr>\n";
        for (SyntaxNode *e : exprs) s += e->debugString();
        s += "<\n";
        return s;
    }
};
// TODO: fix this class, only works for most cases for now
class AssignExpression : public SyntaxNode
{
    // treat unary as condition
    // vector<UnaryExpression *> targets;
    vector<SyntaxNode *> targets;
    SyntaxNode *source;

   public:
    static SyntaxNode *parse(Lexer &lex, Environment *env);
    virtual string debugString()
    {
        string s = "=>\n";
        for (SyntaxNode *t : targets) s += t->debugString();
        if (source) s += source->debugString();
        s += "<\n";
        return s;
    }
};
class CondExpression : public SyntaxNode
{
    SyntaxNode *cond;
    SyntaxNode *left;
    SyntaxNode *right;

   public:
    static SyntaxNode *parse(Lexer &lex, Environment *env);
    virtual string debugString()
    {
        string s = "?:>\n";
        if (cond) s += cond->debugString();
        if (left) s += left->debugString();
        if (right) s += right->debugString();
        s += "<\n";
        return s;
    }
};
class OrExpression : public SyntaxNode
{
    vector<SyntaxNode *> exprs;

   public:
    static SyntaxNode *parse(Lexer &lex, Environment *env);
    virtual string debugString()
    {
        string s = "||>\n";
        for (SyntaxNode *e : exprs) s += e->debugString();
        s += "<\n";
        return s;
    }
};
class AndExpression : public SyntaxNode
{
    vector<SyntaxNode *> exprs;

   public:
    static SyntaxNode *parse(Lexer &lex, Environment *env);
    virtual string debugString()
    {
        string s = "&&>\n";
        for (SyntaxNode *e : exprs) s += e->debugString();
        s += "<\n";
        return s;
    }
};
class BitOrExpression : public SyntaxNode
{
    vector<SyntaxNode *> exprs;

   public:
    static SyntaxNode *parse(Lexer &lex, Environment *env);
    virtual string debugString()
    {
        string s = "|>\n";
        for (SyntaxNode *e : exprs) s += e->debugString();
        s += "<\n";
        return s;
    }
};
class BitXorExpression : public SyntaxNode
{
    vector<SyntaxNode *> exprs;

   public:
    static SyntaxNode *parse(Lexer &lex, Environment *env);
    virtual string debugString()
    {
        string s = "^>\n";
        for (SyntaxNode *e : exprs) s += e->debugString();
        s += "<\n";
        return s;
    }
};
class BitAndExpression : public SyntaxNode
{
    vector<SyntaxNode *> exprs;

   public:
    static SyntaxNode *parse(Lexer &lex, Environment *env);
    virtual string debugString()
    {
        string s = "&>\n";
        for (SyntaxNode *e : exprs) s += e->debugString();
        s += "<\n";
        return s;
    }
};
class EqExpression : public SyntaxNode
{
    vector<SyntaxNode *> exprs;
    vector<TokenType> ops;

   public:
    static SyntaxNode *parse(Lexer &lex, Environment *env);
    virtual string debugString()
    {
        string s = "equal>\n";
        s += exprs[0]->debugString();
        for (size_t i = 1; i < exprs.size(); ++i)
        {
            if (ops[i] == REL_EQ) s += "==\n";
            else s += "!=\n";
            s += exprs[i]->debugString();
        }
        s += "<\n";
        return s;
    }
};
class RelExpression : public SyntaxNode
{
    vector<SyntaxNode *> exprs;
    vector<TokenType> ops;

   public:
    static SyntaxNode *parse(Lexer &lex, Environment *env);
    virtual string debugString()
    {
        string s = "rel>\n";
        s += exprs[0]->debugString();
        for (size_t i = 1; i < exprs.size(); ++i)
        {
            switch (ops[i])
            {
                case REL_GT: s += "\v>\n"; break;
                case REL_GE: s += "\v>=\n"; break;
                case REL_LT: s += "\v<\n"; break;
                case REL_LE: s += "\v<=\n"; break;
                default: break;
            }
            s += exprs[i]->debugString();
        }
        s += "<\n";
        return s;
    }
};
class ShiftExpression : public SyntaxNode
{
    vector<SyntaxNode *> exprs;
    vector<TokenType> ops;

   public:
    static SyntaxNode *parse(Lexer &lex, Environment *env);
    virtual string debugString()
    {
        string s = "shift>\n";
        s += exprs[0]->debugString();
        for (size_t i = 1; i < exprs.size(); ++i)
        {
            switch (ops[i])
            {
                case BIT_SLEFT: s += "\v<\v<\n"; break;
                case BIT_SRIGHT: s += "\v>\v>\n"; break;
                default: break;
            }
            s += exprs[i]->debugString();
        }
        s += "<\n";
        return s;
    }
};
class AddExpression : public SyntaxNode
{
    vector<SyntaxNode *> exprs;
    vector<TokenType> ops;

   public:
    static SyntaxNode *parse(Lexer &lex, Environment *env);
    virtual string debugString()
    {
        string s = "+->\n";
        s += exprs[0]->debugString();
        for (size_t i = 1; i < exprs.size(); ++i)
        {
            switch (ops[i])
            {
                case OP_ADD: s += "+\n"; break;
                case OP_SUB: s += "-\n"; break;
                default: break;
            }
            s += exprs[i]->debugString();
        }
        s += "<\n";
        return s;
    }
};
class MulExpression : public SyntaxNode
{
    vector<SyntaxNode *> exprs;
    vector<TokenType> ops;

   public:
    static SyntaxNode *parse(Lexer &lex, Environment *env);
    virtual string debugString()
    {
        string s = "*/%>\n";
        s += exprs[0]->debugString();
        for (size_t i = 1; i < exprs.size(); ++i)
        {
            switch (ops[i])
            {
                case OP_MUL: s += "*\n"; break;
                case OP_DIV: s += "/\n"; break;
                case OP_MOD: s += "%\n"; break;
                default: break;
            }
            s += exprs[i]->debugString();
        }
        s += "<\n";
        return s;
    }
};
class CastExpression : public SyntaxNode
{
    vector<TypeBase *> types;
    SyntaxNode *target;

   public:
    static SyntaxNode *parse(Lexer &lex, Environment *env);
};
// TODO: finish this!
class UnaryExpression : public SyntaxNode
{
    SyntaxNode *expr;
    TokenType op;

   public:
    static SyntaxNode *parse(Lexer &lex, Environment *env);
};
// TODO: finish this!
class PostfixExpression : public SyntaxNode
{
   public:
    static SyntaxNode *parse(Lexer &lex, Environment *env);
};
// TODO: finish this!
class PrimaryExpression : public SyntaxNode
{
    Token t;
    union {
        int ival;
        char *str;
        Symbol *symbol;
    };

   public:
    static SyntaxNode *parse(Lexer &lex, Environment *env);
    virtual string debugString()
    {
        stringstream ss;
        string s;
        ss << t << endl;
        ss >> s;
        return s;
    }
};
class ConstExpression : public SyntaxNode
{
   public:
    static Token eval(CondExpression *expr);
};

class Parser
{
    Environment env;

   public:
    void parse(Lexer &lex)
    {
        Environment::ParseGlobalDeclaration(lex, &env);
        env.debugPrint(lex);
    }
};
