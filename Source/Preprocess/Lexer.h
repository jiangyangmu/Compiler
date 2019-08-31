#pragma once

#include <cassert>
#include <string>
#include <vector>
#include <bitset>
#include <unordered_map>
#include <array>
#include <deque>
#include <memory>
#include <iostream>

class CharSet
{
public:
    static const size_t N = 27;
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

// Nfa

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

    // debug
    std::vector<NfaStateSet> dfa2nfa;

    DfaMatchResult Run(std::string & text);
};

// Dfa build

struct DfaInput
{
    NfaStateFactory nfaStateFactory;
    std::vector<NfaGraph> nfaGraphs;
};

struct DfaCompileContext
{
    const std::vector<NfaGraph> & nfaGraphs;
    const NfaStateFactory & nfaStates;
    std::unordered_map<const NfaState *, NfaStateSet> c1;
    std::unordered_map<NfaStateSet, NfaStateSet> c2;
};

class NfaStateSetToDfaState
{
public:
    NfaStateSetToDfaState() : ds(0) {}

    DfaState Add(const NfaStateSet & nss)
    {
        auto result = m.try_emplace(nss, ds);
        if (result.second)
        {
            v.emplace_back(result.first->first);
            ++ds;
        }
        return result.first->second;
    }
    DfaState Guard() const
    {
        return ds;
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
    DfaState ds;
    std::unordered_map<NfaStateSet, DfaState> m;
    std::vector<NfaStateSet> v;
};

// ===================================================================
// Print
// ===================================================================

std::string NfaStateSetToString(NfaStateSet c);
std::string NfaGraphToString(NfaState * ns);
std::string NfaGraphToString(NfaGraph g);
void PrintNfaStateSet(NfaStateSet c);
void PrintDfaCompileContext(DfaCompileContext & context);
void PrintDfa(Dfa & dfa);

// ===================================================================
// Build epsilon NFA
// ===================================================================

extern NfaStateFactory * gFactory;

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

NfaGraph MatchEmpty();
NfaGraph MatchChar(char c);
NfaGraph Concat(NfaGraph c1, NfaGraph c2);
NfaGraph Alt(NfaGraph c1, NfaGraph c2);
NfaGraph Rep0(NfaGraph c);
NfaGraph Rep1(NfaGraph c);
NfaGraph Opt(NfaGraph c);
NfaGraph Done(NfaGraph c);

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
NfaGraph FromRegex(std::string regex);
std::string ToRegex(NfaGraph g);

// ===================================================================
// Build DFA
// ===================================================================

NfaStateSet Init(DfaCompileContext & context);
// All reachable Node and self that are non-empty or done
NfaStateSet EpsilonClosure(NfaState * ns, DfaCompileContext & context);
NfaStateSet EpsilonClosure(const NfaStateSet & nss, DfaCompileContext & context);
NfaStateSet Jump(const NfaStateSet & nss, char ch, DfaCompileContext & context);
DfaAction GetAction(const NfaStateSet & nss, DfaCompileContext & context);

Dfa Compile(DfaInput & input);
