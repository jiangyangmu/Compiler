#pragma once

#include <iostream>
#include <stack>
using namespace std;

#include "lexer.h"


// XXX: all tryParse() functions return 'nullptr' on failure!!!
// XXX: all parse() functions raise SyntaxError on failure!!!

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
class Symbol;


// --------- Type System --------
// 1. Representation 2. Declaration 3. Usage

// Part 1: Representation
// Type Information Data Structure
enum EType { T_BASE, T_INT, T_CHAR, T_FUNC };
class TBase
{
   public:
    virtual StringRef id() const = 0;
    virtual size_t msize() const = 0;  // size in memory, in bytes
    virtual void debugPrint() = 0;
};
class TInt : public TBase
{
   public:
    StringRef id() const { return StringRef("I"); }
    size_t msize() const { return 4; }
    void debugPrint() { cout << " int"; }
};
class TChar : public TBase
{
   public:
    StringRef id() const { return StringRef("C"); }
    size_t msize() const { return 1; }
    void debugPrint() { cout << " char"; }
};
class TFunction : public TBase
{
    //string signature;
    //TBase * rtype;
    // Compoundstatement * body;
    vector<Symbol> params;
   public:
    StringRef id() const
    {
        return StringRef("F");
    }
    size_t msize() const { return 1; }
    // TODO: consider declaration & definition
    // TODO: support variant length parameter
    static TFunction * tryParse(Lexer &lex);
    static TFunction * parse(Lexer &lex);
    void debugPrint() { cout << " function returns"; }
};
class TArray : public TBase
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
    static TArray * tryParse(Lexer &lex);
    static TArray * parse(Lexer &lex);
    void debugPrint()
    {
        cout << " array " << length << " of";
        if (etype != nullptr) etype->debugPrint();
    }
};
class TPointer : public TBase
{
    // TODO: PTR_VOLATILE
    enum PointerType { PTR_NORMAL, PTR_CONST };

    PointerType type;
    TBase * etype; // element type
   public:
    StringRef id() const
    {
        return StringRef("P");
    }
    size_t msize() const
    {
        return 8;
    }
    static TPointer * tryParse(Lexer &lex);
    void debugPrint()
    {
        cout << " pointer of";
        if (etype != nullptr) etype->debugPrint();
    }
};
// TODO: struct, enum, union

// Part 2: Declaration
// Specifier: type & storage
// Declarator: symbol and other properties
// Declaration: Specifier + Declarator
// TODO: initialization
enum TypeStorage { TS_AUTO, TS_TYPEDEF, TS_EXTERN, TS_REGISTER, TS_STATIC };
struct Specifier
{
    TBase * type;
    // type storage
    TypeStorage storage;
    // type qualifiers
    bool isconst; // volatile
    bool isunsigned;
    static Specifier * parse(Lexer &lex);
    static Specifier * tryParse(Lexer &lex);
    void debugPrint()
    {
        if (isconst)
        {
            cout << " const";
        }
        if (isunsigned)
        {
            cout << " unsigned";
        }
        type->debugPrint();
    }
};
enum DeclaratorType { DT_NONE, DT_SYMBOL, DT_ARRAY, DT_FUNCTION };
struct Declarator
{
    Declarator * child;
    TPointer * pointer;
    DeclaratorType type;
    union {
        TArray * array; // array
        TFunction * function; // function
        int id; // symbol
    };
    static Declarator * parse(Lexer &lex);
    static Declarator * tryParse(Lexer &lex);
    void debugPrint(Lexer &lex)
    {
        if (child != nullptr)
        {
            child->debugPrint(lex);
        }

        switch (type)
        {
            case DT_SYMBOL:
                cout << "id " << lex.symbolName(id) << " is";
                break;
            case DT_ARRAY:
                array->debugPrint();
                break;
            case DT_FUNCTION:
                function->debugPrint();
                break;
            default:
                break;
        }
        if (pointer != nullptr)
        {
            pointer->debugPrint();
        }
    }
};

// Symbol Data Structure
struct Symbol
{
    Specifier * specifier;
    Declarator * declarator;
    StringRef name;
    int reladdr; // relative address
};
class SymbolTable
{
    vector<Symbol> symbols;
    size_t stack_size;
    SymbolTable * parent;

   public:
    static void AddTable(SymbolTable * table);
    static void RemoveTable();
    static SymbolTable * tryParse(Lexer &lex);
    void debugPrint(Lexer &lex)
    {
        cout << "Symbol Table:" << endl;
        for (Symbol &s : symbols)
        {
            s.declarator->debugPrint(lex);
            s.specifier->debugPrint();
            cout << endl;
        }
    }
};

// Part 3: Usage
// .....

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
    SymbolTable * table;

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
class ConstExpression : public SyntaxNode
{
   public:
    static Token eval(CondExpression * expr);
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
