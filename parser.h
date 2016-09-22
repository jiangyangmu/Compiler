#pragma once

#include <stack>
#include <iostream>
using namespace std;

#include "lexer.h"

// helplers
bool IsDeclaration(TokenType t);
bool IsAssignOperator(TokenType t);
bool IsUnaryOperator(TokenType t);
void SyntaxError(const char * msg);

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

typedef enum SpecType
{
    SPEC_NONE, SPEC_VOID, SPEC_CHAR, SPEC_SHORT, SPEC_INT, SPEC_LONG, SPEC_FLOAT, SPEC_DOUBLE
} SpecType;
struct Specifier
{
    bool isconst, isstatic, issigned;
    SpecType stype;

    static Specifier * ParseSpecifier(Lexer &lex);
    static bool MaybeTypeName(TokenType t);
    void print();
};

typedef enum PointerType
{
    PTR_NORMAL = 0, PTR_CONST = 1 // , PTR_VOLATILE = 2
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

typedef enum DeclType
{
    DT_NONE, DT_ID, DT_ARRAY, DT_FUNCTION
} DeclType;
struct Declarator
{
    Declarator * child;
    Pointer * pointer;
    DeclType dtype;
    union
    {
        Array * array;
        Function * function;
        int id; /* lex.symid */
    };

    static Declarator * ParseDeclarator(Lexer &lex);
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
    Specifier * specifier;
    Declarator * declarator;

    // calculated from specifier & declarator
    int mem_size;
    StringRef name;

    void print();
    // operations: every type should have their own operation set
};

class SymbolTable
{
    vector<SymbolDecl> symbols;
public:
    void add_symbol(Lexer &Lex);
    bool search(StringBuf &name);
};


class SyntaxNode
{
public:
    //virtual SyntaxNode * parse(Lexer &) = 0;
    //SyntaxType type() = 0;
};

class TranslationUnit : public SyntaxNode
{
public:
    static SyntaxNode * parse(Lexer &lex);
};

// TODO: implement 'abstract-decl' part
class TypeName : public SyntaxNode
{
public:
    static SyntaxNode * parse(Lexer &lex);
};

// no instance, only dispatch
class Statement : public SyntaxNode
{
public:
    static SyntaxNode * parse(Lexer &lex);
};
class LabelStatement : public SyntaxNode
{
public:
    static SyntaxNode * parse(Lexer &lex);
};
class CompoundStatement : public SyntaxNode
{
    vector<SyntaxNode *> stmts;
public:
    static SyntaxNode * parse(Lexer &lex);
};
// no instance, dispatch only
class ExpressionStatement : public SyntaxNode
{
public:
    static SyntaxNode * parse(Lexer &lex);
};
class SelectionStatement : public SyntaxNode
{
    SyntaxNode * expr;
    SyntaxNode * stmt;
    SyntaxNode * stmt2;
public:
    static SyntaxNode * parse(Lexer &lex);
};
class IterationStatement : public SyntaxNode
{
    typedef enum IterationType { WHILE_LOOP, DO_LOOP, FOR_LOOP } IterationType;

    IterationType type;
    SyntaxNode * expr;
    SyntaxNode * expr2;
    SyntaxNode * expr3;
    SyntaxNode * stmt;
public:
    static SyntaxNode * parse(Lexer &lex);
};
class JumpStatement : public SyntaxNode
{
    typedef enum JumpType { JMP_GOTO, JMP_CONTINUE, JMP_BREAK, JMP_RETURN } JumpType;

    JumpType type;
    SyntaxNode * expr;
    int id; // for goto Label
public:
    static SyntaxNode * parse(Lexer &lex);
};

class Expression : public SyntaxNode
{
    vector<SyntaxNode *> exprs;
public:
    static SyntaxNode * parse(Lexer &lex);
};
// TODO: fix this class, only works for most cases for now
class AssignExpression : public SyntaxNode
{
    // treat unary as condition
    //vector<UnaryExpression *> targets;
    vector<SyntaxNode *> targets;
    SyntaxNode * source;
public:
    static SyntaxNode * parse(Lexer &lex);
};
class CondExpression : public SyntaxNode
{
    SyntaxNode * cond;
    SyntaxNode * left;
    SyntaxNode * right;
public:
    static SyntaxNode * parse(Lexer &lex);
};
class OrExpression : public SyntaxNode
{
    vector<SyntaxNode *> exprs;
public:
    static SyntaxNode * parse(Lexer &lex);
};
class AndExpression : public SyntaxNode
{
    vector<SyntaxNode *> exprs;
public:
    static SyntaxNode * parse(Lexer &lex);
};
class BitOrExpression : public SyntaxNode
{
    vector<SyntaxNode *> exprs;
public:
    static SyntaxNode * parse(Lexer &lex);
};
class BitXorExpression : public SyntaxNode
{
    vector<SyntaxNode *> exprs;
public:
    static SyntaxNode * parse(Lexer &lex);
};
class BitAndExpression : public SyntaxNode
{
    vector<SyntaxNode *> exprs;
public:
    static SyntaxNode * parse(Lexer &lex);
};
class EqExpression : public SyntaxNode
{
    vector<SyntaxNode *> exprs;
    vector<TokenType> ops;
public:
    static SyntaxNode * parse(Lexer &lex);
};
class RelExpression : public SyntaxNode
{
    vector<SyntaxNode *> exprs;
    vector<TokenType> ops;
public:
    static SyntaxNode * parse(Lexer &lex);
};
class ShiftExpression : public SyntaxNode
{
    vector<SyntaxNode *> exprs;
    vector<TokenType> ops;
public:
    static SyntaxNode * parse(Lexer &lex);
};
class AddExpression : public SyntaxNode
{
    vector<SyntaxNode *> exprs;
    vector<TokenType> ops;
public:
    static SyntaxNode * parse(Lexer &lex);
};
class MulExpression : public SyntaxNode
{
    vector<SyntaxNode *> exprs;
    vector<TokenType> ops;
public:
    static SyntaxNode * parse(Lexer &lex);
};
class CastExpression : public SyntaxNode
{
    vector<SyntaxNode *> types;
    SyntaxNode * target;
public:
    static SyntaxNode * parse(Lexer &lex);
};
// TODO: finish this!
class UnaryExpression : public SyntaxNode
{
public:
    static SyntaxNode * parse(Lexer &lex);
};
// TODO: finish this!
class PostfixExpression : public SyntaxNode
{
public:
    static SyntaxNode * parse(Lexer &lex);
};
// TODO: finish this!
class PrimaryExpression : public SyntaxNode
{
public:
    static SyntaxNode * parse(Lexer &lex);
};

class Parser
{
    SyntaxNode * root;
public:
    void parse(Lexer &lex)
    {
        root = TranslationUnit::parse(lex);
    }
};
