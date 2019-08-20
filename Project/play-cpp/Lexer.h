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

// 1. build dfa
NodeGroup Match(char c)
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

// All reachable Node with non-empty next, and non-empty or done self
Closure EpsilonClosure(Node * n0)
{
    static std::map<Node *, Closure> m;
    auto kv = m.find(n0);
    if (kv != m.end())
        return kv->second;

    Closure c;
    Closure mask;
    if (n0->c || n0->done)
    {
        c.set(n0->id);
    }
    std::deque<Node *> q = {n0};
    while (!q.empty())
    {
        Node * n = q.back();
        q.pop_back();

        if (mask.test(n->id))
            continue;
        mask.set(n->id);

        if (!n->c && n->next)
        {
            assert(n->next != n);
            c.set(n->next->id);
            q.push_front(n->next);
        }
        if (n->alt)
        {
            assert(n->alt != n);
            c.set(n->alt->id);
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
            << n->id << (n->done ? "(done)" : "") << ":" << std::endl
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

void Run(NodeGroup c, std::string & text)
{
    std::cout << "match \"" << text << "\"..." << std::endl;
    Closure cl;
    cl.set(c.in->id);
    for (char ch : text)
    {
        cl = Next(EpsilonClosure(cl), ch);
        PrintNodes(cl);

        MatchState state = GetState(cl);
        if (state == MatchState::GOOD)
        {
            std::cout << "matched." << std::endl;
            break;
        }
        else if (state == MatchState::BAD)
        {
            std::cout << "not matched." << std::endl;
            break;
        }
    }
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
void Run(TransitionTable & tt, std::string & text)
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

    if (tt.state[state] == MatchState::GOOD)
    {
        if (i == text.size())
            std::cout << "matched." << std::endl;
        else
            std::cout << "prefix \"" << text.substr(0, i) << "\" matched." << std::endl;
    }
    else
    {
        std::cout << "not matched." << std::endl;
    }
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
    // ab*c
    NodeGroup c = Done(
        Concat(
            Concat(
                Match('a'),
                Rep0(
                    Match('b')
                )
            ),
            Match('c')
        )
    );
    PrintNodes();

    // Compile.
    TransitionTable tt = Compile(c);
    PrintTable(tt);

    // Simulate.
    std::vector<std::string> input = {
        "ac",
        "abc",
        "abbc",
        "abbbc",
    };
    for (auto text : input)
    {
        std::cout << "pattern: ab*c text: " << text << std::endl;
        //Run(c, text);
        Run(tt, text);
    }
}

// Convert N pattern list to transition table
