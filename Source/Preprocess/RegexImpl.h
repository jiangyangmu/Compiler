#pragma once

#include "../Base/Containers.h"
#include "../Base/String.h"
#include "../Memory/FreeListAllocator.h"

namespace v2 {

namespace re {


class ConcatOperator;
class AlterOperator;
class KleeneStarOperator;
class ASCIICharacter;

// Regex structure visitor
class Visitor
{
public:
    virtual ~Visitor() = default;

    // Operators
    virtual void Visit(ConcatOperator &) {}
    virtual void Visit(AlterOperator &) {}
    virtual void Visit(KleeneStarOperator &) {}

    // Elements
    virtual void Visit(ASCIICharacter &) {}
};

#define ACCEPT_VISIT_INTERFACE virtual void Accept(Visitor & visitor) = 0;
#define ACCEPT_VISIT virtual void Accept(Visitor & visitor) { visitor.Visit(*this); }

class Host
{
public:
    virtual ~Host() = default;

    ACCEPT_VISIT_INTERFACE
};

// Regex structure

class TreeNode : public Host {};
using TreeNodeList = Array<TreeNode *>;

class Character : public TreeNode {};
class UnaryOperation : public TreeNode
{
private:
    TreeNode * pTree;
};
class BinaryOperation : public TreeNode
{
private:
    TreeNode * pLeftTree;
    TreeNode * pRightTree;
};

class ConcatOperator : public BinaryOperation
{
public:
    ACCEPT_VISIT
};
class AlterOperator : public BinaryOperation
{
public:
    ACCEPT_VISIT
};
class KleeneStarOperator : public UnaryOperation
{
public:
    ACCEPT_VISIT
};
class ASCIICharacter : public Character
{
public:
    using CharIndex = int;

    CharIndex Index();

    ACCEPT_VISIT
};

class Tree
{
public:
    ~Tree(); // free all nodes

    void Accept(Visitor & visitor);

private:
    TreeNode * pRoot;
    memory::FreeListAllocator & allocator;
};

// Regex structure compositor
class Compositor
{
public:
    Tree Get();
private:
    TreeNode * pRoot;
    // Use CompositorContext::Get().allocator.
};
Compositor Ascii(ASCIICharacter::CharIndex index);
Compositor Concat(Compositor left, Compositor right);
Compositor Alter(Compositor left, Compositor right);
Compositor KleeneStar(Compositor left);

class CompositorContext
{
public:
    static CompositorContext & Get();
    static void Set(CompositorContext &);

    CompositorContext();

private:
    memory::FreeListAllocator allocator;
};

}

}
