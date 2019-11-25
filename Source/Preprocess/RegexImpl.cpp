#include "RegexImpl.h"

#include "../Memory/Allocate.h"

#include <set>
#include <deque>
#include <map>

namespace v2 {
namespace re {

using ::containers::Array;
using ::containers::Set;
using ::containers::Map;
using ::containers::Queue;
using Allocator = ::memory::GenericFreeListAllocator;

// Memory management

static ::memory::GenericFreeListAllocator gRegexAllocator;

#define REGEX_ALLOC(T) new ((T *)gRegexAllocator.Alloc(sizeof(T))) T
#define REGEX_FREE(T, ptr) ((ptr)->~T(), gRegexAllocator.Free(ptr))

// Charset

using CharIndex = int;
#define CHAR_FIRST (0)
#define CHAR_EPSILON (256 + 1)
#define CHAR_EOS (256 + 2)
#define CHAR_LAST CHAR_EOS
#define CHAR_COUNT (CHAR_LAST + 1)

// Regex

#define ACCEPT_VISIT_INTERFACE  virtual void Accept(RegexStruct::Visitor & visitor) = 0;
#define ACCEPT_VISIT_DECL       virtual void Accept(RegexStruct::Visitor & visitor) override;
#define ACCEPT_VISIT_IMPL(type) void type::Accept(RegexStruct::Visitor & visitor) { visitor.Visit(*this); }

class RegexStruct::Element
{
public:
    virtual ~Element() = default;

    ACCEPT_VISIT_INTERFACE
};
class ConcatOperator : public RegexStruct::Element
{
public:
    ACCEPT_VISIT_DECL
};
class AlterOperator : public RegexStruct::Element
{
public:
    ACCEPT_VISIT_DECL
};
class KleeneStarOperator : public RegexStruct::Element
{
public:
    ACCEPT_VISIT_DECL
};
class ASCIICharacter : public RegexStruct::Element
{
public:
    ACCEPT_VISIT_DECL
        CharIndex index;
};

class RegexStruct::Visitor
{
public:
    virtual ~Visitor() = default;

    virtual void Visit(ConcatOperator &) {}
    virtual void Visit(AlterOperator &) {}
    virtual void Visit(KleeneStarOperator &) {}
    virtual void Visit(ASCIICharacter &) {}
};

ACCEPT_VISIT_IMPL(ConcatOperator)
ACCEPT_VISIT_IMPL(AlterOperator)
ACCEPT_VISIT_IMPL(KleeneStarOperator)
ACCEPT_VISIT_IMPL(ASCIICharacter)

RegexStructFactory::PostfixBuilder RegexStructFactory::CreateBuilder()
{
    return {};
}

// group -> or -> and -> star -> group/char
// RegexStruct RegexStructFactory::CreateFromString(StringView svRegex);

RegexStruct::Node * RegexStructFactory::NewAsciiNode(int ch)
{
    ASCIICharacter * pElement = REGEX_ALLOC(ASCIICharacter);
    pElement->index = ch;

    RegexStruct::Node * pNode = REGEX_ALLOC(RegexStruct::Node);
    pNode->pLeft = nullptr;
    pNode->pRight = nullptr;
    pNode->pElement = pElement;

    return pNode;
}

RegexStruct::Node * RegexStructFactory::NewConcatNode(RegexStruct::Node * pLeft, RegexStruct::Node * pRight)
{
    RegexStruct::Node * pNode = REGEX_ALLOC(RegexStruct::Node);
    pNode->pLeft = pLeft;
    pNode->pRight = pRight;
    pNode->pElement = REGEX_ALLOC(ConcatOperator);

    return pNode;
}

RegexStruct::Node * RegexStructFactory::NewAlterNode(RegexStruct::Node * pLeft, RegexStruct::Node * pRight)
{
    RegexStruct::Node * pNode = REGEX_ALLOC(RegexStruct::Node);
    pNode->pLeft = pLeft;
    pNode->pRight = pRight;
    pNode->pElement = REGEX_ALLOC(AlterOperator);

    return pNode;
}

RegexStruct::Node * RegexStructFactory::NewKleeneStarNode(RegexStruct::Node * pInner)
{
    RegexStruct::Node * pNode = REGEX_ALLOC(RegexStruct::Node);
    pNode->pLeft = pInner;
    pNode->pRight = nullptr;
    pNode->pElement = REGEX_ALLOC(KleeneStarOperator);

    return pNode;
}

void RegexStruct::FreeRegexStruct(Node * pNode)
{
    if (pNode)
    {
        FreeRegexStruct(pNode->pLeft);
        FreeRegexStruct(pNode->pRight);
        REGEX_FREE(Element, pNode->pElement);
    }
}

void RegexStruct::VisitRegexStruct(const Node * pNode,
                                   Visitor & visitor)
{
    if (pNode)
    {
        VisitRegexStruct(pNode->pLeft, visitor);
        VisitRegexStruct(pNode->pRight, visitor);
        pNode->pElement->Accept(visitor);
    }
}

RegexStruct::RegexStruct(RegexStruct && other)
    : pRoot(other.pRoot)
{
    other.pRoot = nullptr;
}

RegexStruct::~RegexStruct()
{
    if (pRoot)
    {
        FreeRegexStruct(pRoot);
    }
}

void RegexStruct::Accept(Visitor & visitor)
{
    VisitRegexStruct(pRoot, visitor);
}

RegexStructFactory::PostfixBuilder & RegexStructFactory::PostfixBuilder::Ascii(int ch)
{
    stack.Add(NewAsciiNode(ch));
    return *this;
}

RegexStructFactory::PostfixBuilder & RegexStructFactory::PostfixBuilder::Concat()
{
    RegexStruct::Node * pNode1 = stack[stack.Count() - 2];
    RegexStruct::Node * pNode2 = stack[stack.Count() - 1];
    RegexStruct::Node * pNodeConcat = NewConcatNode(pNode1, pNode2);
    stack.RemoveAt(stack.Count() - 2, 2);
    stack.Add(pNodeConcat);
    return *this;
}

RegexStructFactory::PostfixBuilder & RegexStructFactory::PostfixBuilder::Alter()
{
    RegexStruct::Node * pNode1 = stack[stack.Count() - 2];
    RegexStruct::Node * pNode2 = stack[stack.Count() - 1];
    RegexStruct::Node * pNodeAlter = NewAlterNode(pNode1, pNode2);
    stack.RemoveAt(stack.Count() - 2, 2);
    stack.Add(pNodeAlter);
    return *this;
}

RegexStructFactory::PostfixBuilder & RegexStructFactory::PostfixBuilder::KleeneStar()
{
    RegexStruct::Node * pNode = stack[stack.Count() - 1];
    RegexStruct::Node * pNodeKleeneStar = NewKleeneStarNode(pNode);
    stack.RemoveAt(stack.Count() - 1, 1);
    stack.Add(pNodeKleeneStar);
    return *this;
}

RegexStruct RegexStructFactory::PostfixBuilder::Build()
{
    ASSERT(stack.Count() == 1);
    return RegexStruct(stack[0]);
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

#define NFA_GC_NEW(argType) (argType *)NfaContext::Get().GetAllocator().Alloc(sizeof(argType))
#define NFA_CONTEXT_FREE(argAddr) (NfaContext::Get().GetAllocator().Free(argAddr))

// Nfa structure builder

class NfaConverter : public RegexStruct::Visitor
{
public:
    static Nfa Convert(RegexStruct & rs)
    {
        NfaConverter cvt;
        rs.Accept(cvt);
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
    virtual void Visit(ConcatOperator &) override
    {
        Nfa inner1 = arrNfa[arrNfa.Count() - 2];
        Nfa inner2 = arrNfa[arrNfa.Count() - 1];

        ASSERT(!inner1.out->out1);
        inner1.out->out1 = inner2.in;

        Nfa nfa = { inner1.in, inner2.out };

        arrNfa.RemoveAt(arrNfa.Count() - 2, 2);
        arrNfa.Add(nfa);
    }
    virtual void Visit(AlterOperator &) override
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
    virtual void Visit(KleeneStarOperator &) override
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
    virtual void Visit(ASCIICharacter & ch) override
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
        NfaNode * n = NFA_GC_NEW(NfaNode);
        n->ch1 = n->ch2 = CHAR_EPSILON;
        n->out1 = n->out2 = nullptr;
        return n;
    }

private:
    Array<Nfa> arrNfa;
};

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

} } namespace containers {
// For Set<NfaNodeSet>, Map<NfaNodeSet, *>
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

    DfaJumpTable()
        : nRow(0)
        , nCol(0)
        , pData(nullptr)
    {
    }
    DfaJumpTable(int row, int col)
        : nRow(row)
        , nCol(col)
        , pData(nullptr)
    {
        ASSERT(nRow > 0 && nCol > 0);
        pData = new int[nRow * nCol];
    }
    ~DfaJumpTable()
    {
        if (pData)
        {
            delete[] pData;
        }
    }

    void Swap(DfaJumpTable & other)
    {
        int iTmp, * pTmp;
        iTmp = nRow; nRow = other.nRow; other.nRow = iTmp;
        iTmp = nCol; nCol = other.nCol; other.nCol = iTmp;
        pTmp = pData; pData = other.pData; other.pData = pTmp;
    }

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
    DfaPropTable()
        : nRow(0)
        , pData(nullptr)
    {
    }
    DfaPropTable(int row)
        : nRow(row)
        , pData(nullptr)
    {
        ASSERT(nRow > 0);
        pData = new bool[nRow];
    }
    ~DfaPropTable()
    {
        if (pData)
            delete[] pData;
    }

    void Swap(DfaPropTable & other)
    {
        int iTmp;
        bool * pTmp;
        iTmp = nRow; nRow = other.nRow; other.nRow = iTmp;
        pTmp = pData; pData = other.pData; other.pData = pTmp;
    }

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

class Dfa
{
public:
    Dfa()
    {
    }
    Dfa(const Dfa &) = delete;
    Dfa & operator = (const Dfa &) = delete;
    Dfa(Dfa && other)
    {
        jumpTable.Swap(other.jumpTable);
        propTable.Swap(other.propTable);
    }
    Dfa & operator = (Dfa && other)
    {
        this->~Dfa();
        new (this) Dfa(static_cast<Dfa &&>(other));
        return *this;
    }

    Dfa(int row, int col)
        : jumpTable(row, col)
        , propTable(row)
    {
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

    DfaJumpTable & GetJumpTable()
    {
        return jumpTable;
    }
    DfaPropTable & GetPropTable()
    {
        return propTable;
    }

private:
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

    Dfa dfa(row, col);

    DfaJumpTable & jumpTable = dfa.GetJumpTable();
    DfaPropTable & propTable = dfa.GetPropTable();

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
    if (maxAcptLen >= 0)
        content.ShrinkTo(maxAcptLen);
    else
        content.Clear();

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

bool MatchResult::IsValid() const
{
    return maxAcceptLen >= 0;
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

Regex::Regex(RegexStruct rs)
{
    NfaContext nfaCtx;

    Nfa nfa = NfaConverter::Convert(rs);
    Dfa dfa = DfaConverter::Convert(nfa);

    Regex::Impl * impl = new Regex::Impl();
    impl->dfa = std::move(dfa);

    pImpl.reset(impl);
}

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

//Regex Compositor::GetCompiled()
//{
//    RegTree regexTree = Get();
//
//    NfaContext nfaCtx;
//
//    Nfa nfa = NfaConverter::Convert(regexTree);
//    Dfa dfa = DfaConverter::Convert(nfa);
//
//    Regex::Impl * impl = new Regex::Impl();
//    impl->dfa = std::move(dfa);
//
//    Regex regex;
//    PImplAccessor::Get(regex).reset(impl);
//    return regex;
//}

bool MatchResultIterator::More()
{
    return pImpl->inputReader.More();
}

MatchResult MatchResultIterator::Next()
{
    return DfaMatch(pImpl->dfa, pImpl->inputReader);
}

int MatchResultIterator::IgnoreCharacter()
{
    if (pImpl->inputReader.More())
        return pImpl->inputReader.Get();
    else
        return CHAR_EOS;
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

using namespace v2::re;

class REToStringConverter : public RegexStruct::Visitor
{
public:
    static String Convert(RegexStruct & rs)
    {
        REToStringConverter cvt;
        rs.Accept(cvt);
        return cvt.arrStr[0];
    }

protected:
    virtual void Visit(ConcatOperator &) override
    {
        ASSERT(arrStr.Count() >= 2);
        String s;
        String s1 = arrStr[arrStr.Count() - 2];
        String s2 = arrStr[arrStr.Count() - 1];
        s.Add('(').Append(s1).Append(s2).Add(')');
        arrStr.RemoveAt(arrStr.Count() - 2, 2);
        arrStr.Add(s);
    }
    virtual void Visit(AlterOperator &) override
    {
        ASSERT(arrStr.Count() >= 2);
        String s;
        String s1 = arrStr[arrStr.Count() - 2];
        String s2 = arrStr[arrStr.Count() - 1];
        s.Add('(').Append(s1).Add('|').Append(s2).Add(')');
        arrStr.RemoveAt(arrStr.Count() - 2, 2);
        arrStr.Add(s);
    }
    virtual void Visit(KleeneStarOperator &) override
    {
        ASSERT(arrStr.Count() >= 1);
        String s;
        s.Add('(').Append(arrStr[arrStr.Count() - 1]).Add(')').Add('*');
        arrStr.RemoveAt(arrStr.Count() - 1, 1);
        arrStr.Add(s);
    }
    virtual void Visit(ASCIICharacter & ch) override
    {
        arrStr.Add(String((char)ch.index, 1));
    }

private:
    containers::Array<String> arrStr;
};

TEST(Regex2_Builder)
{
    // abc*|d
    RegexStruct rs =
        RegexStructFactory::CreateBuilder()
        .Ascii('a')
        .Ascii('b')
        .Concat()
        .Ascii('c')
        .KleeneStar()
        .Concat()
        .Ascii('d')
        .Alter()
        .Build();

    EXPECT_EQ(
        REToStringConverter::Convert(rs),
        String("(((ab)(c)*)|d)"));
}

TEST(Regex2_Nfa)
{
    // re::Start, re::Jump

    NfaContext nfaCtx;

    RegexStruct rs =
        RegexStructFactory::CreateBuilder()
        .Ascii('a')
        .Ascii('b')
        .Concat()
        .Build();
    Nfa nfa = NfaConverter::Convert(rs);

    NfaNode * a = nfa.in;
    NfaNode * b = a->out1->out1;
    NfaNode * end = b->out1->out1;
    NfaNode * term = end->out1;

    int input[] = { 'a', 'b', CHAR_EOS, CHAR_EOS };
    NfaNode * expectNode[] = { a, b, end, term, };
    NfaNodeSet ns = Start(nfa);
    for (int i = 0, idx = 0; i < 10; ++i, idx += (idx < 3 ? 1 : 0))
    {
        EXPECT_EQ(ns.nodes.Count(), 1);
        EXPECT_EQ(ns.nodes.GetFirst(), expectNode[idx]);
        ns = Jump(ns, input[idx]);
    }
}

TEST(Regex2_Dfa)
{
    NfaContext nfaCtx;

    // ab
    RegexStruct rs =
        RegexStructFactory::CreateBuilder()
        .Ascii('a')
        .Ascii('b')
        .Concat()
        .Build();

    Nfa nfa = NfaConverter::Convert(rs);
    Dfa dfa = DfaConverter::Convert(nfa);

    InputReader ir("ab");
    MatchResult mr = DfaMatch(dfa, ir);
    EXPECT_EQ(mr.MaxScanLen(), 2);
    EXPECT_EQ(mr.MaxAcceptLen(), 2);
}

TEST(Regex2_API)
{
    // Build Regex
    Regex regex = RegexStructFactory::CreateBuilder()
        .Ascii('a').KleeneStar()
        .Ascii('b')
        .Concat()
        .Build();

    // Match
    MatchResultIterator mri = IterateMatches(regex, "abccaabffaaab");
    while (mri.More())
    {
        MatchResult mr = mri.Next();
        if (mr.IsValid())
        {
            std::cout << "match: " << mr.Content() << std::endl;
        }
        else
        {
            int ch = mri.IgnoreCharacter();
            std::cout << "ignore: " << (char)(ch == CHAR_EOS ? '$' : ch) << std::endl;
        }
    }
}

#endif