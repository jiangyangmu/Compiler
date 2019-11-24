#include "RegexImpl.h"

#include <set>
#include <deque>
#include <map>

namespace v2 {
namespace re {

using ::containers::Array;
using ::containers::Set;
using ::containers::Map;
using ::containers::Queue;

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

RegexTree::RegexTree(TreeNode * root,
           Allocator & allocator)
    : pRoot(root)
    , allocator(allocator)
{
}

RegexTree::~RegexTree()
{
    FreeTree(pRoot, allocator);
}

void RegexTree::Accept(Visitor & visitor)
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
RegexTree Compositor::Get()
{
    return RegexTree(pTree, CompositorContext::Get().GetAllocator());
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
    static Nfa Convert(re::RegexTree & tree)
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
        return nodes.Empty();
    }
    bool operator == (const NfaNodeSet & other) const
    {
        if (nodes.Count() != other.nodes.Count())
            return false;
        else
        {
            NfaNode * pNode;
            for (auto pos = nodes.GetStartPos();
                 nodes.GetNextElem(pos, pNode);
                 )
            {
                if (!other.nodes.Contains(pNode))
                    return false;
            }
            return true;
        }
    }
    Set<NfaNode *> nodes;
};

} } namespace containers {

template <>
UINT HashKey(v2::re::NfaNodeSet ns)
{
    UINT64 h = 0;
    v2::re::NfaNode * n;
    for (auto pos = ns.nodes.GetStartPos();
         ns.nodes.GetNextElem(pos, n);
         )
    {
        UINT64 p = reinterpret_cast<UINT64>(n);
        h ^= p ^ (p >> 16);
    }
    return static_cast<UINT>(h);
}

} namespace v2 { namespace re {

NfaNodeSet Start(Nfa nfa)
{
    Set<NfaNode *> output;

    Queue<NfaNode *> q;
    Set<NfaNode *> dup;

    q.Enqueue(nfa.in);
    while (!q.Empty())
    {
        NfaNode * pNode = q.Dequeue();
        if (dup.Contains(pNode))
            continue;
        dup.Insert(pNode);

        if (pNode->ch1 != CHAR_EPSILON || pNode->ch2 != CHAR_EPSILON)
            output.Insert(pNode);
        if (pNode->ch1 == CHAR_EPSILON && pNode->out1)
            q.Enqueue(pNode->out1);
        if (pNode->ch2 == CHAR_EPSILON && pNode->out2)
            q.Enqueue(pNode->out2);
    }

    NfaNodeSet ns;

    NfaNode * pNode = nullptr;
    auto pos = output.GetStartPos();
    while (output.GetNextElem(pos, pNode))
    {
        ASSERT(pNode);
        ns.nodes.Insert(pNode);
    }

    return ns;
}
NfaNodeSet Jump(NfaNodeSet ns, CharIndex ch)
{
    Set<NfaNode *> output;

    Queue<NfaNode *> q;
    NfaNode * pNode = nullptr;
    for (auto pos = ns.nodes.GetStartPos();
         ns.nodes.GetNextElem(pos, pNode);
         )
    {
        if (pNode->ch1 == ch && pNode->out1)
            q.Enqueue(pNode->out1);
        if (pNode->ch2 == ch && pNode->out2)
            q.Enqueue(pNode->out2);
    }

    Set<NfaNode *> dup;
    while (!q.Empty())
    {
        NfaNode * pNode = q.Dequeue();
        if (dup.Contains(pNode))
            continue;
        dup.Insert(pNode);

        if (pNode->ch1 != CHAR_EPSILON || pNode->ch2 != CHAR_EPSILON)
            output.Insert(pNode);
        if (pNode->ch1 == CHAR_EPSILON && pNode->out1)
            q.Enqueue(pNode->out1);
        if (pNode->ch2 == CHAR_EPSILON && pNode->out2)
            q.Enqueue(pNode->out2);
    }

    NfaNodeSet ns2;

    NfaNode * pNode2 = nullptr;
    for (auto pos = output.GetStartPos();
         output.GetNextElem(pos, pNode2);
         )
    {
        ASSERT(pNode2);
        ns2.nodes.Insert(pNode2);
    }

    return ns2;
}
bool IsAccept(NfaNodeSet ns)
{
    if (ns.nodes.Count() != 1)
        return false;

    NfaNode * pNode = ns.nodes.GetFirst();
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

    int Start() const
    {
        return 1;
    }
    int Jump(int state, int ch) const
    {
        return jumpTable[state][ch];
    }
    bool Accept(int state) const
    {
        return propTable[state];
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

    Map<NfaNodeSet, int> mNfa2Dfa;
    Map<int, NfaNodeSet> mDfa2Nfa;
    int maxDfaState = 0;

    Queue<NfaNodeSet> qNodeSet;
    qNodeSet.Enqueue(Start(nfa));
    while (!qNodeSet.Empty())
    {
        NfaNodeSet ns = qNodeSet.Dequeue();

        ++maxDfaState;
        mNfa2Dfa.Insert(ns, maxDfaState);
        mDfa2Nfa.Insert(maxDfaState, ns);

        for (CharIndex ch = CHAR_FIRST; ch <= CHAR_LAST; ++ch)
        {
            NfaNodeSet ns2 = Jump(ns, ch);
            if (!ns2.IsEmpty() && !mNfa2Dfa.Contains(ns2))
            {
                qNodeSet.Enqueue(ns2);
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
        NfaNodeSet & ns = mDfa2Nfa.At(state);
        propTable[state] = IsAccept(ns);
        for (int ch = 0; ch != col; ++ch)
        {
            NfaNodeSet ns2 = Jump(ns, ch);
            int nextState = 0;

            jumpTable[state][ch] =
                mNfa2Dfa.Lookup(ns2, nextState)
                ? nextState
                : 0;
        }
    }

    // Dump Dfa
    {
        std::cout << "-----------------------------" << std::endl;
        std::cout << std::endl;
        std::cout << "  Nfa" << std::endl;
        std::cout << std::endl;
        // assign nfa state ids
        std::map<NfaNode *, int> mNfaId = { {nullptr, -1} };
        int nfaId = 0;

        int dfaId;
        NfaNodeSet ns;
        for (auto pos = mDfa2Nfa.GetStartPos();
             mDfa2Nfa.GetNextAssoc(pos, dfaId, ns);
             )
        {
            NfaNode * pNode;
            for (auto pos = ns.nodes.GetStartPos();
                 ns.nodes.GetNextElem(pos, pNode);
                 )
            {
                if (mNfaId.emplace(pNode, nfaId).second)
                    ++nfaId;
                if (mNfaId.emplace(pNode->out1, nfaId).second)
                    ++nfaId;
                if (mNfaId.emplace(pNode->out2, nfaId).second)
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
        for (auto pos = mDfa2Nfa.GetStartPos();
             mDfa2Nfa.GetNextAssoc(pos, dfaId, ns);
             )
        {
            std::cout << "[" << dfaId << "]\t";
            NfaNode * s;
            for (auto pos = ns.nodes.GetStartPos();
                 ns.nodes.GetNextElem(pos, s);
                 )
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

class InputReader
{
public:
    InputReader(const char * str, const char * end = nullptr)
        : pNext(str)
        , pEnd(end)
    {
        ASSERT(pNext);
    }
    bool More()
    {
        return (!pEnd && *pNext) || (pEnd && pNext < pEnd);
    }
    int Peek()
    {
        if (More())
            return *pNext;
        else
        {
            return CHAR_EOS;
        }
    }
    int Get()
    {
        if (More())
            return *pNext++;
        else
        {
            return CHAR_EOS;
        }
    }
private:
    const char * pNext;
    const char * pEnd;
};

MatchResult DfaMatch(const Dfa & dfa,
                     InputReader & input)
{
    int maxScanLen = 0;
    int maxAcptLen = -1;
    String content;

    int state = dfa.Start();
    for (;;)
    {
        if (dfa.Accept(state))
            maxAcptLen = maxScanLen;

        if (!input.More())
            break;

        state = dfa.Jump(state, input.Peek());
        if (state == 0)
            break;
        
        content.Add(input.Get());
        ++maxScanLen;
    }
    content.ShrinkTo(maxAcptLen);

    return MatchResult(content, maxScanLen, maxAcptLen);
}

// Client interface

MatchResult::MatchResult(String content,
                         int maxScanLen,
                         int maxAcceptLen)
    : content(std::move(content))
    , maxScanLen(maxScanLen)
    , maxAcceptLen(maxAcceptLen)
{
}

StringView MatchResult::Content() const
{
    return StringView(content.RawData(), content.Length());
}

int MatchResult::MaxScanLen() const
{
    return maxScanLen;
}

int MatchResult::MaxAcceptLen() const
{
    return maxAcceptLen;
}

class Regex::Impl
{
public:
    Dfa dfa;
};

class MatchResultIterator::Impl
{
public:
    Impl(const Dfa & dfa, InputReader inputReader)
        : dfa(dfa)
        , inputReader(inputReader)
    {}

    const Dfa & dfa;
    InputReader inputReader;
};

class PImplAccessor
{
public:
    template <typename T>
    static std::unique_ptr<typename T::Impl> & Get(T & obj)
    {
        return obj.pImpl;
    }
    template <typename T>
    static std::unique_ptr<typename T::Impl> const & Get(const T & obj)
    {
        return obj.pImpl;
    }
};

Regex Compile()
{
    CompositorContext reCtx;
    NfaContext nfaCtx;

    // ab
    Compositor cp =
        Concat(
            KleeneStar(Ascii('a')),
            Ascii('b'));
    RegexTree re = cp.Get();

    Nfa nfa = NfaConverter::Convert(re);
    Dfa dfa = DfaConverter::Convert(nfa);

    Regex::Impl * impl = new Regex::Impl();
    impl->dfa = dfa;

    Regex regex;
    PImplAccessor::Get(regex).reset(impl);
    return regex;
}

bool MatchResultIterator::More()
{
    return pImpl->inputReader.More();
}

MatchResult MatchResultIterator::Next()
{
    return DfaMatch(pImpl->dfa, pImpl->inputReader);
}

MatchResultIterator IterateMatches(const Regex & regex,
                                   StringView input)
{
    MatchResultIterator::Impl * impl =
        new MatchResultIterator::Impl(
            PImplAccessor::Get(regex)->dfa,
            InputReader(input.Begin(), input.End())
        );

    MatchResultIterator mri;
    PImplAccessor::Get(mri).reset(impl);
    return mri;
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
    static String Convert(re::RegexTree & tree)
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

    re::RegexTree re = cp.Get();

    EXPECT_EQ(
        REToStringConverter::Convert(re),
        String("(((ab)(c)*)|d)"));
}

TEST(Regex_API_Nfa)
{
    // re::Start, re::Jump

    re::CompositorContext reCtx;
    re::NfaContext nfaCtx;

    re::Compositor cp = re::Concat(re::Ascii('a'), re::Ascii('b'));
    re::RegexTree re = cp.Get();
    re::Nfa nfa = re::NfaConverter::Convert(re);

    re::NfaNode * a = nfa.in;
    re::NfaNode * b = a->out1->out1;
    re::NfaNode * end = b->out1->out1;
    re::NfaNode * term = end->out1;

    int input[] = { 'a', 'b', CHAR_EOS, CHAR_EOS };
    re::NfaNode * expectNode[] = { a, b, end, term, };
    re::NfaNodeSet ns = re::Start(nfa);
    for (int i = 0, idx = 0; i < 10; ++i, idx += (idx < 3 ? 1 : 0))
    {
        EXPECT_EQ(ns.nodes.Count(), 1);
        EXPECT_EQ(ns.nodes.GetFirst(), expectNode[idx]);
        ns = re::Jump(ns, input[idx]);
    }
}

TEST(Regex_API_Run)
{
    re::CompositorContext reCtx;
    re::NfaContext nfaCtx;

    // ab
    re::Compositor cp =
        re::Concat(
            re::Ascii('a'),
            re::Ascii('b'));
    re::RegexTree re = cp.Get();

    re::Nfa nfa = re::NfaConverter::Convert(re);
    re::Dfa dfa = re::DfaConverter::Convert(nfa);

    re::InputReader ir("ab");
    re::MatchResult mr = re::DfaMatch(dfa, ir);
    EXPECT_EQ(mr.MaxScanLen(), 3);
    EXPECT_EQ(mr.MaxAcceptLen(), 2);
}

TEST(Regex_API_Usage)
{
    // Build Regex
    re::Regex regex = re::Compile();

    // Match
    re::MatchResultIterator mri = re::IterateMatches(regex, "abaabaaab");
    while (mri.More())
    {
        re::MatchResult mr = mri.Next();
        std::cout
            << "scan: " << mr.MaxScanLen() << std::endl
            << "acpt: " << mr.MaxAcceptLen() << std::endl
            << "text: " << mr.Content() << std::endl;
    }
}

#endif