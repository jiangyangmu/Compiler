#pragma once

#include <iostream>
#include <stack>
using namespace std;

#include "lexer.h"

// helplers
bool IsDeclaration(TokenType t);
bool IsAssignOperator(TokenType t);
bool IsUnaryOperator(TokenType t);
void SyntaxError(const char *msg);

struct Declarator;
class Expression;
class AssignExpression;
class CondExpression;
class OrExpression;
class AndExpression;
class BitOrExpression;
class BitXorExpression;
class BitAndExpression;
class EqExpression;
class RelExpression;
class ShiftExpression;
class AddExpression;
class MulExpression;
class CastExpression;
class UnaryExpression;
class PostfixExpression;
class PrimaryExpression;

// --------- Declarator Parser --------

typedef enum SpecType {
    SPEC_NONE,
    SPEC_VOID,
    SPEC_CHAR,
    SPEC_SHORT,
    SPEC_INT,
    SPEC_LONG,
    SPEC_FLOAT,
    SPEC_DOUBLE
} SpecType;
struct Specifier
{
    bool isconst, isstatic, issigned;
    SpecType stype;

    static Specifier *ParseSpecifier(Lexer &lex);
    static bool MaybeTypeName(TokenType t);
    void print();
};
typedef enum PointerType {
    PTR_NORMAL = 0,
    PTR_CONST = 1  // , PTR_VOLATILE = 2
} PointerType;
struct Pointer
{
    vector<int> ptypes;

    static Pointer ParsePointer(Lexer &lex);

    void print();
};
struct Function
{
    vector<Specifier *> specifiers;
    vector<Declarator *> decls;

    static Function ParseFunction(Lexer &lex);
};
struct Array
{
    int length;

    static Array ParseArray(Lexer &lex);
};
typedef enum DeclType { DT_NONE, DT_ID, DT_ARRAY, DT_FUNCTION } DeclType;
struct Declarator
{
    Declarator *child;
    Pointer *pointer;
    DeclType dtype;
    union {
        Array *array;
        Function *function;
        int id; /* lex.symid */
    };

    static Declarator *ParseDeclarator(Lexer &lex);
    void print();
};
struct SymbolDecl
{
    // L: sepcifier+pointer
    // M: id|(decl)
    // R: array | function
    // D -> P(MR)
    // M -> (D)
    // <Pointer, Array|Function|Id>
    Specifier *specifier;
    Declarator *declarator;

    // calculated from specifier & declarator
    int mem_size;
    StringRef name;

    void print();
    // operations: every type should have their own operation set
};

// --------- Type System --------
// TODO: functions that extract TypeInfo and Object(Symbol) from source code,
// so it's easy to build SymbolTable.
// SEE: Declarator Parser above, which will be removed after this finish.
typedef enum EType { T_BASE, T_INT, T_CHAR, T_FUNC } EType;
class TBase
{
   public:
    virtual StringRef id() const = 0;
    virtual size_t msize() const = 0;  // size in memory, in bytes
};
class TInt : class TBase
{
   public:
    StringRef id() const { return "I"; }
    size_t msize() const { return 4; }
};
class TChar : class TBase
{
   public:
    StringRef id() const { return "C"; }
    size_t msize() const { return 1; }
};
class TFunction : class TBase
{
    string signature;
   public:
    StringRef id() const
    {
        return StringRef(signature.data(), signature.size());
    }
    size_t msize() const { return 1; }
};
class TArray : class TBase
{
    size_t length;
    string alenstr; // "A" + to_string(length)
    TBase * etype; // element type
   public:
    StringRef id() const
    {
        return StringRef(alenstr.data(), alenstr.size());
    }
    size_t msize() const
    {
        // TODO: may overflow
        return etype->msize() * length;
    }
};
// TODO: struct, enum

struct Object
{
    TBase * type;
    StringRef name;
    int reladdr; // relative address
};

class SymbolTable
{
    // array of < type , name , rel-addr >
    // total stack size
    vector<Object> objects;
    size_t stack_size;
};

class SyntaxNode {};

class TranslationUnit : public SyntaxNode
{
   public:
    static SyntaxNode *parse(Lexer &lex);
};

// TODO: implement 'abstract-decl' part
class TypeName : public SyntaxNode
{
   public:
    static SyntaxNode *parse(Lexer &lex);
};

// no instance, only dispatch
class Statement : public SyntaxNode
{
   public:
    static SyntaxNode *parse(Lexer &lex);
};
class LabelStatement : public SyntaxNode
{
   public:
    static SyntaxNode *parse(Lexer &lex);
};
class CompoundStatement : public SyntaxNode
{
    vector<SyntaxNode *> stmts;

   public:
    static SyntaxNode *parse(Lexer &lex);
};
// no instance, dispatch only
class ExpressionStatement : public SyntaxNode
{
   public:
    static SyntaxNode *parse(Lexer &lex);
};
class SelectionStatement : public SyntaxNode
{
    SyntaxNode *expr;
    SyntaxNode *stmt;
    SyntaxNode *stmt2;

   public:
    static SyntaxNode *parse(Lexer &lex);
};
class IterationStatement : public SyntaxNode
{
    typedef enum IterationType { WHILE_LOOP,
                                 DO_LOOP,
                                 FOR_LOOP } IterationType;

    IterationType type;
    SyntaxNode *expr;
    SyntaxNode *expr2;
    SyntaxNode *expr3;
    SyntaxNode *stmt;

   public:
    static SyntaxNode *parse(Lexer &lex);
};
class JumpStatement : public SyntaxNode
{
    typedef enum JumpType { JMP_GOTO,
                            JMP_CONTINUE,
                            JMP_BREAK,
                            JMP_RETURN } JumpType;

    JumpType type;
    SyntaxNode *expr;
    int id;  // for goto Label
   public:
    static SyntaxNode *parse(Lexer &lex);
};

class Expression : public SyntaxNode
{
    vector<SyntaxNode *> exprs;

   public:
    static SyntaxNode *parse(Lexer &lex);
};
// TODO: fix this class, only works for most cases for now
class AssignExpression : public SyntaxNode
{
    // treat unary as condition
    //vector<UnaryExpression *> targets;
    vector<SyntaxNode *> targets;
    SyntaxNode *source;

   public:
    static SyntaxNode *parse(Lexer &lex);
};
class CondExpression : public SyntaxNode
{
    SyntaxNode *cond;
    SyntaxNode *left;
    SyntaxNode *right;

   public:
    static SyntaxNode *parse(Lexer &lex);
};
class OrExpression : public SyntaxNode
{
    vector<SyntaxNode *> exprs;

   public:
    static SyntaxNode *parse(Lexer &lex);
};
class AndExpression : public SyntaxNode
{
    vector<SyntaxNode *> exprs;

   public:
    static SyntaxNode *parse(Lexer &lex);
};
class BitOrExpression : public SyntaxNode
{
    vector<SyntaxNode *> exprs;

   public:
    static SyntaxNode *parse(Lexer &lex);
};
class BitXorExpression : public SyntaxNode
{
    vector<SyntaxNode *> exprs;

   public:
    static SyntaxNode *parse(Lexer &lex);
};
class BitAndExpression : public SyntaxNode
{
    vector<SyntaxNode *> exprs;

   public:
    static SyntaxNode *parse(Lexer &lex);
};
class EqExpression : public SyntaxNode
{
    vector<SyntaxNode *> exprs;
    vector<TokenType> ops;

   public:
    static SyntaxNode *parse(Lexer &lex);
};
class RelExpression : public SyntaxNode
{
    vector<SyntaxNode *> exprs;
    vector<TokenType> ops;

   public:
    static SyntaxNode *parse(Lexer &lex);
};
class ShiftExpression : public SyntaxNode
{
    vector<SyntaxNode *> exprs;
    vector<TokenType> ops;

   public:
    static SyntaxNode *parse(Lexer &lex);
};
class AddExpression : public SyntaxNode
{
    vector<SyntaxNode *> exprs;
    vector<TokenType> ops;

   public:
    static SyntaxNode *parse(Lexer &lex);
};
class MulExpression : public SyntaxNode
{
    vector<SyntaxNode *> exprs;
    vector<TokenType> ops;

   public:
    static SyntaxNode *parse(Lexer &lex);
};
class CastExpression : public SyntaxNode
{
    vector<SyntaxNode *> types;
    SyntaxNode *target;

   public:
    static SyntaxNode *parse(Lexer &lex);
};
// TODO: finish this!
class UnaryExpression : public SyntaxNode
{
    SyntaxNode *expr;
    TokenType op;

   public:
    static SyntaxNode *parse(Lexer &lex);
};
// TODO: finish this!
class PostfixExpression : public SyntaxNode
{
   public:
    static SyntaxNode *parse(Lexer &lex);
};
// TODO: finish this!
class PrimaryExpression : public SyntaxNode
{
   public:
    static SyntaxNode *parse(Lexer &lex);
};

class Parser
{
    SyntaxNode *root;

   public:
    void parse(Lexer &lex)
    {
        root = TranslationUnit::parse(lex);
    }
};
