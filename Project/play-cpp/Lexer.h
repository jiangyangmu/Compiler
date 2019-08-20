#pragma once

#include <cassert>
#include <string>
#include <vector>
#include <bitset>
#include <map>
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

typedef std::vector<int> TransitionTable;
typedef std::bitset<8> Closure;

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

// All reachable Node with non-empty next (including non-empty self)
Closure EpsilonClosure(Node * n0)
{
    static std::map<Node *, Closure> m;
    auto kv = m.find(n0);
    if (kv != m.end())
        return kv->second;

    Closure c;
    Closure mask;
    if (n0->c)
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

TransitionTable Compile(NodeGroup c);

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

// Convert 1 pattern list to transition table
void Convert1()
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

    // Simulate.
    std::vector<std::string> input = {
        "ac",
        "abc",
        "abbc",
        "abbbc",
    };
    for (auto text : input)
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
}

// Convert N pattern list to transition table
