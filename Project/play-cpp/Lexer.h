#pragma once

#include <cassert>
#include <string>
#include <vector>
#include <bitset>
#include <map>
#include <array>
#include <deque>
#include <memory>

/*
    Problem:

        pattern 1   => Context, Result Object A
        pattern 2   => Context, Result Object B
        ...
        ELSE        => Context, Result Function

        * if matches pattern 1 & 2, prefer 1
        * one char look-after assertion

    Example:

        proc        => proc keyword
        [_a-zA-Z]+  => id
        [0-9]+      => num
        ELSE        => throw error

        if          => if keyword
        ifthis      => id
        if0else     => id

*/

class CharSet
{
public:
    typedef std::array<size_t, 27> ContainerType;
    static const std::vector<char> & Chars()
    {
        static const std::vector<char> chars = { 'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','\0', };
        return chars;
    }
    static const std::vector<const char *> & CharStrs()
    {
        static const std::vector<const char *> char_strs = { "a","b","c","d","e","f","g","h","i","j","k","l","m","n","o","p","q","r","s","t","u","v","w","x","y","z","\\0", };
        return char_strs;
    }
    static const std::vector<size_t> & CharIdxs()
    {
        static const std::vector<size_t> char_idxs = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, };
        return char_idxs;
    }
    static bool Contains(char ch)
    {
        return ch == '\0' || ('a' <= ch && ch <= 'z');
    }
    static size_t CharIdx(char ch)
    {
        assert(Contains(ch));
        return ch == '\0' ? 26 : size_t(ch - 'a');
    }
    static const char * CharStr(char ch)
    {
        assert(Contains(ch));
        return CharStrs()[CharIdx(ch)];
    }
};

struct NfaState
{
    size_t id;
    bool done;

    char * c;
    NfaState * next;
    NfaState * alt;
};

typedef std::bitset<512> NfaStateSet;

struct NfaGraph
{
    NfaState * in;
    NfaState ** next;
    NfaState ** alt;
};

class NfaStateFactory
{
public:
    NfaState * NewState()
    {
        v.emplace_back(new NfaState);

        NfaState * n = v.back().get();
        n->id = v.size() - 1;
        n->done = false;
        n->c = nullptr;
        n->next = nullptr;
        n->alt = nullptr;
        
        return n;
    }
    NfaState * operator [] (size_t i) const
    {
        return v.at(i).get();
    }
    std::vector<std::unique_ptr<NfaState>> & States() { return v; }
private:
    std::vector<std::unique_ptr<NfaState>> v;
};

struct MatchStatus
{
    bool bad;
    bool cont;
    size_t goodBranch;
};
typedef size_t TableState;
struct TransitionTable
{
    // (Table-State, Char) to (Table-State)
    std::vector<CharSet::ContainerType> nextStateTable;
    // (Table-State) to (Match-Status)
    std::vector<MatchStatus> matchStatusTable;
    // (Table-State) to (Match-Branch)
    std::vector<size_t> matchedBranchTable;
    
    // debug
    std::vector<NfaStateSet> nfaStateSetTable;

    size_t Run(std::string & text);
};

struct NfaStateSetComparator
{
    bool operator () (const NfaStateSet & c1, const NfaStateSet & c2) const
    {
        size_t i = 0;
        while (i < c1.size() && c1.test(i) == c2.test(i))
            ++i;
        return i < c1.size() && !c1.test(i) && c2.test(i);
    }
};
struct CompileContext
{
    NfaStateFactory nfaStateFactory;

    std::vector<NfaGraph> nfaGraphs;
    
    std::map<NfaState *, NfaStateSet> n2c_cache;
    std::map<NfaStateSet, NfaStateSet, NfaStateSetComparator> c2c_cache;

    NfaStateSet EpsilonClosure(NfaState * n0);
    NfaStateSet EpsilonClosure(NfaStateSet c);
    NfaStateSet Next(NfaStateSet c, char ch);
    MatchStatus GetState(NfaStateSet c);

    void PrintNodes();

    TransitionTable Compile();
};

// ===================================================================
// Print
// ===================================================================

std::string NfaStateSetToString(NfaStateSet c)
{
    std::string s;
    s += "{";
    for (size_t i = 0; i < c.size(); ++i)
    {
        if (c.test(i))
            s += " " + std::to_string(i);
    }
    s += " }";
    return s;
}
void PrintNfaStateSet(NfaStateSet c)
{
    std::cout << "{";
    for (size_t i = 0; i < c.size(); ++i)
    {
        if (c.test(i))
            std::cout << " " << i;
    }
    std::cout << " }" << std::endl;;
}

void CompileContext::PrintNodes()
{
    for (auto & n : nfaStateFactory.States())
    {
        std::cout
            << n->id << (n->done ? "(done)" : (n->c ? "(non-empty)" : "")) << ":" << std::endl
            << "  --" << (n->c ? *n->c : '-') << "--> " << (n->next ? std::to_string(n->next->id) : "null") << std::endl
            << "  -----> " << (n->alt ? std::to_string(n->alt->id) : "null") << std::endl;
    }
    for (auto & n : nfaStateFactory.States())
    {
        std::cout
            << n->id << (n->done ? "(done)" : "") << ": ";
        PrintNfaStateSet(EpsilonClosure(n.get()));
    }
}

void PrintTable(TransitionTable & tt)
{
    const int w = 3;

    std::cout << "  ";
    std::cout.width(w);
    std::cout << " ";
    for (auto ch : CharSet::CharStrs())
    {
        std::cout.width(w);
        std::cout << ch;
    }
    std::cout << std::endl;

    size_t state = 0;
    for (auto row : tt.nextStateTable)
    {
        std::cout << " [";
        std::cout.width(w - 1);
        std::cout << state++ << "]";
        for (auto i : CharSet::CharIdxs())
        {
            std::cout.width(w);
            std::cout << row[i];
        }
        std::cout << std::endl;
    }

    state = 0;
    for (auto row : tt.matchStatusTable)
    {
        std::cout << " [" << state << "] ";
        if (row.bad)
            std::cout << "bad";
        else
        {
            if (row.cont && row.goodBranch > 0)
                std::cout << "ctgd #" << row.goodBranch;
            else if (row.cont)
                std::cout << "cont.";
            else if (row.goodBranch > 0)
                std::cout << "good #" << row.goodBranch;
        }
        std::cout << "\t" << NfaStateSetToString(tt.nfaStateSetTable[state]) << std::endl;

        ++state;
    }
}

// ===================================================================
// Build epsilon NFA
// ===================================================================

NfaStateFactory * gFactory = nullptr;

struct NfaStateFactoryScope
{
    NfaStateFactory * origin;
public:
    NfaStateFactoryScope(NfaStateFactory * factory)
    {
        origin = gFactory;
        gFactory = factory;
    }
    ~NfaStateFactoryScope()
    {
        gFactory = origin;
    }
};

NfaGraph MatchEmpty()
{
    NfaState * n = gFactory->NewState();
    return { n, &n->next, &n->alt };
}

NfaGraph MatchChar(char c)
{
    NfaState * n = gFactory->NewState();
    n->c = new char(c);

    return { n, &n->next, &n->alt };
}

NfaGraph Concat(NfaGraph c1, NfaGraph c2)
{
    *c1.next = c2.in;

    return { c1.in, c2.next , c1.alt };
}

NfaGraph Alt(NfaGraph c1, NfaGraph c2)
{
    NfaState * n = gFactory->NewState();
    *c1.next = n;
    *c1.alt = c2.in;
    *c2.next = n;

    return { c1.in, &n->next, c2.alt };
}

NfaGraph Rep0(NfaGraph c)
{
    NfaState * n = gFactory->NewState();
    n->alt = c.in;
    *c.next = n;

    return { n, &n->next, c.alt };
}

NfaGraph Rep1(NfaGraph c)
{
    NfaState * n = gFactory->NewState();
    n->alt = c.in;
    *c.next = n;

    return { c.in, &n->next, c.alt };
}

NfaGraph Opt(NfaGraph c)
{
    NfaState * n = gFactory->NewState();
    *c.next = n;
    *c.alt = n;

    return { c.in, &n->next, &n->alt };
}

NfaGraph Done(NfaGraph c)
{
    NfaState * n = gFactory->NewState();
    n->done = true;
    *c.next = n;

    return { c.in, &n->next, c.alt };
}

// abc
// a|b|c
// ab*c
// ab+c
// ab?c
// a(b|c)d
// n(?=o)
// alt-group - concat-list - (char|alt-group)[repeat]
#define REGEX_EXPECT(p, expect) \
    do { \
        if (*(p) != (expect)) \
            throw std::invalid_argument("bad regex"); \
    } while (false)
#define REGEX_EXPECT_TRUE(cond) \
    do { \
        if (!(cond)) \
            throw std::invalid_argument("bad regex"); \
    } while (false)

NfaGraph FromRegexChar(const char * & c);
NfaGraph FromRegexConcatList(const char * & c);
NfaGraph FromRegexAltGroup(const char * & c);

NfaGraph FromRegexChar(const char * & c)
{
    NfaGraph g;

    REGEX_EXPECT_TRUE(CharSet::Contains(*c));
    g = MatchChar(*c);
    ++c;

    return g;
}
NfaGraph FromRegexConcatList(const char * & c)
{
    // ((char | alt-group) repeat?)+
    
    NfaGraph g;
    
    assert(*c != '|' && *c != ')');
    if (*c == '(')
        g = FromRegexAltGroup(c);
    else
        g = FromRegexChar(c);
    switch (*c)
    {
        case '*': g = Rep0(g); ++c; break;
        case '+': g = Rep1(g); ++c; break;
        case '?': g = Opt(g); ++c; break;
        default: break;
    }

    while (*c != '|' && *c != ')')
    {
        NfaGraph g2;

        if (*c == '(')
            g2 = FromRegexAltGroup(c);
        else
            g2 = FromRegexChar(c);
        switch (*c)
        {
            case '*': g2 = Rep0(g2); ++c; break;
            case '+': g2 = Rep1(g2); ++c; break;
            case '?': g2 = Opt(g2); ++c; break;
            default: break;
        }

        g = Concat(g, g2);
    }

    return g;
}
NfaGraph FromRegexAltGroup(const char * & c)
{
    // '(' concat-list? ('|' concat-list?)* ')'
    
    NfaGraph g;

    REGEX_EXPECT(c, '(');
    ++c;

    if (*c == '|' || *c == ')')
        g = MatchEmpty();
    else
        g = FromRegexConcatList(c);

    while (*c == '|')
    {
        ++c;
        if (*c != ')')
            g = Alt(g, FromRegexConcatList(c));
    }

    REGEX_EXPECT(c, ')');
    ++c;

    return g;
}
NfaGraph FromRegex(std::string regex)
{
    regex = "(" + regex + ")";

    const char * c = regex.data();
    return Done(FromRegexAltGroup(c));
}
std::string ToRegex(NfaGraph g);

// ===================================================================
// Compile/run epsilon NFA
// ===================================================================

// All reachable Node and self that are non-empty or done
NfaStateSet CompileContext::EpsilonClosure(NfaState * n0)
{
    auto kv = n2c_cache.find(n0);
    if (kv != n2c_cache.end())
        return kv->second;

    NfaStateSet c;
    NfaStateSet mask;
    std::deque<NfaState *> q = {n0};
    while (!q.empty())
    {
        NfaState * n = q.back();
        q.pop_back();

        if (mask.test(n->id))
            continue;
        mask.set(n->id);

        if (n->c || n->done) // non-empty or done
        {
            c.set(n->id);
        }
        if (!n->c && n->next) // epsilon 1
        {
            assert(n->next != n);
            q.push_front(n->next);
        }
        if (n->alt) // epsilon 2
        {
            assert(n->alt != n);
            q.push_front(n->alt);
        }
    }

    return n2c_cache.emplace_hint(kv, n0, c)->second;
}

NfaStateSet CompileContext::EpsilonClosure(NfaStateSet c)
{
    NfaStateSet c2;

    auto kv = c2c_cache.find(c);
    if (kv != c2c_cache.end())
    {
        c2 = kv->second;
    }
    else
    {
        for (size_t i = 0; i < c.size(); ++i)
        {
            if (c[i])
            {
                c2 |= EpsilonClosure(nfaStateFactory[i]);
            }
        }
        c2c_cache.emplace_hint(kv, c, c2);

        //std::cout << NodesString(c) << " -> " << NodesString(c2) << std::endl;
    }

    return c2;
}

NfaStateSet CompileContext::Next(NfaStateSet c, char ch)
{
    NfaStateSet c2;
    for (size_t i = 0; i < c.size(); ++i)
    {
        if (c[i])
        {
            NfaState * n = nfaStateFactory[i];
            assert(n->done || n->c);
            if (n->done)
                /*c2.set(n->id)*/;
            else if (*n->c == ch && n->next)
                c2.set(n->next->id);
        }
    }
    return c2;
}

MatchStatus CompileContext::GetState(NfaStateSet c)
{
    if (c.none())
        return {true, false, 0};

    bool cont = false;
    size_t goodBranch = 0;

    bool done = false;
    size_t doneNfaStateId = 0;
    for (size_t i = 0; i < c.size() && (!cont || !goodBranch); ++i)
    {
        if (c.test(i))
        {
            if (!done && nfaStateFactory[i]->done)
                done = true, doneNfaStateId = i;
            else
                cont = true;
        }
    }
    if (done)
    {
        for (size_t i = 0; i <= doneNfaStateId; ++i)
        {
            if (nfaStateFactory[i]->done)
                ++goodBranch;
        }
    }
    
    return { false, cont, goodBranch };
}

TransitionTable CompileContext::Compile()
{
    TransitionTable t;

    std::deque<NfaStateSet> q;
    std::map<NfaStateSet, TableState, NfaStateSetComparator> toTableState;
    TableState nextTableState = 0;

    NfaStateSet c0;
    for (NfaGraph & g : nfaGraphs)
        c0.set(g.in->id);
    c0 = EpsilonClosure(c0);
    q.push_front(c0);
    toTableState.emplace(c0, nextTableState++);

    size_t row = 0;
    while (!q.empty())
    {
        if (t.nextStateTable.size() < row + 1)
            t.nextStateTable.resize(row + 1);

        NfaStateSet c1 = q.back();
        q.pop_back();
        for (char ch : CharSet::Chars())
        {
            NfaStateSet c2 = EpsilonClosure(Next(c1, ch));

            auto iter = toTableState.find(c2);
            if (iter == toTableState.end())
            {
                q.push_front(c2);
                iter = toTableState.emplace_hint(iter, c2, nextTableState++);
            }

            t.nextStateTable[row][CharSet::CharIdx(ch)] = iter->second;
        }

        ++row;
    }

    t.matchStatusTable.resize(row);
    t.matchedBranchTable.resize(row);
    t.nfaStateSetTable.resize(row);

    // Convert Node id to branch number.
    for (auto cl_id : toTableState)
    {
        MatchStatus m = GetState(cl_id.first);
        t.matchStatusTable[cl_id.second] = m;
        t.nfaStateSetTable[cl_id.second] = cl_id.first;
        t.matchedBranchTable[cl_id.second] = m.goodBranch;
    }

    return t;
}

size_t TransitionTable::Run(std::string & text)
{
    std::cout << "match \"" << text << "\"..." << std::endl;
    
    TableState st = 0;
    size_t i = 0;

    size_t matchedBranch = 0;
    size_t matchedOffset = 0;

    if (this->matchStatusTable[st].cont)
    {
        for (;
             i < text.size();
             ++i)
        {
            char ch = text.data()[i];
            size_t next_state = this->nextStateTable[st][CharSet::CharIdx(ch)];
            if (!this->matchStatusTable[st].bad)
            {
                st = next_state;
                std::cout << st << NfaStateSetToString(this->nfaStateSetTable[st]) << " --" << CharSet::CharStr(ch) << "-> " << next_state << NfaStateSetToString(this->nfaStateSetTable[next_state]) << std::endl;
                
                if (matchedBranch == 0 ||
                    (0 < this->matchStatusTable[st].goodBranch && this->matchStatusTable[st].goodBranch <= matchedBranch))
                {
                    matchedBranch = this->matchStatusTable[st].goodBranch;
                    matchedOffset = i + 1;
                }
            }
            else
                break;
        }
    }

    // matchedOffset points to first unmatched char.

    if (matchedBranch > 0)
    {
        if (matchedOffset == text.size())
            std::cout << "matched pattern #" << matchedBranch << "." << std::endl;
        else
            std::cout << "prefix \"" << text.substr(0, matchedOffset) << "\" matched pattern #" << matchedBranch << "." << std::endl;
    }
    else
    {
        std::cout << "not matched." << std::endl;
    }
    return matchedBranch;
}

// ===================================================================
// Test
// ===================================================================

// Convert 1 pattern list to transition table
void Test_ConvertOnePattern()
{
    CompileContext context;
    NfaStateFactoryScope scope(&context.nfaStateFactory);

    // ab*c+d?e
    NfaGraph c = Done(
        Concat(
            Concat(
                Concat(
                    Concat(
                        MatchChar('a'),
                        Rep0(
                            MatchChar('b')
                        )
                    ),
                    Rep1(
                        MatchChar('c')
                    )
                ),
                Opt(
                    MatchChar('d')
                )
            ),
            MatchChar('e')
        )
    );
    context.nfaGraphs.push_back(c);
    context.PrintNodes();

    // Compile.
    TransitionTable tt = context.Compile();
    PrintTable(tt);

    // Simulate.
    std::vector<std::pair<std::string, bool>> test_cases = {
        { "ace", true },
        { "abce", true },
        { "abbce", true },
        { "abbbce", true },
        { "acce", true },
        { "acde", true },
        { "ce", false },
        { "ae", false },
        { "ac", false },
    };
    for (auto kv : test_cases)
    {
        auto & text = kv.first;
        bool result = kv.second;
        std::cout << "pattern: ab*c text: " << text << std::endl;
        assert((tt.Run(text) == 1) == result);
    }
}

void Test_ConvertOnePattern2()
{
    CompileContext context;
    NfaStateFactoryScope scope(&context.nfaStateFactory);

    // (a|b)+
    NfaGraph c = MatchChar('a');
    c = Alt(c, MatchChar('b'));
    c = Rep1(c);
    c = Done(c);
    context.nfaGraphs.push_back(c);
    context.PrintNodes();

    // Compile.
    TransitionTable tt = context.Compile();
    PrintTable(tt);

    // Simulate.
    std::vector<std::pair<std::string, bool>> test_cases = {
        { "a", true },
        { "b", true },
        { "ab", true },
        { "ba", true },
        { "baz", true },
    };
    for (auto kv : test_cases)
    {
        auto & text = kv.first;
        bool result = kv.second;
        std::cout << "pattern: (a|b)+ text: " << text << std::endl;
        assert((tt.Run(text) == 1) == result);
    }
}

// Convert N pattern list to transition table
void Test_ConvertMultiplePattern()
{
    CompileContext context;
    NfaStateFactoryScope scope(&context.nfaStateFactory);

    std::vector<NfaGraph> vc = {
        FromRegex("apple"),
        FromRegex("banan")
    };
    context.nfaGraphs = vc;
    context.PrintNodes();

    // Compile.
    TransitionTable tt = context.Compile();
    PrintTable(tt);

    // Simulate.
    std::vector<std::pair<std::string, size_t>> test_cases = {
        { "apple", 1 },
        { "banan", 2 },
        { "appla", 0 },
        { "banana", 2 },
    };
    for (auto kv : test_cases)
    {
        auto & text = kv.first;
        auto which = kv.second;
        std::cout << "text: " << text << std::endl;
        assert(tt.Run(text) == which);
    }
}

void Test_ConvertMultiplePattern2()
{
    CompileContext context;
    NfaStateFactoryScope scope(&context.nfaStateFactory);

    std::vector<NfaGraph> vc = {
        FromRegex("app"),
        FromRegex("ap"),
        FromRegex("apple"),
        FromRegex("a"),
        FromRegex("appl"),
    };
    context.nfaGraphs = vc;
    context.PrintNodes();

    // Compile.
    TransitionTable tt = context.Compile();
    PrintTable(tt);

    // Simulate.
    std::vector<std::pair<std::string, size_t>> test_cases = {
        { "app", 1 },
        { "ap", 2 },
        { "apple", 1 },
        { "a", 4 },
        { "appl", 1 },
    };
    for (auto kv : test_cases)
    {
        auto & text = kv.first;
        auto which = kv.second;
        std::cout << "text: " << text << std::endl;
        assert(tt.Run(text) == which);
    }
}

void Test_ConvertMultiplePattern3()
{
    CompileContext context;
    NfaStateFactoryScope scope(&context.nfaStateFactory);

    std::vector<NfaGraph> vc = {
        FromRegex("abcde"),
        FromRegex("xy"),
        FromRegex("(a|b|c|d|e|f|g|h|i|j|k|l|m|n|o|p|q|r|s|t|u|v|w|x|y)+")
    };
    context.nfaGraphs = vc;
    context.PrintNodes();

    // Compile.
    TransitionTable tt = context.Compile();
    PrintTable(tt);

    // Simulate.
    std::vector<std::pair<std::string, size_t>> test_cases = {
        { "abcde", 1 },
        { "xy", 2 },
        { "hello", 3 },
        { "helloz", 3 },
    };
    for (auto kv : test_cases)
    {
        auto & text = kv.first;
        auto which = kv.second;
        std::cout << "text: " << text << std::endl;
        assert(tt.Run(text) == which);
    }
}

void Test_Compile(std::string pattern)
{
    CompileContext context;
    NfaStateFactoryScope scope(&context.nfaStateFactory);

    std::vector<NfaGraph> vc = {
        FromRegex(pattern),
    };
    context.nfaGraphs = vc;
    context.PrintNodes();

    // Compile.
    TransitionTable tt = context.Compile();
    PrintTable(tt);
}

void Test_CLex()
{
    CompileContext context;
    NfaStateFactoryScope scope(&context.nfaStateFactory);

    std::vector<NfaGraph> vc = {
        FromRegex("while"),
        FromRegex("volatile"),
        FromRegex("void"),
        FromRegex("unsigned"),
        FromRegex("union"),
        FromRegex("typedef"),
        FromRegex("switch"),
        FromRegex("struct"),
        FromRegex("static"),
        FromRegex("sizeof"),
        FromRegex("signed"),
        FromRegex("short"),
        FromRegex("return"),
        FromRegex("register"),
        FromRegex("long"),
        FromRegex("int"),
        FromRegex("if"),
        FromRegex("goto"),
        FromRegex("for"),
        FromRegex("float"),
        FromRegex("extern"),
        FromRegex("enum"),
        FromRegex("else"),
        FromRegex("double"),
        FromRegex("do"),
        FromRegex("default"),
        FromRegex("continue"),
        FromRegex("const"),
        FromRegex("char"),
        FromRegex("case"),
        FromRegex("break"),
        FromRegex("auto"),
        FromRegex("(a|b|c|d|e|f|g|h|i|j|k|l|m|n|o|p|q|r|s|t|u|v|w|x|y|z)+"),
    };
    context.nfaGraphs = vc;
    //PrintNodes();

    TransitionTable tt = context.Compile();
    PrintTable(tt);

    std::vector<std::pair<std::string, size_t>> test_cases = {
        { "while", 1 },
        { "volatile", 2 },
        { "void", 3 },
        { "unsigned", 4 },
        { "union", 5 },
        { "typedef", 6 },
        { "switch", 7 },
        { "struct", 8 },
        { "static", 9 },
        { "sizeof", 10 },
        { "signed", 11 },
        { "short", 12 },
        { "return", 13 },
        { "register", 14 },
        { "long", 15 },
        { "int", 16 },
        { "if", 17 },
        { "goto", 18 },
        { "for", 19 },
        { "float", 20 },
        { "extern", 21 },
        { "enum", 22 },
        { "else", 23 },
        { "double", 24 },
        { "do", 25 },
        { "default", 26 },
        { "continue", 27 },
        { "const", 28 },
        { "char", 29 },
        { "case", 30 },
        { "break", 31 },
        { "auto", 32 },
        { "justanidname", 33 },
    };
    for (auto kv : test_cases)
    {
        auto & text = kv.first;
        auto which = kv.second;
        std::cout << "pattern: C-Lex text: " << text << std::endl;
        assert(tt.Run(text) == which);
    }
}
