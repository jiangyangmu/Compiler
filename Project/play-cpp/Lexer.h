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

    Example:

        proc        => proc keyword
        [_a-zA-Z]+  => id
        [0-9]+      => num
        ELSE        => throw error

*/

typedef std::bitset<512> Closure;

struct Node
{
    size_t id;
    bool done;

    char * c;
    Node * next;
    Node * alt;
};

struct NodeGroup
{
    Node * in;
    Node ** next;
    Node ** alt;
};

class NodeFactory
{
public:
    Node * NewNode()
    {
        v.emplace_back(new Node);

        Node * n = v.back().get();
        n->id = v.size() - 1;
        n->done = false;
        n->c = nullptr;
        n->next = nullptr;
        n->alt = nullptr;
        
        return n;
    }
    Node * operator [] (size_t i) const
    {
        return v.at(i).get();
    }
    std::vector<std::unique_ptr<Node>> & nodes() { return v; }
private:
    std::vector<std::unique_ptr<Node>> v;
};

enum class MatchState
{
    CONT,
    GOOD,
    BAD,
};
struct TransitionTable
{
    std::vector<std::array<size_t, 27>> jump;
    std::vector<MatchState> state;
    std::vector<Closure> state_closure;
    std::vector<size_t> matched_branch;

    bool Run(std::string & text);
    size_t RunN(std::string & text);
};

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
struct CompileContext
{
    NodeFactory factory;

    std::vector<NodeGroup> groups;
    
    std::map<Node *, Closure> n2c_cache;
    std::map<Closure, Closure, ClosureLessThan> c2c_cache;

    Closure EpsilonClosure(Node * n0);
    Closure EpsilonClosure(Closure c);
    Closure Next(Closure c, char ch);
    std::pair<MatchState, size_t> GetState(Closure c);

    void PrintNodes();

    size_t Simulate(std::string & text);

    TransitionTable Compile();
};


std::string NodesString(Closure c)
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
void PrintClosure(Closure c)
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
    for (auto & n : factory.nodes())
    {
        std::cout
            << n->id << (n->done ? "(done)" : (n->c ? "(non-empty)" : "")) << ":" << std::endl
            << "  --" << (n->c ? *n->c : '-') << "--> " << (n->next ? std::to_string(n->next->id) : "null") << std::endl
            << "  -----> " << (n->alt ? std::to_string(n->alt->id) : "null") << std::endl;
    }
    for (auto & n : factory.nodes())
    {
        std::cout
            << n->id << (n->done ? "(done)" : "") << ": ";
        PrintClosure(EpsilonClosure(n.get()));
    }
}

// ===================================================================
// Build epsilon NFA
// ===================================================================

NodeFactory * gNodeFactory = nullptr;
struct NodeFactoryScope
{
    NodeFactory * origin;
public:
    NodeFactoryScope(NodeFactory * factory)
    {
        origin = gNodeFactory;
        gNodeFactory = factory;
    }
    ~NodeFactoryScope()
    {
        gNodeFactory = origin;
    }
};

NodeGroup MatchEmpty()
{
    Node * n = gNodeFactory->NewNode();
    return { n, &n->next, &n->alt };
}

NodeGroup MatchChar(char c)
{
    Node * n = gNodeFactory->NewNode();
    n->c = new char(c);

    return { n, &n->next, &n->alt };
}

NodeGroup Concat(NodeGroup c1, NodeGroup c2)
{
    *c1.next = c2.in;

    return { c1.in, c2.next , c1.alt };
}

NodeGroup Alt(NodeGroup c1, NodeGroup c2)
{
    Node * n = gNodeFactory->NewNode();
    *c1.next = n;
    *c1.alt = c2.in;
    *c2.next = n;

    return { c1.in, &n->next, c2.alt };
}

NodeGroup Rep0(NodeGroup c)
{
    Node * n = gNodeFactory->NewNode();
    n->alt = c.in;
    *c.next = n;

    return { n, &n->next, c.alt };
}

NodeGroup Rep1(NodeGroup c)
{
    Node * n = gNodeFactory->NewNode();
    n->alt = c.in;
    *c.next = n;

    return { c.in, &n->next, c.alt };
}

NodeGroup Opt(NodeGroup c)
{
    Node * n = gNodeFactory->NewNode();
    *c.next = n;
    *c.alt = n;

    return { c.in, &n->next, &n->alt };
}

NodeGroup Done(NodeGroup c)
{
    Node * n = gNodeFactory->NewNode();
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
Closure CompileContext::EpsilonClosure(Node * n0)
{
    auto kv = n2c_cache.find(n0);
    if (kv != n2c_cache.end())
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

    return n2c_cache.emplace_hint(kv, n0, c)->second;
}


Closure CompileContext::EpsilonClosure(Closure c)
{
    Closure c2;

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
                c2 |= EpsilonClosure(factory[i]);
            }
        }
        c2c_cache.emplace_hint(kv, c, c2);

        //std::cout << NodesString(c) << " -> " << NodesString(c2) << std::endl;
    }

    return c2;
}

Closure CompileContext::Next(Closure c, char ch)
{
    Closure c2;
    for (size_t i = 0; i < c.size(); ++i)
    {
        if (c[i])
        {
            Node * n = factory[i];
            assert(n->done || n->c);
            if (n->done)
                c2.set(n->id);
            else if (*n->c == ch && n->next)
                c2.set(n->next->id);
        }
    }
    return c2;
}

std::pair<MatchState, size_t> CompileContext::GetState(Closure c)
{
    if (c.none())
        return {MatchState::BAD, 0};

    size_t i = 0;
    while (i < c.size() && !c.test(i))
        ++i;
    
    if (factory[i]->done)
        return {MatchState::GOOD, i};
    else
        return {MatchState::CONT, 0};
}

size_t CompileContext::Simulate(std::string & text)
{
    std::cout << "match \"" << text << "\"..." << std::endl;
    Closure cl;
    for (NodeGroup & g : groups)
        cl.set(g.in->id);

    MatchState state = MatchState::CONT;
    size_t state_id = 0;
    size_t i = 0;
    for (;
         i < text.size() && state == MatchState::CONT;
         ++i)
    {
        char ch = text[i];
        cl = EpsilonClosure(cl);
        cl = Next(cl, ch);
        //PrintNodes(cl);

        std::tie(state, state_id) = GetState(cl);
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

TransitionTable CompileContext::Compile()
{
    TransitionTable t;

    std::deque<Closure> q;
    std::map<Closure, size_t, ClosureLessThan> id;
    size_t next_id = 0;

    Closure c0;
    for (NodeGroup & g : groups)
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
        {
            Closure c2 = EpsilonClosure(Next(c1, 0));

            auto cid = id.find(c2);
            if (cid == id.end())
            {
                q.push_front(c2);
                cid = id.emplace_hint(cid, c2, next_id++);
            }

            t.jump[row][26] = cid->second;
        }

        ++row;
    }

    t.state.resize(row);
    t.matched_branch.resize(row);
    t.state_closure.resize(row);

    // Convert Node id to branch number.
    for (auto cl_id : id)
    {
        auto state_and_done_node = GetState(cl_id.first);
        t.state[cl_id.second] = state_and_done_node.first;
        t.state_closure[cl_id.second] = cl_id.first;

        size_t done_node = state_and_done_node.second;
        size_t branch = 0;
        for (size_t i = 0; i <= done_node; ++i)
        {
            if (factory[i]->done)
                ++branch;
        }
        t.matched_branch[cl_id.second] = branch;
    }

    return t;
}

bool TransitionTable::Run(std::string & text)
{
    std::cout << "match \"" << text << "\"..." << std::endl;
    size_t st = 0;
    size_t i = 0;
    for (;
         i < text.size() && this->state[st] == MatchState::CONT;
         ++i)
    {
        char ch = text[i];
        st = this->jump[st][ch - 'a'];
    }
    for (;
         i < text.size() && this->state[st] == MatchState::GOOD;
         ++i)
    {
        char ch = text[i];
        size_t next_state = this->jump[st][ch - 'a'];
        if (this->state[next_state] != MatchState::GOOD)
        {
            --i;
            break;
        }
         st = next_state;
    }
    if (i == text.size())
    {
        size_t next_state = this->jump[st][26];
        st = this->jump[st][26];
    }

    bool matched = false;
    if (this->state[st] == MatchState::GOOD)
    {
        if (i == text.size())
            std::cout << "matched pattern ." << std::endl, matched = true;
        else
            std::cout << "prefix \"" << text.substr(0, i) << "\" matched." << std::endl;
    }
    else
    {
        std::cout << "not matched." << std::endl;
    }
    return matched;
}

size_t TransitionTable::RunN(std::string & text)
{
    std::cout << "match \"" << text << "\"..." << std::endl;
    size_t st = 0;
    size_t i = 0;
    for (;
         i < text.size() && this->state[st] == MatchState::CONT;
         ++i)
    {
        char ch = text[i];
        size_t next_state = this->jump[st][ch - 'a'];
        std::cout << st << NodesString(this->state_closure[st]) << " --" << ch << "-> " << next_state << NodesString(this->state_closure[next_state]) << std::endl;
        st = next_state;
    }
    for (;
         i < text.size() && this->state[st] == MatchState::GOOD;
         ++i)
    {
        char ch = text[i];
        size_t next_state = this->jump[st][ch - 'a'];
        if (this->state[next_state] != MatchState::GOOD)
        {
            --i;
            break;
        }
        std::cout << st << NodesString(this->state_closure[st]) << " --" << ch << "-> " << next_state << NodesString(this->state_closure[next_state]) << std::endl;
        st = next_state;
    }
    if (i == text.size())
    {
        size_t next_state = this->jump[st][26];
        std::cout << st << NodesString(this->state_closure[st]) << " -\\0-> " << next_state << NodesString(this->state_closure[next_state]) << std::endl;
        st = this->jump[st][26];
    }

    if (this->state[st] == MatchState::GOOD)
    {
        if (i == text.size())
            std::cout << "matched pattern #" << this->matched_branch[st] << "." << std::endl;
        else
            std::cout << "prefix \"" << text.substr(0, i) << "\" matched pattern #" << this->matched_branch[st] << "." << std::endl;
    }
    else
    {
        std::cout << "not matched." << std::endl;
    }
    return this->matched_branch[st];
}

void PrintTable(TransitionTable & tt)
{
    const int w = 3;

    std::cout << "  ";
    std::cout.width(w);
    std::cout << " ";
    for (char ch = 'a'; ch <= 'z'; ++ch)
    {
        std::cout.width(w);
        std::cout << ch;
    }
    std::cout.width(w);
    std::cout << "\\0";
    std::cout << std::endl;

    size_t state = 0;
    for (auto row : tt.jump)
    {
        std::cout << " [";
        std::cout.width(w - 1);
        std::cout << state++ << "]";
        for (char ch = 'a'; ch <= 'z'; ++ch)
        {
            std::cout.width(w);
            std::cout << row[ch - 'a'];
        }
        std::cout.width(w);
        std::cout << row[26];
        std::cout << std::endl;
    }

    state = 0;
    for (auto row : tt.state)
    {
        std::cout << " [" << state << "] ";
        switch (row)
        {
            case MatchState::CONT: std::cout << "cont."; break;
            case MatchState::GOOD: std::cout << "good #" << (!tt.matched_branch.empty() ? std::to_string(tt.matched_branch[state]) : "1"); break;
            case MatchState::BAD: std::cout << "bad"; break;
            default: break;
        }
        std::cout << "\t" << NodesString(tt.state_closure[state]) << std::endl;

        ++state;
    }
}

// Convert 1 pattern list to transition table
void Test_ConvertOnePattern()
{
    CompileContext context;
    NodeFactoryScope scope(&context.factory);

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
    context.groups.push_back(c);
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
        assert(tt.Run(text) == result);
    }
}

void Test_ConvertOnePattern2()
{
    CompileContext context;
    NodeFactoryScope scope(&context.factory);

    // (a|b)+
    NodeGroup c = MatchChar('a');
    c = Alt(c, MatchChar('b'));
    c = Rep1(c);
    c = Done(c);
    context.groups.push_back(c);
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
        { "baz", false },
    };
    for (auto kv : test_cases)
    {
        auto & text = kv.first;
        bool result = kv.second;
        std::cout << "pattern: (a|b)+ text: " << text << std::endl;
        assert(tt.Run(text) == result);
    }
}
/*
// Convert N pattern list to transition table
void Test_ConvertMultiplePattern()
{
    CompileContext context;
    NodeFactoryScope scope(&context.factory);

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

void Test_ConvertMultiplePattern2()
{
    CompileContext context;
    NodeFactoryScope scope(&context.factory);

    std::vector<NodeGroup> vc = {
        FromRegex("app"),
        FromRegex("ap"),
        FromRegex("apple"),
        FromRegex("a"),
        FromRegex("appl"),
    };
    PrintNodes();

    // Compile.
    TransitionTable tt = CompileN(vc);
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
        assert(RunN(tt, text) == which);
    }
}

void Test_ConvertMultiplePattern3()
{
    CompileContext context;
    NodeFactoryScope scope(&context.factory);

    std::vector<NodeGroup> vc = {
        FromRegex("abcde"),
        FromRegex("xy"),
        FromRegex("(a|b|c|d|e|f|g|h|i|j|k|l|m|n|o|p|q|r|s|t|u|v|w|x|y)+")
    };
    PrintNodes();

    // Compile.
    TransitionTable tt = CompileN(vc);
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
        assert(RunN(tt, text) == which);
    }
}

void Test_CLex()
{
    std::vector<NodeGroup> vc = {
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
    //PrintNodes();

    TransitionTable tt = CompileN(vc);
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
        assert(RunN(tt, text) == which);
    }
}
*/