#include "RegexImpl.h"

#include <set>
#include <deque>
#include <map>

namespace v2 {
namespace re {

using ::containers::Array;

ACCEPT_VISIT_IMPL(ConcatOperator)
ACCEPT_VISIT_IMPL(AlterOperator)
ACCEPT_VISIT_IMPL(KleeneStarOperator)
ACCEPT_VISIT_IMPL(ASCIICharacter)

static void FreeTree(TreeNode * pTree,
                     Allocator & allocator)
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
           Allocator & allocator)
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

#define CONTEXT_ALLOC(argType) new ((argType *)CompositorContext::Get().GetAllocator().Alloc(sizeof(argType))) argType()
#define CONTEXT_FREE(argAddr) (CompositorContext::Get().GetAllocator().Free(argAddr))

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
    return Tree(pTree, CompositorContext::Get().GetAllocator());
}

CompositorContext * CompositorContext::pCurrentContext = nullptr;
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

// Nfa structure

struct NfaNode
{
    CharIndex ch1;
    CharIndex ch2;
    NfaNode * out1;
    NfaNode * out2;
};

struct Nfa
{
    NfaNode * in;
    NfaNode * out;
};

class NfaContext
{
public:
    static NfaContext & Get()
    {
        ASSERT(pCurrentContext);
        return *pCurrentContext;
    }

    NfaContext()
        : pOldContext(pCurrentContext)
    {
        pCurrentContext = this;
    }
    ~NfaContext()
    {
        pCurrentContext = pOldContext;
    }

    Allocator & GetAllocator() { return allocator; }

private:
    NfaContext * pOldContext;
    Allocator allocator;

    static NfaContext * pCurrentContext;
};
NfaContext * NfaContext::pCurrentContext = nullptr;

#define NFA_CONTEXT_ALLOC(argType) (argType *)NfaContext::Get().GetAllocator().Alloc(sizeof(argType))
#define NFA_CONTEXT_FREE(argAddr) (NfaContext::Get().GetAllocator().Free(argAddr))

// Nfa structure builder

class NfaConverter : public re::Visitor
{
public:
    static Nfa Convert(re::Tree & tree)
    {
        NfaConverter cvt;
        tree.Accept(cvt);
        ASSERT(cvt.arrNfa.Count() == 1);

        // Add end-of-input node.
        NfaNode * eos = cvt.NewNode();
        NfaNode * eosMore = cvt.NewNode();
        
        eos->ch1 = CHAR_EOS;
        eos->out1 = eosMore;
        eosMore->ch1 = CHAR_EOS;
        eosMore->out1 = eosMore;

        Nfa nfa = cvt.arrNfa.First();

        nfa.out->out1 = eos;

        return { nfa.in, eosMore };
    }

protected:
    virtual void Visit(re::ConcatOperator &) override
    {
        Nfa inner1 = arrNfa[arrNfa.Count() - 2];
        Nfa inner2 = arrNfa[arrNfa.Count() - 1];

        ASSERT(!inner1.out->out1);
        inner1.out->out1 = inner2.in;

        Nfa nfa = { inner1.in, inner2.out };

        arrNfa.RemoveAt(arrNfa.Count() - 2, 2);
        arrNfa.Add(nfa);
    }
    virtual void Visit(re::AlterOperator &) override
    {
        NfaNode * in = NewNode();
        NfaNode * out = NewNode();

        Nfa inner1 = arrNfa[arrNfa.Count() - 2];
        Nfa inner2 = arrNfa[arrNfa.Count() - 1];

        in->out1 = inner1.in;
        in->out2 = inner2.in;

        ASSERT(!inner1.out->out1 && !inner2.out->out1);
        inner1.out->out1 = out;
        inner2.out->out1 = out;

        Nfa nfa = { in, out };

        arrNfa.RemoveAt(arrNfa.Count() - 2, 2);
        arrNfa.Add(nfa);
    }
    virtual void Visit(re::KleeneStarOperator &) override
    {
        NfaNode * in = NewNode();
        NfaNode * out = NewNode();

        Nfa inner = arrNfa.Last();

        in->out1 = inner.in;
        in->out2 = out;

        ASSERT(!inner.out->out1 && !inner.out->out2);
        inner.out->out1 = inner.in;
        inner.out->out2 = in;

        arrNfa.Last() = { in, out };
    }
    virtual void Visit(re::ASCIICharacter & ch) override
    {
        NfaNode * in = NewNode();
        NfaNode * out = NewNode();

        in->ch1 = ch.index;
        in->out1 = out;

        Nfa nfa = { in, out };

        arrNfa.Add(nfa);
    }
    NfaNode * NewNode()
    {
        NfaNode * n = NFA_CONTEXT_ALLOC(NfaNode);
        n->ch1 = n->ch2 = CHAR_EPSILON;
        n->out1 = n->out2 = nullptr;
        return n;
    }

private:
    Array<Nfa> arrNfa;
};

// Nfa visitor

struct NfaNodeSet
{
    bool IsEmpty() const
    {
        return nodes.empty();
    }
    bool operator < (const NfaNodeSet & other) const
    {
        auto i1 = nodes.begin();
        auto i2 = other.nodes.begin();
        for (;;)
        {
            if (i1 == nodes.end() && i2 == other.nodes.end())
                return false;
            else if (i1 == nodes.end())
                return true;
            else if (i2 == other.nodes.end())
                return false;
            else if (*i1 != *i2)
                return *i1 < *i2;
            else
                ++i1, ++i2;
        }
    }
    std::set<NfaNode *> nodes;
};
NfaNodeSet Start(Nfa nfa)
{
    std::set<NfaNode *> output;

    std::deque<NfaNode *> q = { nfa.in };
    std::set<NfaNode *> dup;
    while (!q.empty())
    {
        NfaNode * pNode = q.back();
        q.pop_back();
        if (dup.find(pNode) != dup.end())
            continue;
        dup.insert(pNode);

        if (pNode->ch1 != CHAR_EPSILON || pNode->ch2 != CHAR_EPSILON)
            output.insert(pNode);
        if (pNode->ch1 == CHAR_EPSILON && pNode->out1)
            q.push_front(pNode->out1);
        if (pNode->ch2 == CHAR_EPSILON && pNode->out2)
            q.push_front(pNode->out2);
    }

    NfaNodeSet ns;
    ns.nodes = ::std::move(output);
    return ns;
}
NfaNodeSet Jump(NfaNodeSet ns, CharIndex ch)
{
    std::set<NfaNode *> output;

    std::deque<NfaNode *> q;
    for (NfaNode * pNode : ns.nodes)
    {
        if (pNode->ch1 == ch && pNode->out1)
            q.push_front(pNode->out1);
        if (pNode->ch2 == ch && pNode->out2)
            q.push_front(pNode->out2);
    }

    std::set<NfaNode *> dup;
    while (!q.empty())
    {
        NfaNode * pNode = q.back();
        q.pop_back();
        if (dup.find(pNode) != dup.end())
            continue;
        dup.insert(pNode);

        if (pNode->ch1 != CHAR_EPSILON || pNode->ch2 != CHAR_EPSILON)
            output.insert(pNode);
        if (pNode->ch1 == CHAR_EPSILON && pNode->out1)
            q.push_front(pNode->out1);
        if (pNode->ch2 == CHAR_EPSILON && pNode->out2)
            q.push_front(pNode->out2);
    }

    NfaNodeSet ns2;
    ns2.nodes = ::std::move(output);
    return ns2;
}
bool IsAccept(NfaNodeSet ns)
{
    if (ns.nodes.size() != 1)
        return false;

    NfaNode * pNode = *ns.nodes.begin();
    return pNode->ch1 == CHAR_EOS && pNode->out1 != pNode;
}

// Dfa structure

struct DfaJumpTable
{
    struct ConstRow
    {
        const int & operator[] (int col) const
        {
            ASSERT(0 <= col && col < table.nCol);
            return table.pData[row * table.nCol + col];
        }

        const DfaJumpTable & table;
        int row;
    };
    struct Row
    {
        int & operator[] (int col)
        {
            ASSERT(0 <= col && col < table.nCol);
            return table.pData[row * table.nCol + col];
        }

        DfaJumpTable & table;
        int row;
    };
    const ConstRow operator[] (int row) const
    {
        ASSERT(0 <= row && row < nRow);
        return { *this, row };
    }
    Row operator[] (int row)
    {
        ASSERT(0 <= row && row < nRow);
        return { *this, row };
    }

    int nRow;
    int nCol;
    int * pData;
};
struct DfaPropTable
{
    const bool & operator[] (int state) const
    {
        ASSERT(0 <= state && state < nRow);
        return pData[state];
    }
    bool & operator[] (int state)
    {
        ASSERT(0 <= state && state < nRow);
        return pData[state];
    }

    int nRow;
    bool * pData;
};

struct Dfa
{
    static Dfa Create(int row, int col)
    {
        Dfa dfa;
        DfaJumpTable & jump = dfa.jumpTable;
        DfaPropTable & prop = dfa.propTable;

        jump.nRow = prop.nRow = row;
        jump.nCol = col;
        jump.pData = new int[row * col];
        prop.pData = new bool[row];

        return dfa;
    }

    DfaJumpTable jumpTable;
    DfaPropTable propTable;
};

class DfaConverter
{
public:
    static Dfa Convert(Nfa nfa);
};

Dfa DfaConverter::Convert(Nfa nfa)
{
    // Subset construction.

    std::map<NfaNodeSet, int> mNfa2Dfa;
    std::map<int, NfaNodeSet> mDfa2Nfa;
    int maxDfaState = 0;

    NfaNodeSet n1 = Start(nfa);
    NfaNodeSet n2 = Jump(n1, 'a');
    NfaNodeSet n3 = Jump(n2, 'b');
    NfaNodeSet n4 = Jump(n3, CHAR_EOS);
    NfaNodeSet n5 = Jump(n4, CHAR_EOS);
    NfaNodeSet n6 = Jump(n5, CHAR_EOS);

    std::deque<NfaNodeSet> qNodeSet = { Start(nfa) };
    while (!qNodeSet.empty())
    {
        NfaNodeSet ns = qNodeSet.back();
        qNodeSet.pop_back();

        ++maxDfaState;
        mNfa2Dfa.emplace(ns, maxDfaState);
        mDfa2Nfa.emplace(maxDfaState, ns);

        for (CharIndex ch = CHAR_FIRST; ch <= CHAR_LAST; ++ch)
        {
            NfaNodeSet ns2 = Jump(ns, ch);
            if (!ns2.IsEmpty() &&
                mNfa2Dfa.find(ns2) == mNfa2Dfa.end())
            {
                qNodeSet.push_front(ns2);
            }
        }
    }

    int row = maxDfaState + 1;
    int col = CHAR_COUNT;

    Dfa dfa = Dfa::Create(row, col);

    DfaJumpTable & jumpTable = dfa.jumpTable;
    DfaPropTable & propTable = dfa.propTable;

    propTable[0] = false;
    for (int ch = 0; ch != col; ++ch)
    {
        jumpTable[0][ch] = 0;
    }
    for (int state = 1; state < row; ++state)
    {
        NfaNodeSet & ns = mDfa2Nfa[state];
        propTable[state] = IsAccept(ns);
        for (int ch = 0; ch != col; ++ch)
        {
            jumpTable[state][ch] = mNfa2Dfa[Jump(ns, ch)];
        }
    }

    // Dump Dfa
    {
        std::cout << "-----------------------------" << std::endl;
        std::cout << std::endl;
        std::cout << "  Nfa" << std::endl;
        std::cout << std::endl;
        // assign nfa state ids
        std::map<NfaNode *, int> mNfaId= { {nullptr, -1} };
        int nfaId = 0;
        for (auto p : mDfa2Nfa)
        {
            for (auto s : p.second.nodes)
            {
                if (mNfaId.emplace(s, nfaId).second)
                    ++nfaId;
                if (mNfaId.emplace(s->out1, nfaId).second)
                    ++nfaId;
                if (mNfaId.emplace(s->out2, nfaId).second)
                    ++nfaId;
            }
        }
        for (auto p : mNfaId)
        {
            if (!p.first) continue;
            std::cout << p.second << "\t";
            if (p.first->out1)
            {
                if (p.first->ch1 == CHAR_EPSILON) std::cout << "--->" << mNfaId[p.first->out1];
                else if (p.first->ch1 == CHAR_EOS) std::cout << "-$->" << mNfaId[p.first->out1];
                else std::cout << "-" << (char)p.first->ch1 << "->" << mNfaId[p.first->out1];
                std::cout << "\t";
            }
            if (p.first->out2)
            {
                if (p.first->ch2 == CHAR_EPSILON) std::cout << "--->" << mNfaId[p.first->out2];
                else if (p.first->ch2 == CHAR_EOS) std::cout << "-$->" << mNfaId[p.first->out2];
                else std::cout << "-" << (char)p.first->ch2 << "->" << mNfaId[p.first->out2];
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
        std::cout << "  Dfa -> Nfa Set" << std::endl;
        std::cout << std::endl;
        // dfa -> nfa set
        for (auto p : mDfa2Nfa)
        {
            int dfa = p.first;
            NfaNodeSet & nfa = p.second;

            std::cout << "[" << dfa << "]\t";
            for (auto s : nfa.nodes)
            {
                std::cout << mNfaId[s] << ", ";
            }
            std::cout << std::endl;
        }
        // dfa
        std::cout << std::endl;
        std::cout << "  Dfa" << std::endl;
        std::cout << std::endl;
        for (int r = 0; r < row; ++r)
        {
            std::cout << "[" << r << "]\t";
            for (int c = 0; c < col; ++c)
            {
                if (jumpTable[r][c] && (c == CHAR_EOS || (c <= 127 && isalnum(c))))
                    std::cout << (c != CHAR_EOS ? (char)c : '$') << " -> " << jumpTable[r][c] << ", ";
                // std::cout << c << "('" << (char)c << "') -> " << jumpTable[r][c] << ", ";
            }
            std::cout << (propTable[r] ? "Accept" : "") << std::endl;
        }
        std::cout << std::endl;
        std::cout << "-----------------------------" << std::endl;
    }

    return dfa;
}

// Dfa match
struct DfaResult
{
    int maxScanLen;
    int maxAcceptLen;
};
class InputReader
{
public:
    InputReader(const char * str)
        : pNext(str)
        , bEndOfInput(false)
    {
        ASSERT(pNext);
    }
    bool More()
    {
        return !bEndOfInput;
    }
    int Get()
    {
        if (*pNext)
            return *pNext++;
        else
        {
            bEndOfInput = true;
            return CHAR_EOS;
        }
    }
private:
    const char * pNext;
    bool bEndOfInput;
};
DfaResult Run(const Dfa & dfa,
              InputReader input)
{
    int state = 1;
    int maxAcptLen = 0;

    int maxScanLen = 0;
    while (state && input.More())
    {
        if (dfa.propTable[state])
            maxAcptLen = maxScanLen;
        state = dfa.jumpTable[state][input.Get()];
        ++maxScanLen;
    }

    return { maxScanLen, maxAcptLen };
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
    static String Convert(re::Tree & tree)
    {
        REToStringConverter cvt;
        tree.Accept(cvt);
        return cvt.arrStr[0];
    }

protected:
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

TEST(Regex_API_Compile)
{
    TRACE_MEMORY(RegexCompile);

    re::CompositorContext reCtx;
    re::NfaContext nfaCtx;

    // ab
    re::Compositor cp =
        re::Concat(
            re::Ascii('a'),
            re::Ascii('b'));
    re::Tree re = cp.Get();

    re::Nfa nfa = re::NfaConverter::Convert(re);
    re::Dfa dfa = re::DfaConverter::Convert(nfa);

    re::DfaResult r1 = re::Run(dfa, "ab");
    EXPECT_EQ(r1.maxScanLen, 3);
    EXPECT_EQ(r1.maxAcceptLen, 2);
}

#endif