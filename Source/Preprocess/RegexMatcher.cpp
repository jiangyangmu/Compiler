#include "RegexMatcher.h"

#include <cassert>
#include <bitset>
#include <unordered_map>
#include <array>
#include <deque>
#include <memory>
#include <iostream>
#include <fstream>
#include <string>
#include <numeric>

class CharSet
{
public:
    static const size_t N = 128;
    static const std::vector<char> & Chars()
    {
        static const std::vector<char> chars = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127 };
        return chars;
    }

    static const std::vector<const char *> & CharStrs()
    {
        static const std::vector<const char *> char_strs = { "NUL", "SOH", "STX", "ETX", "EOT", "ENQ", "ACK", "BEL", "BS", "TAB", "LF", "VT", "FF", "CR", "SO", "SI", "DLE", "DC1", "DC2", "DC3", "DC4", "NAK", "SYN", "ETB", "CAN", "EM", "SUB", "ESC", "FS", "GS", "RS", "US", "SP", "!", "\"", "#", "$", "%", "&", "'", "(", ")", "*", "+", ",", "-", ".", "/", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", ":", ";", "<", "=", ">", "?", "@", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "[", "\\", "]", "^", "_", "`", "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "{", "|", "}", "~", "DEL", };
        return char_strs;
    }

    static const std::vector<size_t> & CharIdxs()
    {
        static const std::vector<size_t> char_idxs = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127 };
        return char_idxs;
    }

    static bool Contains(char ch)
    {
        return 0 <= ch && ch < 128;
    }

    static size_t CharIdx(char ch)
    {
        if (!Contains(ch))
            std::cerr << "Unknown char: " << ch << std::endl;
        assert(Contains(ch));
        return ch;
    }

    static const char * CharStr(char ch)
    {
        assert(Contains(ch));
        return CharStrs()[CharIdx(ch)];
    }

};

// Nfa

struct NfaState
{
    size_t id;
    bool done;

    char * c;
    NfaState * next;
    NfaState * alt;
};

typedef std::bitset<4096> NfaStateSet;

struct Nfa
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
    const std::vector<std::unique_ptr<NfaState>> & States() const { return v; }
private:
    std::vector<std::unique_ptr<NfaState>> v;
};

// Dfa

typedef size_t DfaState;

struct DfaAction
{
    bool bad;
    bool cont;
    size_t goodBranch;
};

struct DfaMatchResult
{
    size_t offset;
    size_t length; // 0: no match
    size_t which;  // 0: no match, 1: match 1st pattern, ...
};

typedef std::array<size_t, CharSet::N> DfaTableRow;

struct Dfa
{
    // (Table-State, Char) to (Table-State)
    std::vector<DfaTableRow> table;
    // (Table-State) to (Match-Status)
    std::vector<DfaAction> action;
};

// Dfa build

struct DfaCompileInput
{
    NfaStateFactory nfaStateFactory;
    std::vector<Nfa> nfaList;
};

struct DfaCompileContext
{
    const std::vector<Nfa> & nfas;
    const NfaStateFactory & nfaStates;
    std::unordered_map<const NfaState *, NfaStateSet> c1;
    std::unordered_map<NfaStateSet, NfaStateSet> c2;
};

class NfaStateSetToDfaState
{
public:
    NfaStateSetToDfaState() : nextDS(0) {}

    DfaState Add(const NfaStateSet & nss)
    {
        auto result = m.try_emplace(nss, nextDS);
        if (result.second)
        {
            v.emplace_back(result.first->first);
            ++nextDS;
        }
        return result.first->second;
    }
    DfaState Guard() const
    {
        return nextDS;
    }
    const NfaStateSet & Get(DfaState ds)
    {
        return v.at(ds);
    }
    const std::unordered_map<NfaStateSet, DfaState> & AsMap() const
    {
        return m;
    }

private:
    DfaState nextDS;
    std::unordered_map<NfaStateSet, DfaState> m;
    std::vector<NfaStateSet> v;
};

// ==== Print ====

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
std::string NfaGraphToString(NfaState * ns)
{
    std::string s;
    s += "(";
    if (ns)
    {
        if (ns->done)
        {
            assert(!ns->c);
            s += "DONE";
        }
        else
        {
            s += ns->c ? std::string(CharSet::CharStr(*ns->c)) : "EPS";
        }
        s += " ";
        s += NfaGraphToString(ns->next);
        s += " ";
        s += NfaGraphToString(ns->alt);
    }
    else
    {
        s += "NULL";
    }
    s += ")";
    return s;
}
std::string NfaGraphToString(Nfa g)
{
    return NfaGraphToString(g.in);
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
NfaStateSet EpsilonClosure(NfaState * ns, DfaCompileContext & context);
void PrintDfaCompileContext(DfaCompileContext & context)
{
    for (const auto & n : context.nfaStates.States())
    {
        std::cout
            << n->id << (n->done ? "(done)" : (n->c ? "(non-empty)" : "")) << ":" << std::endl
            << "  --" << (n->c ? CharSet::CharStr(*n->c) : "-") << "--> " << (n->next ? std::to_string(n->next->id) : "null") << std::endl
            << "  -----> " << (n->alt ? std::to_string(n->alt->id) : "null") << std::endl;
    }
    for (const auto & n : context.nfaStates.States())
    {
        std::cout
            << n->id << (n->done ? "(done)" : "") << ": ";
        PrintNfaStateSet(EpsilonClosure(n.get(), context));
    }
}
void PrintDfa(Dfa & dfa)
{
    const int w = 4;

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
    for (auto row : dfa.table)
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
    for (auto row : dfa.action)
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
        std::cout << /*"\t" << NfaStateSetToString(dfa.dfa2nfa[state]) <<*/ std::endl;

        ++state;
    }
}

// ==== Build NFA ====

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

Nfa MatchEmpty()
{
    NfaState * n = gFactory->NewState();
    return { n, &n->next, &n->alt };
}
Nfa MatchChar(char c)
{
    NfaState * n = gFactory->NewState();
    n->c = new char(c);

    return { n, &n->next, &n->alt };
}
Nfa Concat(Nfa c1, Nfa c2)
{
    *c1.next = c2.in;

    return { c1.in, c2.next , c1.alt };
}
Nfa Alt(Nfa c1, Nfa c2)
{
    NfaState * n = gFactory->NewState();
    *c1.next = n;
    *c1.alt = c2.in;
    *c2.next = n;

    return { c1.in, &n->next, c2.alt };
}
Nfa Rep0(Nfa c)
{
    NfaState * n = gFactory->NewState();
    n->alt = c.in;
    *c.next = n;

    return { n, &n->next, c.alt };
}
Nfa Rep1(Nfa c)
{
    NfaState * n = gFactory->NewState();
    n->alt = c.in;
    *c.next = n;

    return { c.in, &n->next, c.alt };
}
Nfa Opt(Nfa c)
{
    NfaState * n = gFactory->NewState();
    *c.next = n;
    *c.alt = n;

    return { c.in, &n->next, &n->alt };
}
Nfa Done(Nfa c)
{
    NfaState * n = gFactory->NewState();
    n->done = true;
    *c.next = n;

    return { c.in, &n->next, c.alt };
}

// ==== Regex -> NFA ====

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
#define REGEX_EXPECT_NOT(p, expect) \
    do { \
        if (*(p) == (expect)) \
            throw std::invalid_argument("bad regex"); \
    } while (false)
#define REGEX_EXPECT_TRUE(cond) \
    do { \
        if (!(cond)) \
            throw std::invalid_argument("bad regex"); \
    } while (false)

Nfa FromRegexCharGroup(const char * & c)
{
    // '[' '^'? (char|char '-' char)+ ']'
    std::vector<bool> v(CharSet::N, false);
    bool revert = false;

    REGEX_EXPECT(c, '[');
    ++c;

    if (*c == '^')
    {
        revert = true;
        ++c;
    }

    REGEX_EXPECT_NOT(c, ']');
    do
    {
        REGEX_EXPECT_TRUE(CharSet::Contains(*c));
        if (*c == '\\')
        {
            ++c;
            REGEX_EXPECT_TRUE(CharSet::Contains(*c));
            v[CharSet::CharIdx(*c)] = true;
            ++c;
        }
        else
        {
            if (*(c + 1) == '-')
            {
                char cb = *c;
                char ce = *(c + 2);
                REGEX_EXPECT_TRUE(cb < ce);
                while (cb <= ce)
                    v[CharSet::CharIdx(cb)] = true, ++cb;
                c += 2;
            }
            else
            {
                v[CharSet::CharIdx(*c)] = true;
                ++c;
            }
        };
    } while (*c != ']');

    REGEX_EXPECT(c, ']');
    ++c;

    Nfa g;
    size_t i = 0;
    for (; i != v.size(); ++i)
    {
        if (v[i] != revert)
        {
            g = MatchChar(CharSet::Chars()[i]);
            ++i;
            break;
        }
    }
    for (; i != v.size(); ++i)
    {
        if (v[i] != revert)
        {
            g = Alt(g, MatchChar(CharSet::Chars()[i]));
        }
    }

    return g;
}
// char, escape, wildcard
Nfa FromRegexChar(const char * & c)
{
    if (*c == '[')
        return FromRegexCharGroup(c);

    Nfa g;

    REGEX_EXPECT_TRUE(CharSet::Contains(*c));
    if (*c == '\\')
    {
        ++c;
        REGEX_EXPECT_TRUE(CharSet::Contains(*c));
        g = MatchChar(*c);
        ++c;
    }
    else if (*c == '.')
    {
        g = MatchChar(CharSet::Chars()[0]);
        for (size_t i = 1; i < CharSet::N; ++i)
            g = Alt(g, MatchChar(CharSet::Chars()[i]));
        ++c;
    }
    else
    {
        g = MatchChar(*c);
        ++c;
    }

    return g;
}
Nfa FromRegexAltGroup(const char * & c);
Nfa FromRegexConcatList(const char * & c)
{
    // ((char | alt-group) repeat?)+

    Nfa g;

    REGEX_EXPECT_NOT(c, '|');
    REGEX_EXPECT_NOT(c, ')');
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
        Nfa g2;

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
Nfa FromRegexAltGroup(const char * & c)
{
    // '(' concat-list? ('|' concat-list?)* ')'

    Nfa g;

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
Nfa FromRegex(std::string regex)
{
    regex = "(" + regex + ")";

    const char * c = regex.data();
    return Done(FromRegexAltGroup(c));
}

// ==== Subset Construction ====

NfaStateSet Init(DfaCompileContext & context)
{
    NfaStateSet nss;
    for (const Nfa & g : context.nfas)
        nss.set(g.in->id);
    return nss;
}
// All reachable Node and self that are non-empty or done
NfaStateSet EpsilonClosure(NfaState * ns, DfaCompileContext & context)
{
    auto kv = context.c1.find(ns);
    if (kv != context.c1.end())
        return kv->second;

    NfaStateSet nss;
    NfaStateSet mask;
    std::deque<NfaState *> q = { ns };
    while (!q.empty())
    {
        NfaState * n = q.back();
        q.pop_back();

        if (mask.test(n->id))
            continue;
        mask.set(n->id);

        if (n->c || n->done) // non-empty or done
        {
            nss.set(n->id);
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

    return context.c1.emplace_hint(kv, ns, nss)->second;
}
NfaStateSet EpsilonClosure(const NfaStateSet & nss, DfaCompileContext & context)
{
    NfaStateSet nss2;

    auto kv = context.c2.find(nss);
    if (kv != context.c2.end())
    {
        nss2 = kv->second;
    }
    else
    {
        for (size_t i = 0; i < nss.size(); ++i)
        {
            if (nss[i])
            {
                nss2 |= EpsilonClosure(context.nfaStates[i], context);
            }
        }
        context.c2.emplace_hint(kv, nss, nss2);
    }

    return nss2;
}
NfaStateSet Jump(const NfaStateSet & nss, char ch, DfaCompileContext & context)
{
    NfaStateSet nss2;
    for (size_t i = 0; i < nss.size(); ++i)
    {
        if (nss[i])
        {
            NfaState * n = context.nfaStates[i];
            assert(n->done || n->c);
            if (n->done)
                /*c2.set(n->id)*/;
            else if (*n->c == ch && n->next)
                nss2.set(n->next->id);
        }
    }
    return nss2;
}
// Die? Cont? Which rule (prefer given first)?
DfaAction GetAction(const NfaStateSet & nss, DfaCompileContext & context)
{
    if (nss.none())
        return { true, false, 0 };

    bool cont = false;
    size_t goodBranch = 0;

    bool done = false;
    size_t doneNfaStateId = 0;
    for (size_t i = 0; i < nss.size() && (!cont || !goodBranch); ++i)
    {
        if (nss.test(i))
        {
            if (!done && context.nfaStates[i]->done)
                done = true, doneNfaStateId = i;
            else
                cont = true;
        }
    }
    if (done)
    {
        for (size_t i = 0; i <= doneNfaStateId; ++i)
        {
            if (context.nfaStates[i]->done)
                ++goodBranch;
        }
    }

    return { false, cont, goodBranch };
}

#define DFA_EXPECT_TRUE(cond) \
    do { \
        if (!(cond)) \
            throw std::invalid_argument("bad DFA"); \
    } while (false)

Dfa Compile(DfaCompileInput & input)
{
    std::vector<DfaTableRow> dfaTable;

    DfaCompileContext context = { input.nfaList, input.nfaStateFactory };
    NfaStateSetToDfaState nfa2dfa;
    NfaStateSet nss;
    DfaState ds;

    //PrintDfaCompileContext(context);

    nss =
        EpsilonClosure(
            Init(context),
            context
        );
    ds = nfa2dfa.Add(nss);
    for (; ds < nfa2dfa.Guard(); ++ds)
    {
        nss = nfa2dfa.Get(ds);
        dfaTable.emplace_back();
        for (char ch : CharSet::Chars())
        {
            NfaStateSet nss2 =
                EpsilonClosure(
                    Jump(nss, ch, context),
                    context
                );
            dfaTable[ds][CharSet::CharIdx(ch)] =
                nfa2dfa.Add(nss2);
        }
    }
    assert(ds < nss.size());

    Dfa dfa;

    dfa.table = std::move(dfaTable);
    dfa.action.resize(dfa.table.size());
    for (auto nss_ds : nfa2dfa.AsMap())
    {
        dfa.action[nss_ds.second] = GetAction(nss_ds.first, context);
    }

    return dfa;
}

DfaMatchResult Match(Dfa & dfa, const char * begin, const char * end)
{
    DfaState ds = 0;

    size_t matchedBranch = 0;
    size_t matchedLength = 0;

    assert (!dfa.action[ds].bad && dfa.action[ds].cont && dfa.action[ds].goodBranch == 0);
    for (const char * pos = begin;
         pos != end;
         ++pos)
    {
        char ch = *pos;
        size_t ds2 = dfa.table[ds][CharSet::CharIdx(ch)];

        ds = ds2;

        if (dfa.action[ds].bad)
            break;

        if (dfa.action[ds].goodBranch)
        {
            matchedBranch = dfa.action[ds].goodBranch;
            matchedLength = pos - begin + 1;
        }
    }

    // Offset filled by client.
    return { 0, matchedLength, matchedBranch };
}

// ==== API ====

Dfa * NewDfa()
{
    static std::vector<std::unique_ptr<Dfa>> gDfaPool;
    return gDfaPool.emplace_back(new Dfa).get();
}

void SaveToFile(std::string fileName, const Dfa & dfa)
{
    std::ofstream ofs(fileName, std::ofstream::out);

    ofs << dfa.table.size() << ' ';
    for (const DfaTableRow & tableRow : dfa.table)
    {
        for (size_t state : tableRow)
            ofs << state << ' ';
    }

    ofs << dfa.action.size() << ' ';
    for (const DfaAction & action : dfa.action)
    {
        ofs << action.bad << ' ' << action.cont << ' ' << action.goodBranch << ' ';
    }
}
bool LoadFromFile(std::string fileName, Dfa * dfa)
{
    std::ifstream ifs(fileName, std::ifstream::in);
    if (!ifs.is_open())
        return false;

    size_t tableSize = 0;
    ifs >> tableSize;
    assert(tableSize < 10000);

    dfa->table.resize(tableSize);
    for (DfaTableRow & tableRow : dfa->table)
    {
        for (size_t & state : tableRow)
            ifs >> state;
    }

    size_t actionSize = 0;
    ifs >> actionSize;
    assert(actionSize < 10000);

    dfa->action.resize(actionSize);
    for (DfaAction & action : dfa->action)
    {
        ifs >> action.bad >> action.cont >> action.goodBranch;
    }

    return true;
}

MatchEngine Compile(std::vector<std::string> & patterns)
{
    MatchEngine m;

    DfaCompileInput input;
    NfaStateFactoryScope scope(&input.nfaStateFactory);

    input.nfaList.reserve(patterns.size());
    for (auto & p : patterns)
        input.nfaList.emplace_back(FromRegex(p));

    // No cache
    // this->dfa = new Dfa(std::move(::Compile(input)));

    // With cache
    m.dfa = NewDfa();
    const std::string cacheFile = "lex_cache.bin";
    if (LoadFromFile(cacheFile, m.dfa))
    {
        std::cout << "Load lex cache from file: " << cacheFile << std::endl;
    }
    else
    {
        *m.dfa = std::move(::Compile(input));
        SaveToFile(cacheFile, *m.dfa);
        std::cout << "Save lex cache to file: " << cacheFile << std::endl;
    }

    return m;
}

MatchResult MatchPrefix(MatchEngine m, StringView text)
{
    assert(m.dfa);

    DfaMatchResult r = ::Match(*m.dfa, text.Begin(), text.End());

    return { r.offset, r.length, r.which };
}

std::vector<MatchResult> MatchAll(MatchEngine m, StringView text)
{
    assert(m.dfa);

    std::vector<MatchResult> mrs;

    const char * begin = text.Begin();
    const char * end = text.End();
    const char * pos = begin;
    while (pos < end)
    {
        DfaMatchResult r = ::Match(*m.dfa, pos, end);
        r.offset = pos - begin;

        if (r.length == 0)
        {
            std::string msg = "Unexpected char: ";
            msg.push_back(*pos);
            throw std::invalid_argument(msg);
        }

        mrs.push_back({ r.offset, r.length, r.which });
        pos += r.length;
    }

    return mrs;
}

std::vector<MatchResult> MatchAll(std::vector<std::string> patterns, StringView text)
{
    DfaCompileInput input;
    NfaStateFactoryScope scope(&input.nfaStateFactory);

    input.nfaList.reserve(patterns.size());
    for (auto & p : patterns)
        input.nfaList.emplace_back(FromRegex(p));

    Dfa dfa = Compile(input);
    //PrintDfa(dfa);

    std::vector<MatchResult> m;

    const char * begin = text.Begin();
    const char * end = text.End();
    const char * pos = begin;
    while (pos < end)
    {
        DfaMatchResult r = Match(dfa, pos, end);
        r.offset = pos - begin;

        if (r.length == 0)
        {
            std::string msg = "Unexpected char: ";
            msg.push_back(*pos);
            throw std::invalid_argument(msg);
        }

        m.push_back({r.offset, r.length, r.which});
        pos += r.length;
    }

    return m;
}

#ifdef UNIT_TEST
#include "../UnitTest/UnitTest.h"

TEST(RegexMatcher_Escape)
{
    try
    {
        std::vector<std::string> patterns = {
            "+=",
            "+\\+",
            "+",
            "[ \t\r]+",
            "\n",
        };
        std::vector<std::string> types = {
            "ADD1",
            "ADD2",
            "ADD3",
            "SPACE",
            "NEW_LINE",
        };
        std::string text =
            "+= ++ + "
            " \t\r"
            "\n\n"
            ;
        std::vector<MatchResult> mr = MatchAll(patterns, StringView(text.data(), text.length()));

        for (auto r : mr)
        {
            if (r.which > 0)
                std::cout << "Matched: " << types[r.which - 1] << " " << text.substr(r.offset, r.length) << std::endl;
        }
    }
    catch (const std::invalid_argument& e)
    {
        std::cerr << "Caught exception: " << e.what() << std::endl;
    }
}

TEST(RegexMatcher_Complete)
{
    try
    {
        std::vector<std::string> patterns = {
            "if",
            "[_a-zA-Z][_a-zA-Z0-9]*",
            "#[_a-zA-Z][_a-zA-Z0-9]*",
            "[0-9]+\\.[0-9]*|\\.[0-9]+",
            "[0-9]+",
            "'([^']|\\\\')+'|\"([^\"]|\\\\\")*\"|<([^> ]|\\\\>)+>",
            "~|}|\\|\\||\\|=|\\||{|^=|^|]|\\[|\\?|>>=|>>|>=|>|==|=|<=|<<=|<<|<|;|:|/=|/|\\.\\.\\.|\\.|->|-=|--|-|,|+=|++|+|*=|*|\\)|\\(|&=|&&|&|%=|%|##|#|!=|!",
            "[ \t\r]+",
            "\n",
        };
        std::vector<std::string> types = {
            "IF",
            "ID",
            "DIRECTIVE",
            "FLOAT",
            "INT",
            "RANGE",
            "OP",
            "SPACE",
            "NEW_LINE",
        };
        std::string text =
            "if "
            "if_as_an_id hello_0_this_Is_a_Long_Name good "
            "#hello_0_this_Is_a_Long_Directive #haha "
            "0. 0.0 .0 "
            "0 1 22 4890 "
            "'a0=>?\"\\'' \"a0=>?'\\\"\" <a0=\\>?'\">"
            "~ } || |= | { ^= ^ ] [ ? >>= >> >= > == = <= <<= << < ; : /= / ... . -> -= -- - , += ++ + *= * ) ( &= && & %= % ## # != ! "
            " \t\r"
            "\n\n"
            ;
        std::vector<MatchResult> mr = MatchAll(patterns, StringView(text.data(), text.length()));

        for (auto r : mr)
        {
            if (r.which > 0)
                std::cout << "Matched: " << types[r.which - 1] << " " << text.substr(r.offset, r.length) << std::endl;
        }
    }
    catch (const std::invalid_argument& e)
    {
        std::cerr << "Caught exception: " << e.what() << std::endl;
    }
}

#endif