#pragma once

#include <iostream>
#include <stack>
using namespace std;

#include "lexer.h"
#include "env.h"
#include "codegen.h"

// XXX: all tryParse() functions return 'nullptr' on failure!!!
// XXX: all parse() functions raise SyntaxError on failure!!!

// helplers
bool IsDeclaration(TokenType t);
bool IsAssignOperator(TokenType t);
bool IsUnaryOperator(TokenType t);

class SyntaxNode : public CodeGenerator
{
   public:
    virtual string debugString() { return "~<?~>"; }
    virtual void emit(const Environment *env) const { Emit("<emit not implemented>"); }
};
class Expression : public SyntaxNode
{
   protected:
    TypeBase *type_;
   public:
    Expression() : type_(nullptr) {}
    // Expression(TypeBase *t) type_(t) { assert(type_ != nullptr); }
    virtual TypeBase *type() const { return type_; }
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
    virtual string debugString()
    {
        return "~<LabelStatement~>";
    }
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
        s += "<\n}\n";
        return s;
    }
    virtual void emit(const Environment *__not_used) const
    {
        env->emit();
        for (const SyntaxNode *stmt : stmts)
        {
            stmt->emit(env);
        }
    }
};
// no instance, dispatch only
class ExpressionStatement : public SyntaxNode
{
    Expression *expr;

   public:
    static SyntaxNode *parse(Lexer &lex, Environment *env);
    virtual string debugString()
    {
        return expr->debugString();
    }
    virtual void emit(const Environment *env) const
    {
        expr->emit(env);
        Emit("pop");
    }
};
// TODO: implement switch
class SelectionStatement : public SyntaxNode
{
    Expression *expr;
    SyntaxNode *stmt;
    SyntaxNode *stmt2;

   public:
    static SyntaxNode *parse(Lexer &lex, Environment *env);
    virtual string debugString()
    {
        string s;
        // switch (type)
        // {
        //     case IF: s += "if"; break;
        //     case SWITCH: s += "switch"; break;
        //     default: break;
        // }
        s += "if>\n";
        s += expr->debugString();
        s += "<then>\n";
        s += stmt->debugString();
        if (stmt2)
        {
            s += "<else>\n";
            s += stmt2->debugString();
        }
        s += "<\n";
        return s;
    }
    virtual void emit(const Environment *env) const
    {
        expr->emit(env);
        Emit("jmp_if L1");
        Emit("jmp L2");
        Emit("L1:");
        stmt->emit(env);
        Emit("jmp L3");
        Emit("L2:");
        if (stmt2)
        {
            stmt2->emit(env);
        }
        Emit("L3:");
    }
};
class IterationStatement : public SyntaxNode
{
    typedef enum IterationType { WHILE_LOOP, DO_LOOP, FOR_LOOP } IterationType;

    IterationType type;
    Expression *expr;
    Expression *expr2;
    Expression *expr3;
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
        if (stmt) s += stmt->debugString();
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
    Expression *expr;
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
    virtual void emit(const Environment *env) const
    {
        expr->emit(env);
        Emit("ret");
    }
};

class CommaExpression : public Expression
{
    // vector<Expression *> exprs;
    Expression *curr, *next;

   public:
    static Expression *parse(Lexer &lex, Environment *env);
    virtual string debugString()
    {
        string s = ",>\n";
        s += curr->debugString();
        if (next) s += next->debugString();
        s += "<\n";
        return s;
    }
};
// TODO: fix this class, only works for most cases for now
class AssignExpression : public Expression
{
    // treat unary as condition
    // vector<UnaryExpression *> targets;
    Expression *target, *source;
    TokenType op;

   public:
    static Expression *parse(Lexer &lex, Environment *env);
    virtual string debugString()
    {
        string s = "=>\n";
        // for (SyntaxNode *t : targets) s += t->debugString();
        if (target) s += target->debugString();
        if (source) s += source->debugString();
        s += "<\n";
        return s;
    }
    virtual void emit(const Environment *env) const
    {
        target->emit(env); // target address on stack top
        source->emit(env);
        Emit("assign");
    }
};
class CondExpression : public Expression
{
    Expression *cond;
    Expression *left;
    Expression *right;

   public:
    static Expression *parse(Lexer &lex, Environment *env);
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
class OrExpression : public Expression
{
    Expression *left, *right;

   public:
    static Expression *parse(Lexer &lex, Environment *env);
    virtual string debugString()
    {
        string s = "||>\n";
        if (left) s += left->debugString();
        if (right) s += right->debugString();
        s += "<\n";
        return s;
    }
};
class AndExpression : public Expression
{
    Expression *left, *right;

   public:
    static Expression *parse(Lexer &lex, Environment *env);
    virtual string debugString()
    {
        string s = "&&>\n";
        if (left) s += left->debugString();
        if (right) s += right->debugString();
        s += "<\n";
        return s;
    }
};
class BitOrExpression : public Expression
{
    Expression *left, *right;

   public:
    static Expression *parse(Lexer &lex, Environment *env);
    virtual string debugString()
    {
        string s = "|>\n";
        if (left) s += left->debugString();
        if (right) s += right->debugString();
        s += "<\n";
        return s;
    }
};
class BitXorExpression : public Expression
{
    Expression *left, *right;

   public:
    static Expression *parse(Lexer &lex, Environment *env);
    virtual string debugString()
    {
        string s = "^>\n";
        if (left) s += left->debugString();
        if (right) s += right->debugString();
        s += "<\n";
        return s;
    }
};
class BitAndExpression : public Expression
{
    Expression *left, *right;

   public:
    static Expression *parse(Lexer &lex, Environment *env);
    virtual string debugString()
    {
        string s = "&>\n";
        if (left) s += left->debugString();
        if (right) s += right->debugString();
        s += "<\n";
        return s;
    }
};
class EqExpression : public Expression
{
    Expression *left, *right;
    TokenType op;

   public:
    static Expression *parse(Lexer &lex, Environment *env);
    virtual string debugString()
    {
        string s;
        if (op == REL_EQ) s += "==";
        else s += "!=";
        s += ">\n";
        if (left) s += left->debugString();
        if (right) s += right->debugString();
        s += "<\n";
        return s;
    }
};
class RelExpression : public Expression
{
    Expression *left, *right;
    TokenType op;

   public:
    static Expression *parse(Lexer &lex, Environment *env);
    virtual string debugString()
    {
        string s;
        switch (op)
        {
            case REL_GT: s += "~>\n"; break;
            case REL_GE: s += "~>=\n"; break;
            case REL_LT: s += "~<\n"; break;
            case REL_LE: s += "~<=\n"; break;
            default: break;
        }
        s += ">\n";
        if (left) s += left->debugString();
        if (right) s += right->debugString();
        s += "<\n";
        return s;
    }
};
class ShiftExpression : public Expression
{
    Expression *left, *right;
    TokenType op;

   public:
    static Expression *parse(Lexer &lex, Environment *env);
    virtual string debugString()
    {
        string s;
        switch (op)
        {
            case BIT_SLEFT: s += "~<~<\n"; break;
            case BIT_SRIGHT: s += "~>~>\n"; break;
            default: break;
        }
        s += ">\n";
        if (left) s += left->debugString();
        if (right) s += right->debugString();
        s += "<\n";
        return s;
    }
};
class AddExpression : public Expression
{
    Expression *left, *right;
    TokenType op;

   public:
    static Expression *parse(Lexer &lex, Environment *env,
                             Expression *left = nullptr);
    virtual string debugString()
    {
        string s;
        switch (op)
        {
            case OP_ADD: s += "+\n"; break;
            case OP_SUB: s += "-\n"; break;
            default: break;
        }
        s += ">\n";
        if (left) s += left->debugString();
        if (right) s += right->debugString();
        s += "<\n";
        return s;
    }
};
class MulExpression : public Expression
{
    Expression *left, *right;
    TokenType op;

   public:
    static Expression *parse(Lexer &lex, Environment *env);
    virtual string debugString()
    {
        string s;
        switch (op)
        {
            case OP_MUL: s += "*\n"; break;
            case OP_DIV: s += "/\n"; break;
            case OP_MOD: s += "%\n"; break;
            default: break;
        }
        s += ">\n";
        if (left) s += left->debugString();
        if (right) s += right->debugString();
        s += "<\n";
        return s;
    }
};
class CastExpression : public Expression
{
    vector<TypeBase *> types;
    Expression *target;

   public:
    static Expression *parse(Lexer &lex, Environment *env);
};
// TODO: finish this!
class UnaryExpression : public Expression
{
    Expression *expr;
    TokenType op;

   public:
    static Expression *parse(Lexer &lex, Environment *env);
};
// TODO: finish this!
class PostfixExpression : public Expression
{
    typedef enum PostfixOperation {
        POSTFIX_INDEX,
        POSTFIX_CALL,
        POSTFIX_OBJECT_OFFSET,
        POSTFIX_POINTER_OFFSET,
        POSTFIX_INC,
        POSTFIX_DEC
    } PostfixOperation;
    PostfixOperation op;
    Expression *target;
    union {
        Expression *index; // array index
        vector<Expression *> params; // function arguments
        StringRef member; // member id
    };

   public:
    PostfixExpression() : params() {}
    static Expression *parse(Lexer &lex, Environment *env, Expression *target);
    virtual string debugString()
    {
        assert( target != nullptr );
        string s;
        switch (op)
        {
            case POSTFIX_INDEX:
                s += "[]>\n";
                s += index->debugString();
                break;
            case POSTFIX_CALL:
                s += "()>\n";
                for (Expression *e : params)
                    s += e->debugString();
                break;
            case POSTFIX_OBJECT_OFFSET:
                s += ".>\n";
                s += member.toString();
                break;
            case POSTFIX_POINTER_OFFSET:
                s += "-~>>\n";
                s += member.toString();
                break;
            case POSTFIX_INC:
                s += "++>\n";
                break;
            case POSTFIX_DEC:
                s += "-->\n";
                break;
            default:
                break;
        }
        s += target->debugString();
        s += "<\n";
        return s;
    }
    virtual void emit(const Environment *env) const
    {
        switch (op)
        {
            case POSTFIX_CALL:
                for (auto it = params.rbegin(); it != params.rend(); ++it)
                    (*it)->emit(env);
                target->emit(env);
                Emit("call");
                for (size_t i = 0; i < params.size(); ++i)
                    Emit("pop");
                break;
            default:
                SyntaxError("PostfixExpression: not implemented");
                break;
        }
    }
};
// TODO: finish this!
class PrimaryExpression : public Expression
{
    Token t;
    union {
        int ival;
        StringRef *str;
        Symbol *symbol;
    };

   public:
    static Expression *parse(Lexer &lex, Environment *env);
    virtual string debugString()
    {
        string s;
        switch (t.type)
        {
            case SYMBOL:
                s += symbol->name.toString();
                break;
            case CONST_INT:
                s += to_string(ival);
                break;
            case STRING:
                s += str->toString();
                break;
            default:
                break;
        }
        s += '\n';
        return s;
    }
    virtual void emit(const Environment *env) const
    {
        switch (t.type)
        {
            case CONST_INT: Emit("push %d", ival); break;
            case STRING:
                Emit("push str(\"%s\")", str->toString().data());
                break;
            case SYMBOL:
                Emit("push addr(%s)", symbol->name.toString().data());
                break;
            default: Emit("<unknown PrimaryExpression>"); break;
        }
    }
};
class ConstExpression : public Expression
{
   public:
    static Token eval(CondExpression *expr);
};

class Parser
{
    Environment env;
    Lexer &lex;

   public:
    Parser(Lexer &l) : lex(l)
    {
    }
    void parse()
    {
        Environment::ParseGlobalDeclaration(lex, &env);
    }
    void debugPrint() const
    {
        env.debugPrint(lex);
    }
    void emit() const
    {
        env.emit();
    }
};
