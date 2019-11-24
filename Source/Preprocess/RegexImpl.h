#pragma once

#include "../Base/Containers.h"
#include "../Base/String.h"
#include "../Memory/FreeListAllocator.h"

#include <memory>

namespace v2 {

namespace re {

using Allocator = ::memory::GenericFreeListAllocator;

class Visitor;

#define ACCEPT_VISIT_INTERFACE  virtual void Accept(Visitor & visitor) = 0;
#define ACCEPT_VISIT_DECL       virtual void Accept(Visitor & visitor) override;
#define ACCEPT_VISIT_IMPL(type) void type::Accept(Visitor & visitor) { visitor.Visit(*this); }

// Charset

using CharIndex = int;
#define CHAR_FIRST (0)
#define CHAR_EPSILON (256 + 1)
#define CHAR_EOS (256 + 2)
#define CHAR_LAST CHAR_EOS
#define CHAR_COUNT (CHAR_LAST + 1)

// Regex elements

class Element
{
public:
    virtual ~Element() = default;

    ACCEPT_VISIT_INTERFACE
};

class ConcatOperator : public Element
{
public:
    ACCEPT_VISIT_DECL
};
class AlterOperator : public Element
{
public:
    ACCEPT_VISIT_DECL
};
class KleeneStarOperator : public Element
{
public:
    ACCEPT_VISIT_DECL
};
class ASCIICharacter : public Element
{
public:
    ACCEPT_VISIT_DECL
    CharIndex index;
};

// Regex visitor

class Visitor
{
public:
    virtual ~Visitor() = default;

    // Operators
    virtual void Visit(ConcatOperator &) {}
    virtual void Visit(AlterOperator &) {}
    virtual void Visit(KleeneStarOperator &) {}

    // Chars
    virtual void Visit(ASCIICharacter &) {}
};

// Regex structure

// Memory management of TreeNode and Element:
// 1. Allocate by CompositorContext::allocator
// 2. Free by Tree::~Tree()

struct TreeNode
{
    TreeNode * pLeftTree;
    TreeNode * pRightTree;
    Element * pElement;
};

class RegexTree
{
public:
    ~RegexTree();

    void Accept(Visitor & visitor);

private:
    RegexTree(TreeNode * root, Allocator & allocator);

    TreeNode * pRoot;
    Allocator & allocator;

    friend class Compositor;
};

// Regex structure compositor

class Compositor
{
public:
    RegexTree Get();

private:
    TreeNode * pTree;
    // Use CompositorContext allocator.

    friend Compositor Ascii(int index);
    friend Compositor Concat(Compositor left, Compositor right);
    friend Compositor Alter(Compositor left, Compositor right);
    friend Compositor KleeneStar(Compositor inner);
};
Compositor Ascii(int index);
Compositor Concat(Compositor left, Compositor right);
Compositor Alter(Compositor left, Compositor right);
Compositor KleeneStar(Compositor inner);

class CompositorContext
{
public:
    static CompositorContext & Get();

    CompositorContext();
    ~CompositorContext();

    Allocator & GetAllocator() { return allocator; }

private:
    CompositorContext * pOldContext;
    Allocator allocator;

    static CompositorContext * pCurrentContext;
};

// Client interface

class Regex
{
public:
    // TODO: full type operation set impl.
    // TODO: dfa destroy.

    class Impl;
private:
    std::unique_ptr<Impl> pImpl;

    friend class PImplAccessor;
};

class MatchResult
{
public:
    MatchResult(String content, int maxScanLen, int maxAcceptLen);

    StringView Content() const;
    int MaxScanLen() const;
    int MaxAcceptLen() const;

private:
    String content;
    int maxScanLen;
    int maxAcceptLen;
};

class MatchResultIterator
{
public:
    bool More();
    MatchResult Next();

    class Impl;
private:
    std::unique_ptr<Impl> pImpl;

    friend class PImplAccessor;
};

Regex Compile(StringView regex);
MatchResultIterator IterateMatches(const Regex & regex, StringView input);

}

}
