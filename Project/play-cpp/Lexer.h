#pragma once

#include <cassert>
#include <string>
#include <vector>
#include <bitset>
#include <map>
#include <array>
#include <deque>

/*
    Problem:

        pattern 1   => Context, Result Object A
        pattern 2   => Context, Result Object B
        ...
        ELSE        => Context, Result Function

        * if matches pattern 1 & 2, prefer 1

    Example:

        proc        => proc keyword
        [_a-zA-Z]+  => id
        [0-9]+      => num
        ELSE        => throw error

*/

typedef std::bitset<32> Closure;

struct Node
{
    size_t id;
    bool done;

    char c;
    Node * next;
    Node * alt;
};

struct NodeGroup
{
    Node * in;
    Node ** next;
    Node ** alt;
};

std::vector<Node *> NodeFactory;

Node * NewNode()
{
    Node * n = new Node;
    n->id = NodeFactory.size();
    n->done = false;
    n->c = 0;
    n->next = nullptr;
    n->alt = nullptr;

    NodeFactory.push_back(n);

    return n;
}

// ===================================================================
// Build epsilon NFA
// ===================================================================

NodeGroup MatchEmpty()
{
    Node * n = NewNode();

    return { n, &n->next, &n->alt };
}

NodeGroup MatchChar(char c)
{
    Node * n = NewNode();
    n->c = c;

    return { n, &n->next, &n->alt };
}

NodeGroup Concat(NodeGroup c1, NodeGroup c2)
{
    *c1.next = c2.in;

    return { c1.in, c2.next , c1.alt };
}

NodeGroup Alt(NodeGroup c1, NodeGroup c2)
{
    Node * n = NewNode();
    *c1.next = n;
    *c1.alt = c2.in;
    *c2.next = n;

    return { c1.in, c2.alt, &n->alt };
}

NodeGroup Rep0(NodeGroup c)
{
    Node * n = NewNode();
    n->alt = c.in;
    c.in->next = n;

    return { n, &n->next, c.alt };
}

NodeGroup Rep1(NodeGroup c)
{
    Node * n = NewNode();
    n->alt = c.in;
    c.in->next = n;

    return { c.in, &n->next, c.alt };
}

NodeGroup Opt(NodeGroup c)
{
    Node * n = NewNode();
    *c.next = n;
    *c.alt = n;

    return { c.in, &n->next, &n->alt };
}

NodeGroup Done(NodeGroup c)
{
    Node * n = NewNode();
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

NodeGroup FromRegexChar(const char * & c);
NodeGroup FromRegexConcatList(const char * & c);
NodeGroup FromRegexAltGroup(const char * & c);

NodeGroup FromRegexChar(const char * & c)
{
    NodeGroup g;

    REGEX_EXPECT_TRUE('a' <= *c && *c <= 'z');
    g = MatchChar(*c);
    ++c;

    return g;
}
NodeGroup FromRegexConcatList(const char * & c)
{
    // ((char | alt-group) repeat?)+
    
    NodeGroup g;
    
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
        if (*c == '(')
            g = Concat(g, FromRegexAltGroup(c));
        else
            g = Concat(g, FromRegexChar(c));
        switch (*c)
        {
            case '*': g = Rep0(g); ++c; break;
            case '+': g = Rep1(g); ++c; break;
            case '?': g = Opt(g); ++c; break;
            default: break;
        }
    }

    return g;
}
NodeGroup FromRegexAltGroup(const char * & c)
{
    // '(' concat-list? ('|' concat-list?)* ')'
    
    NodeGroup g;

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
NodeGroup FromRegex(std::string regex)
{
    if (regex.empty() || regex.at(0) != '(')
        regex = "(" + regex + ")";

    const char * c = regex.data();
    return Done(FromRegexAltGroup(c));
}
std::string ToRegex(NodeGroup g);

// ===================================================================
// Simulate/compile/run epsilon NFA
// ===================================================================

// All reachable Node and self that are non-empty or done
Closure EpsilonClosure(Node * n0)
{
    static std::map<Node *, Closure> m;
    auto kv = m.find(n0);
    if (kv != m.end())
        return kv->second;

    Closure c;
    Closure mask;
    std::deque<Node *> q = {n0};
    while (!q.empty())
    {
        Node * n = q.back();
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

    return m.emplace_hint(kv, n0, c)->second;
}

struct ClosureLessThan
{
    bool operator () (const Closure & c1, const Closure & c2) const
    {
        size_t i = 0;
        while (i < c1.size() && c1.test(i) == c2.test(i))
            ++i;
        return i < c1.size() && !c1.test(i) && c2.test(i);
    }
};

Closure EpsilonClosure(Closure c)
{
    static std::map<Closure, Closure, ClosureLessThan> m;
    auto kv = m.find(c);
    if (kv != m.end())
        return kv->second;

    Closure c2;
    for (size_t i = 0; i < c.size(); ++i)
    {
        if (c[i])
        {
            c2 |= EpsilonClosure(NodeFactory[i]);
        }
    }

    return m.emplace_hint(kv, c, c2)->second;
}

Closure Next(Closure c, char ch)
{
    Closure c2;
    for (size_t i = 0; i < c.size(); ++i)
    {
        if (c[i])
        {
            Node * n = NodeFactory[i];
            assert(n->done || n->c);
            if (n->c == ch && n->next)
            {
                c2.set(n->next->id);
            }
        }
    }
    return c2;
}

enum class MatchState
{
    CONT,
    GOOD,
    BAD,
};
MatchState GetState(Closure c)
{
    if (c.none())
        return MatchState::BAD;

    for (size_t i = 0; i < c.size(); ++i)
    {
        if (c[i] && NodeFactory[i]->done)
            return MatchState::GOOD;
    }

    return MatchState::CONT;
}
MatchState GetStateN(Closure c)
{
    if (c.none())
        return MatchState::BAD;

    size_t i = 0;
    while (i < c.size() && !c.test(i))
        ++i;
    
    if (NodeFactory[i]->done)
        return MatchState::GOOD;
    else
        return MatchState::CONT;
}

void PrintNodes(Closure c)
{
    std::cout << "{";
    for (size_t i = 0; i < c.size(); ++i)
    {
        if (c.test(i))
            std::cout << " " << NodeFactory[i]->id;
    }
    std::cout << " }" << std::endl;;
}
void PrintNodes()
{
    for (Node * n : NodeFactory)
    {
        std::cout
            << n->id << (n->done ? "(done)" : (n->c ? "(non-empty)" : "")) << ":" << std::endl
            << "  --" << (n->c ? n->c : '-') << "--> " << (n->next ? std::to_string(n->next->id) : "null") << std::endl
            << "  -----> " << (n->alt ? std::to_string(n->alt->id) : "null") << std::endl;
    }
    for (Node * n : NodeFactory)
    {
        std::cout
            << n->id << (n->done ? "(done)" : "") << ": ";
        PrintNodes(EpsilonClosure(n));
    }
}

bool Run(NodeGroup c, std::string & text)
{
    std::cout << "match \"" << text << "\"..." << std::endl;
    Closure cl;
    cl.set(c.in->id);

    MatchState state = MatchState::CONT;
    size_t i = 0;
    for (;
         i < text.size() && state == MatchState::CONT;
         ++i)
    {
        char ch = text[i];
        cl = EpsilonClosure(cl);
        cl = Next(cl, ch);
        PrintNodes(cl);

        state = GetState(cl);
    }

    bool matched = false;
    if (state == MatchState::GOOD)
    {
        if (i == text.size())
            std::cout << "matched." << std::endl, matched = true;
        else
            std::cout << "prefix \"" << text.substr(0, i) << "\" matched." << std::endl;
    }
    else
    {
        std::cout << "not matched." << std::endl;
    }
    return matched;
}

struct TransitionTable
{
    std::vector<std::array<size_t, 26>> jump;
    std::vector<MatchState> state;
};
TransitionTable Compile(NodeGroup g)
{
    TransitionTable t;

    std::deque<Closure> q;
    std::map<Closure, size_t, ClosureLessThan> id;
    size_t next_id = 0;

    Closure c0;
    c0.set(g.in->id);
    c0 = EpsilonClosure(c0);
    q.push_front(c0);
    id.emplace(c0, next_id++);

    size_t row = 0;
    while (!q.empty())
    {
        if (t.jump.size() < row + 1)
            t.jump.resize(row + 1);

        Closure c1 = q.back();
        q.pop_back();
        for (char ch = 'a'; ch <= 'z'; ++ch)
        {
            Closure c2 = EpsilonClosure(Next(c1, ch));
            
            auto cid = id.find(c2);
            if (cid == id.end())
            {
                q.push_front(c2);
                cid = id.emplace_hint(cid, c2, next_id++);
            }

            t.jump[row][ch - 'a'] = cid->second;
        }

        ++row;
    }

    t.state.resize(row);
    for (auto cl_id : id)
    {
        t.state[cl_id.second] = GetState(cl_id.first);
    }

    return t;
}

TransitionTable CompileN(std::vector<NodeGroup> vg)
{
    TransitionTable t;

    std::deque<Closure> q;
    std::map<Closure, size_t, ClosureLessThan> id;
    size_t next_id = 0;

    Closure c0;
    for (NodeGroup & g : vg)
        c0.set(g.in->id);
    c0 = EpsilonClosure(c0);
    q.push_front(c0);
    id.emplace(c0, next_id++);

    size_t row = 0;
    while (!q.empty())
    {
        if (t.jump.size() < row + 1)
            t.jump.resize(row + 1);

        Closure c1 = q.back();
        q.pop_back();
        for (char ch = 'a'; ch <= 'z'; ++ch)
        {
            Closure c2 = EpsilonClosure(Next(c1, ch));

            auto cid = id.find(c2);
            if (cid == id.end())
            {
                q.push_front(c2);
                cid = id.emplace_hint(cid, c2, next_id++);
            }

            t.jump[row][ch - 'a'] = cid->second;
        }

        ++row;
    }

    t.state.resize(row);
    for (auto cl_id : id)
    {
        t.state[cl_id.second] = GetStateN(cl_id.first);
    }

    return t;
}

bool Run(TransitionTable & tt, std::string & text)
{
    std::cout << "match \"" << text << "\"..." << std::endl;
    size_t state = 0;
    size_t i = 0;
    for (;
         i < text.size() && tt.state[state] == MatchState::CONT;
         ++i)
    {
        char ch = text[i];
        state = tt.jump[state][ch - 'a'];
    }

    bool matched = false;
    if (tt.state[state] == MatchState::GOOD)
    {
        size_t matched_pattern = 0;
        for (size_t i = 0; i <= state; ++i)
        {
            if (tt.state[i] == MatchState::GOOD)
                ++matched_pattern;
        }

        if (i == text.size())
            std::cout << "matched pattern #" << matched_pattern << "." << std::endl, matched = true;
        else
            std::cout << "prefix \"" << text.substr(0, i) << "\" matched pattern #" << matched_pattern << "." << std::endl;
    }
    else
    {
        std::cout << "not matched." << std::endl;
    }
    return matched;
}

void PrintTable(TransitionTable & tt)
{
    std::cout << "    ";
    for (char ch = 'a'; ch <= 'z'; ++ch)
    {
        std::cout << "  " << ch;
    }
    std::cout << std::endl;

    size_t state = 0;
    for (auto row : tt.jump)
    {
        std::cout.width(4);
        std::cout << state++;
        for (char ch = 'a'; ch <= 'z'; ++ch)
        {
            std::cout << "  " << row[ch - 'a'];
        }
        std::cout << std::endl;
    }

    state = 0;
    for (auto row : tt.state)
    {
        std::cout << "[" << state++ << "] ";
        switch (row)
        {
            case MatchState::CONT: std::cout << "cont."; break;
            case MatchState::GOOD: std::cout << "good"; break;
            case MatchState::BAD: std::cout << "bad"; break;
            default: break;
        }
        std::cout << std::endl;
    }
}

// Convert 1 pattern list to transition table
void Test_ConvertOnePattern()
{
    // ab*c+d?e
    NodeGroup c = Done(
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
    PrintNodes();

    // Compile.
    TransitionTable tt = Compile(c);
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
        //assert(Run(c, text) == result);
        assert(Run(tt, text) == result);
    }
}

// Convert N pattern list to transition table
void Test_ConvertMultiplePattern()
{
    std::vector<NodeGroup> vc = {
        FromRegex("apple"),
        FromRegex("banan")
    };
    PrintNodes();

    // Compile.
    TransitionTable tt = CompileN(vc);
    PrintTable(tt);

    // Simulate.
    std::vector<std::pair<std::string, bool>> test_cases = {
        { "apple", true },
        { "banan", true },
        { "appla", false },
        { "banana", false },
    };
    for (auto kv : test_cases)
    {
        auto & text = kv.first;
        bool result = kv.second;
        std::cout << "pattern: (apple|banan) text: " << text << std::endl;
        assert(Run(tt, text) == result);
    }
}
