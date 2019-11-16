#include "RegexImpl.h"

namespace v2 {
namespace re {

ACCEPT_VISIT_IMPL(ConcatOperator)
ACCEPT_VISIT_IMPL(AlterOperator)
ACCEPT_VISIT_IMPL(KleeneStarOperator)
ACCEPT_VISIT_IMPL(ASCIICharacter)

static void FreeTree(TreeNode * pTree,
                     memory::GenericFreeListAllocator & allocator)
{
    if (pTree->pLeftTree)
        FreeTree(pTree->pLeftTree, allocator);
    if (pTree->pRightTree)
        FreeTree(pTree->pRightTree, allocator);
    pTree->pElement->~Element();
    allocator.Free(pTree->pElement);
    allocator.Free(pTree);
}

static void VisitTree(TreeNode * pTree,
                      Visitor & visitor)
{
    if (pTree->pLeftTree)
        VisitTree(pTree->pLeftTree, visitor);
    if (pTree->pRightTree)
        VisitTree(pTree->pRightTree, visitor);
    pTree->pElement->Accept(visitor);
}

Tree::Tree(TreeNode * root,
           memory::GenericFreeListAllocator & allocator)
    : pRoot(root)
    , allocator(allocator)
{
}

Tree::~Tree()
{
    FreeTree(pRoot, allocator);
}

void Tree::Accept(Visitor & visitor)
{
    VisitTree(pRoot, visitor);
}

#define CONTEXT_ALLOC(argType) new ((argType *)CompositorContext::Get().Allocator().Alloc(sizeof(argType))) argType()
#define CONTEXT_FREE(argAddr) (CompositorContext::Get().Allocator().Free(argAddr))

Compositor Ascii(int index)
{
    ASCIICharacter * pElement = CONTEXT_ALLOC(ASCIICharacter);
    pElement->index = index;

    TreeNode * pNode = CONTEXT_ALLOC(TreeNode);
    pNode->pLeftTree = nullptr;
    pNode->pRightTree = nullptr;
    pNode->pElement = pElement;

    Compositor cp;
    cp.pTree = pNode;
    return cp;
}
Compositor Concat(Compositor left, Compositor right)
{
    ConcatOperator * pElement = CONTEXT_ALLOC(ConcatOperator);

    TreeNode * pNode = CONTEXT_ALLOC(TreeNode);
    pNode->pLeftTree = left.pTree;
    pNode->pRightTree = right.pTree;
    pNode->pElement = pElement;

    Compositor cp;
    cp.pTree = pNode;
    return cp;
}
Compositor Alter(Compositor left, Compositor right)
{
    AlterOperator * pElement = CONTEXT_ALLOC(AlterOperator);

    TreeNode * pNode = CONTEXT_ALLOC(TreeNode);
    pNode->pLeftTree = left.pTree;
    pNode->pRightTree = right.pTree;
    pNode->pElement = pElement;

    Compositor cp;
    cp.pTree = pNode;
    return cp;
}
Compositor KleeneStar(Compositor inner)
{
    KleeneStarOperator * pElement = CONTEXT_ALLOC(KleeneStarOperator);

    TreeNode * pNode = CONTEXT_ALLOC(TreeNode);
    pNode->pLeftTree = inner.pTree;
    pNode->pRightTree = nullptr;
    pNode->pElement = pElement;

    Compositor cp;
    cp.pTree = pNode;
    return cp;
}
Tree Compositor::Get()
{
    return Tree(pTree, CompositorContext::Get().Allocator());
}

static CompositorContext * pCurrentContext = nullptr;
CompositorContext & CompositorContext::Get()
{
    ASSERT(pCurrentContext);
    return *pCurrentContext;
}

CompositorContext::CompositorContext()
{
    pOldContext = pCurrentContext;
    pCurrentContext = this;
}

CompositorContext::~CompositorContext()
{
    pCurrentContext = pOldContext;
}

}
}

#ifdef UNIT_TEST
#include "../UnitTest/UnitTest.h"
#include "../Memory/MemoryTrace.h"

using namespace v2;

class REToStringConverter : public re::Visitor
{
public:
    virtual void Visit(re::ConcatOperator &) override
    {
        ASSERT(arrStr.Count() >= 2);
        String s;
        String s1 = arrStr[arrStr.Count() - 2];
        String s2 = arrStr[arrStr.Count() - 1];
        s.Add('(').Append(s1).Append(s2).Add(')');
        arrStr.RemoveAt(arrStr.Count() - 2, 2);
        arrStr.Add(s);
    }
    virtual void Visit(re::AlterOperator &) override
    {
        ASSERT(arrStr.Count() >= 2);
        String s;
        String s1 = arrStr[arrStr.Count() - 2];
        String s2 = arrStr[arrStr.Count() - 1];
        s.Add('(').Append(s1).Add('|').Append(s2).Add(')');
        arrStr.RemoveAt(arrStr.Count() - 2, 2);
        arrStr.Add(s);
    }
    virtual void Visit(re::KleeneStarOperator &) override
    {
        ASSERT(arrStr.Count() >= 1);
        String s;
        s.Add('(').Append(arrStr[arrStr.Count() - 1]).Add(')').Add('*');
        arrStr.RemoveAt(arrStr.Count() - 1, 1);
        arrStr.Add(s);
    }
    virtual void Visit(re::ASCIICharacter & ch) override
    {
        arrStr.Add(String((char)ch.index, 1));
    }

    virtual ~REToStringConverter() override
    {
        std::cout << arrStr.RawData() << std::endl;
    }

    static String Convert(re::Tree & tree)
    {
        REToStringConverter cvt;
        tree.Accept(cvt);
        return cvt.arrStr[0];
    }

private:
    containers::Array<String> arrStr;
};

TEST(Regex_API_Build)
{
    re::CompositorContext context;

    // abc*|d
    re::Compositor cp =
        re::Alter(
            re::Concat(re::Concat(
                re::Ascii('a'),
                re::Ascii('b')),
                re::KleeneStar(re::Ascii('c'))),
            re::Ascii('d'));

    re::Tree re = cp.Get();

    EXPECT_EQ(
        REToStringConverter::Convert(re),
        String("(((ab)(c)*)|d)"));
}

#endif