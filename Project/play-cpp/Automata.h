#pragma once

#include <set>
#include <map>
#include <vector>
#include <exception>
#include <cassert>
#include <iostream>
#include <memory>
#include <locale>
#include <codecvt>
#include <string>
#include <functional>

std::wostream & operator<< (std::wostream & o, const std::wstring & s)
{
    return o << s.data();
}

std::wstring ToWString(const std::string & s)
{
    static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(s);
}

#define FOR_EACH_REV(elementName, container) for (auto it = (container).rbegin(); it != (container).rend() && ((elementName) = *it, true); ++it)

// ==== RE ====

// > forward decl

class Node;
class HeadNode;

class Concatenation;
class Alternation;
class Repeation;
class ASCIICharacter;
class UnicodeCharacter;
class ActionNode;
class Reference;

class RETree;

#define ACCEPT_VISIT_BASE virtual void Accept(REVisitor & visitor) = 0;
#define ACCEPT_VISIT(host) virtual void Accept(REVisitor & visitor) { visitor.Visit##host(*this); }
#define NOT_ACCEPT_VISIT virtual void Accept(REVisitor & visitor) {}
class REVisitor
{
public:
    virtual ~REVisitor() = default;

    // Operators
    virtual void VisitConcatenation(Concatenation &) {}
    virtual void VisitAlternation(Alternation &) {}
    virtual void VisitRepeation(Repeation &) {}

    // Elements
    virtual void VisitASCIICharacter(ASCIICharacter &) {}
    virtual void VisitUnicodeCharacter(UnicodeCharacter &) {}
    virtual void VisitReference(Reference &) {}

    // Action
    virtual void VisitActionNode(ActionNode &) {}
};

using REIndex = int;

// > repr

class Node
{
public:
    virtual ~Node() = default;
    virtual bool IsLeaf() const = 0;

    ACCEPT_VISIT_BASE
};
class LeafNode : public Node
{
public:
    bool IsLeaf() const override { return true; }
};
class InternalNode : public Node
{
public:
    bool IsLeaf() const override { return false; }

    const std::vector<Node *> & GetChildren() { return children; }

protected:
    void AddChild(Node * child) { children.push_back(child); }

    std::vector<Node *> children;
};

class Concatenation : public InternalNode
{
    friend class RE;

public:
    ACCEPT_VISIT(Concatenation)

private:
    Concatenation(Node * a0, Node * a1)
    {
        AddChild(a0);
        AddChild(a1);
    }
};
class Alternation : public InternalNode
{
    friend class RE;

public:
    ACCEPT_VISIT(Alternation)

private:
    Alternation(Node * a0, Node * a1)
    {
        AddChild(a0);
        AddChild(a1);
    }
};
class Repeation : public InternalNode
{
    friend class RE;

public:
    enum Type { ZERO_OR_MORE, ONE_OR_MORE, ZERO_OR_ONE };

    Type GetType() const { return type; }

public:
    ACCEPT_VISIT(Repeation)

private:
    Repeation(Node * a0, Type a1) : type(a1)
    {
        AddChild(a0);
    }

    Type type;
};

class Character : public LeafNode {};
class ASCIICharacter : public Character
{
    friend class RE;

public:
    char GetValue() const { return value; }

public:
    ACCEPT_VISIT(ASCIICharacter)

private:
    explicit ASCIICharacter(char c) : value(c) {}

    char value;
};
class UnicodeCharacter : public Character
{
    friend class RE;

public:
    wchar_t GetValue() const { return value; }

public:
    ACCEPT_VISIT(UnicodeCharacter)

private:
    explicit UnicodeCharacter(wchar_t c) : value(c) {}

    wchar_t value;
};
class Reference : public LeafNode
{
    friend class RE;

public:
    Reference & SetTag(const char * a0) { tag = a0; return *this; }
    const char * GetTag() const { return tag; }

    REIndex Get() { return index; }

public:
    ACCEPT_VISIT(Reference)

private:
    explicit Reference(REIndex a0, const char * a1) : index(a0), tag(a1) {}

    REIndex index;
    const char * tag;
};

class ActionNode : public LeafNode
{
    friend class RE;

public:
    std::function<void()> & GetAction() { return action; }

public:
    ACCEPT_VISIT(ActionNode)

private:
    explicit ActionNode(std::function<void()> a0) : action(std::move(a0)) {}

    std::function<void()> action;
};

// > build

class REExpression;

class REContext
{
    friend class RE;

public:
    REContext();
    ~REContext();

    Node * FindTree(REIndex index);
    void AddNode(Node * node);
    void AddTree(REIndex index, Node * tree);

private:
    // RE node allocator
    std::vector<Node *> nodes;
    // RE index allocator
    REIndex indexGen;
    // RE tree index -> RE tree
    std::map<REIndex, Node *> trees;
};
REContext::REContext()
    : indexGen(0)
{
}
REContext::~REContext()
{
    for (Node * node : nodes)
    {
        delete node;
    }
}
Node * REContext::FindTree(REIndex index)
{
    return trees.at(index);
}
void REContext::AddNode(Node * node)
{
    nodes.push_back(node);
}
void REContext::AddTree(REIndex index, Node * tree)
{
    trees.emplace(index, tree);
}
class REScopedContext
{
public:
    REScopedContext();
    ~REScopedContext();

private:
    REContext scopedContext;
    REContext * oldContext;
};
class RE
{
    friend class REScopedContext;

public:
    static REIndex NewIndex();

    static Node * Concat(Node * a0, Node * a1);
    static Node * Alter(Node * a0, Node * a1);
    static Node * Repeat(Node * a0, Repeation::Type a1);
    static Node * Ascii(char c);
    static Node * Unicode(wchar_t c);
    static Node * Ref(REIndex a0, const char * a1);

    static void Visit(RETree & tree, REVisitor & visitor);

private:
    static REContext * CurrentContext;
};
REContext * RE::CurrentContext = nullptr;
REIndex RE::NewIndex()
{
    return CurrentContext->indexGen++;
}
Node * RE::Concat(Node * a0, Node * a1)
{
    Node * n = new Concatenation(a0, a1);
    CurrentContext->AddNode(n);
    return n;
}
Node * RE::Alter(Node * a0, Node * a1)
{
    Node * n = new Alternation(a0, a1);
    CurrentContext->AddNode(n);
    return n;
}
Node * RE::Repeat(Node * a0, Repeation::Type a1)
{
    Node * n = new Repeation(a0, a1);
    CurrentContext->AddNode(n);
    return n;
}
Node * RE::Ascii(char c)
{
    Node * n = new ASCIICharacter(c);
    CurrentContext->AddNode(n);
    return n;
}
Node * RE::Unicode(wchar_t c)
{
    Node * n = new UnicodeCharacter(c);
    CurrentContext->AddNode(n);
    return n;
}
Node * RE::Ref(REIndex a0, const char * a1)
{
    Node * n = new Reference(a0, a1);
    CurrentContext->AddNode(n);
    return n;
}
REScopedContext::REScopedContext()
{
    oldContext = RE::CurrentContext;
    RE::CurrentContext = &scopedContext;
}
REScopedContext::~REScopedContext()
{
    RE::CurrentContext = oldContext;
}

class RETree
{
public:
    RETree() : index(RE::NewIndex()) {}

    REIndex Index() const { return index; }
    Node * Tree() { return tree; }
    void SetTree(Node * a0) { tree = a0; }

    RETree & operator= (REExpression && build);

    // bool operator== (const RETree & other) const { return index == other.index; }
    // bool operator!= (const RETree & other) const { return index != other.index; }

private:
    REIndex index;
    Node * tree;
};
class REExpression
{
public:
    REExpression() = default;

    REExpression operator& (REExpression);
    REExpression operator| (REExpression);
    REExpression operator~ ();
    REExpression operator* ();
    REExpression operator+ ();
    REExpression (RETree origin);
    friend REExpression Ascii(char c);
    friend REExpression Unicode(wchar_t c);
    friend REExpression Action(std::function<void()> action);

    friend RETree & RETree::operator= (REExpression && build);;

private:
    Node * tree;
};
REExpression REExpression::operator& (REExpression other)
{
    REExpression re;
    re.tree = RE::Concat(tree, other.tree);
    return re;
}
REExpression REExpression::operator| (REExpression other)
{
    REExpression re;
    re.tree = RE::Alter(tree, other.tree);
    return re;
}
REExpression REExpression::operator~ ()
{
    REExpression re;
    re.tree = RE::Repeat(tree, Repeation::ZERO_OR_ONE);
    return re;
}
REExpression REExpression::operator* ()
{
    REExpression re;
    re.tree = RE::Repeat(tree, Repeation::ZERO_OR_MORE);
    return re;
}
REExpression REExpression::operator+ ()
{
    REExpression re;
    re.tree = RE::Repeat(tree, Repeation::ONE_OR_MORE);
    return re;
}
REExpression::REExpression(RETree origin)
{
    tree = RE::Ref(origin.Index(), "");
}
#define REF(x) REExpression(x)
REExpression Ascii(char c)
{
    REExpression re;
    re.tree = RE::Ascii(c);
    return re;
}
REExpression Unicode(wchar_t c)
{
    REExpression re;
    re.tree = RE::Unicode(c);
    return re;
}
RETree & RETree::operator= (REExpression && build)
{
    tree = build.tree;
    return *this;
}

// > visit

void RE::Visit(RETree & tree, REVisitor & visitor)
{
    Node * body = tree.Tree();

    assert(body);
    std::vector<Node *> nodeStack = { body };
    std::vector<int> seenStack = { 0 };
    while (!nodeStack.empty())
    {
        Node * node = nodeStack.back();
        if (node->IsLeaf())
        {
            node->Accept(visitor);
        }
        else
        {
            InternalNode * branch = dynamic_cast<InternalNode *>(node);
            int & seen = seenStack.back();
            if (!seen)
            {
                seen = 1;
                Node * child;
                FOR_EACH_REV(child, branch->GetChildren())
                {
                    nodeStack.push_back(child);
                    seenStack.push_back(0);
                }

                continue;
            }

            branch->Accept(visitor);
        }
        nodeStack.pop_back();
        seenStack.pop_back();
    }
}

// ==== NFA ====

// > repr

struct NfaValue
{
    virtual ~NfaValue() = default;

    virtual std::string ToString() { return ""; }
};

struct NfaState
{
    // id
    NfaValue * value;
    NfaState * valueEdge;
    NfaState * epsilonEdge;
    NfaState * refEdge;

public:
    NfaState() : value(nullptr), valueEdge(nullptr), epsilonEdge(nullptr), refEdge(nullptr) {}
    NfaState(NfaValue * a0) : value(a0), valueEdge(nullptr), epsilonEdge(nullptr), refEdge(nullptr) {}
    virtual ~NfaState() = default;
};

struct NfaHead : public NfaState
{
public:
    NfaState * GetBody() { return epsilonEdge; }
    void SetBody(NfaState * a0) { epsilonEdge = a0; }
};
struct NfaAction : public NfaState
{
public:
    std::function<void()> & GetAction() { return action; }
    void SetAction(std::function<void()> a0) { action = std::move(a0); }
private:
    std::function<void()> action;
};

struct Nfa_Build
{
    NfaState * in;
    NfaState ** next;
    NfaState ** alt;
};

class Nfa_Ref
{
public:
    Nfa_Ref() : head(new NfaHead) {}

    NfaHead * Dereference() { return head.get(); }

    void operator= (Nfa_Build && build) { head->SetBody(build.in); }

    bool operator== (const Nfa_Ref & other) const { return head == other.head; }
    bool operator!= (const Nfa_Ref & other) const { return head != other.head; }

private:
    std::shared_ptr<NfaHead> head;
};

struct Term : public NfaValue // Token
{
    explicit Term(char c) : value(c) {}

    virtual std::string ToString() override { return std::string(1, value); }

    char value;
};
struct NonTerm : public NfaValue // NfaReference
{
    explicit NonTerm(Nfa_Ref a0, const char * a1) : origin(a0), tag(a1) {}
    NonTerm & SetTag(const char * a0) { tag = a0; return *this; }
    const char * GetTag() const { return tag; }

    virtual std::string ToString() override { return std::string(tag); }

    Nfa_Ref origin;
    const char * tag;
};

// > build

Nfa_Build MatchValue(NfaValue * value)
{
    NfaState * n = new NfaState(value);

    return { n, &n->valueEdge, &n->epsilonEdge };
}
Nfa_Build MatchRef(NfaValue * ref, NfaState * refNfa)
{
    NfaState * n = new NfaState(ref);
    n->refEdge = refNfa;

    return { n, &n->valueEdge, &n->epsilonEdge };
}
Nfa_Build DoAction(std::function<void()> action)
{
    NfaAction * n = new NfaAction();
    n->SetAction(action);

    return { n, &n->valueEdge, &n->epsilonEdge };
}
Nfa_Build Concat(Nfa_Build c1, Nfa_Build c2)
{
    *c1.next = c2.in;

    return { c1.in, c2.next , c1.alt };
}
Nfa_Build Alt(Nfa_Build c1, Nfa_Build c2)
{
    NfaState * n = new NfaState;
    *c1.next = n;
    *c1.alt = c2.in;
    *c2.next = n;

    return { c1.in, &n->valueEdge, c2.alt };
}
Nfa_Build Rep0(Nfa_Build c)
{
    NfaState * n = new NfaState;
    n->epsilonEdge = c.in;
    *c.next = n;

    return { n, &n->valueEdge, c.alt };
}
Nfa_Build Rep1(Nfa_Build c)
{
    NfaState * n = new NfaState;
    n->epsilonEdge = c.in;
    *c.next = n;

    return { c.in, &n->valueEdge, c.alt };
}
Nfa_Build Opt(Nfa_Build c)
{
    NfaState * n = new NfaState;
    *c.next = n;
    *c.alt = n;

    return { c.in, &n->valueEdge, &n->epsilonEdge };
}
Nfa_Build End(Nfa_Build c)
{
    NfaState * n = new NfaState;
    *c.next = n;

    return { c.in, &n->valueEdge, c.alt };
}

// > visit

class NfaVisitor
{
public:
    virtual ~NfaVisitor() = default;
    virtual void VisitNfaState(NfaState * state) {}
};

void Visit(NfaState * nfa, NfaVisitor & visitor);

void Visit(Nfa_Ref nfa, NfaVisitor & visitor)
{
    Visit(nfa.Dereference(), visitor);
}
void Visit(Nfa_Build nfa, NfaVisitor & visitor)
{
    Visit(nfa.in, visitor);
}
void Visit(NfaState * nfa, NfaVisitor & visitor)
{
    std::vector<NfaState *> toVisit = { nfa };
    std::set<NfaState *> visited;

    while (!toVisit.empty())
    {
        NfaState * ns = toVisit.back();
        toVisit.pop_back();

        if (visited.count(ns) != 0)
            continue;
        visited.insert(ns);

        visitor.VisitNfaState(ns);
        if (ns->refEdge) toVisit.push_back(ns->refEdge);
        if (ns->epsilonEdge) toVisit.push_back(ns->epsilonEdge);
        if (ns->valueEdge) toVisit.push_back(ns->valueEdge);
    }
}

// > run

void Run()
{
}

// ==== DFA ====

// ==== CF ====

// > repr

using CFRule = RETree;
using CFIndex = REIndex;
class CFGrammar
{
public:
    CFRule DeclareRule() { rules.emplace_back(); return rules.back(); }

    const std::vector<CFRule> & GetAllRules() const { return rules; }

    CFRule & StartRule() { return rules.front(); }

    CFRule & FindRule(CFIndex index) { assert(false); return rules.front(); }

private:
    std::vector<CFRule> rules;
    REContext context;
};

// ==== CONVERT ====

// > RE -> NFA
class RE2NFAConverter : public REVisitor
{
private:
    virtual void VisitConcatenation(Concatenation &) override
    {
        Nfa_Build n2 = stackNfa.back(); stackNfa.pop_back();
        Nfa_Build n1 = stackNfa.back(); stackNfa.pop_back();
        stackNfa.push_back(Concat(n1, n2));
    }
    virtual void VisitAlternation(Alternation &) override
    {
        Nfa_Build n2 = stackNfa.back(); stackNfa.pop_back();
        Nfa_Build n1 = stackNfa.back(); stackNfa.pop_back();
        stackNfa.push_back(Alt(n1, n2));
    }
    virtual void VisitRepeation(Repeation & r) override
    {
        Nfa_Build n = stackNfa.back(); stackNfa.pop_back();
        switch (r.GetType())
        {
            case Repeation::ZERO_OR_MORE:
                stackNfa.push_back(Rep0(n));
                break;
            case Repeation::ONE_OR_MORE:
                stackNfa.push_back(Rep1(n));
                break;
            case Repeation::ZERO_OR_ONE:
                stackNfa.push_back(Opt(n));
                break;
            default:
                break;
        }
    }
    virtual void VisitASCIICharacter(ASCIICharacter & c) override
    {
        stackNfa.push_back(MatchValue(new Term(c.GetValue())));
    }
    virtual void VisitReference(Reference & r) override
    {
        if (reRef2NfaRef)
            stackNfa.push_back(MatchRef(
                new NonTerm(reRef2NfaRef->at(r.Get()), r.GetTag()),
                reRef2NfaRef->at(r.Get()).Dereference()
            ));
        else
            assert(false);
    }
    virtual void VisitActionNode(ActionNode & a) override
    {
        stackNfa.push_back(DoAction(a.GetAction()));
    }

public:
    RE2NFAConverter(std::map<REIndex, Nfa_Ref> * a0 = nullptr) : reRef2NfaRef(a0) {}
    
    Nfa_Build Get()
    {
        assert(stackNfa.size() == 1);
        return End(stackNfa.back());
    }

private:
    std::vector<Nfa_Build> stackNfa;
    std::map<REIndex, Nfa_Ref> * reRef2NfaRef;
};

// > CF -> NFA with ref
Nfa_Ref ConvertCF2NFA(CFGrammar & grammar)
{
    std::map<REIndex, Nfa_Ref> reRef2NfaRef;

    for (const CFRule & rule : grammar.GetAllRules())
    {
        reRef2NfaRef.emplace(rule.Index(), Nfa_Ref());
    }

    for (auto & p : reRef2NfaRef)
    {
        REIndex reRef = p.first;
        Nfa_Ref & nfaRef = p.second;

        RE2NFAConverter conv(&reRef2NfaRef);
        RE::Visit(grammar.FindRule(reRef), conv);

        nfaRef = conv.Get();
    }

    return reRef2NfaRef.at(grammar.StartRule().Index());
}

// > RE -> String
class REStringConverter : public REVisitor
{
public:
    virtual void VisitConcatenation(Concatenation &) override
    {
        std::wstring s;
        std::swap(stackStrRE.back(), s);
        stackStrRE.pop_back();

        stackStrRE.back().append(s);
        stackStrRE.back().insert(0, L"(").append(L")");
    }
    virtual void VisitAlternation(Alternation &) override
    {
        std::wstring s;
        std::swap(stackStrRE.back(), s);
        stackStrRE.pop_back();

        stackStrRE.back().append(L"|").append(s);
        stackStrRE.back().insert(0, L"(").append(L")");
    }
    virtual void VisitRepeation(Repeation & r) override
    {
        stackStrRE.back().insert(0, L"(").append(L")");
        switch (r.GetType())
        {
            case Repeation::ZERO_OR_MORE:
                stackStrRE.back().append(L"*");
                break;
            case Repeation::ONE_OR_MORE:
                stackStrRE.back().append(L"+");
                break;
            case Repeation::ZERO_OR_ONE:
                stackStrRE.back().append(L"?");
                break;
            default:
                break;
        }
    }
    virtual void VisitASCIICharacter(ASCIICharacter & c) override
    {
        std::wstring s;
        s.push_back(c.GetValue());
        stackStrRE.push_back(s);
    }
    virtual void VisitUnicodeCharacter(UnicodeCharacter & c) override
    {
        std::wstring s;
        s.push_back(c.GetValue());
        stackStrRE.push_back(s);
    }
    virtual void VisitReference(Reference & r) override
    {
        std::wstring s;
        s.push_back(L'<');
        s.append(std::to_wstring(r.Get()));
        s.push_back(L'>');
        stackStrRE.push_back(s);
    }

    std::wstring Get()
    {
        assert(stackStrRE.size() == 1);
        return stackStrRE.back();
    }
    void Clear()
    {
        stackStrRE.clear();
    }

    std::vector<std::wstring> stackStrRE;
};

// ==== Debug ====

void PrintRE(RETree re)
{
    REStringConverter converter;
    std::wcout << (RE::Visit(re, converter), converter.Get()) << std::endl;
}
#define PrintNamedRE(re) (std::wcout << L#re L"<" << std::to_wstring(re.Index()) << "> = ", PrintRE(re))
void PrintCF(CFGrammar & grammar)
{
    for (CFRule rule : grammar.GetAllRules())
    {
        std::wcout << rule.Index() << L" = ", PrintRE(rule);
    }
}
#include "Graphviz.h"
void ShowNFA(NfaState * nfa, std::string label = "");
void ShowNFA(Nfa_Ref nfa, std::string label = "")
{
    ShowNFA(nfa.Dereference(), label);
}
void ShowNFA(Nfa_Build nfa, std::string label = "")
{
    ShowNFA(nfa.in, label);
}
void ShowNFA(NfaState * nfa, std::string label)
{
    class Nfa2DotConverter : public NfaVisitor
    {
    public:
        std::string Get(std::string label = "") {
            return
                "digraph {\n" +
                (label.empty() ? "" : "label=\"" + label + "\"\n") +
                dotProgram +
                "}\n";
        }

    private:
        virtual void VisitNfaState(NfaState * state) override
        {
            AddNfaState(state);
            if (state->epsilonEdge) AddNfaState(state->epsilonEdge);
            if (state->valueEdge) AddNfaState(state->valueEdge);
            if (state->refEdge) AddNfaState(state->refEdge);

            std::string from_state = std::to_string(nfaToId.at(state));

            if (state->epsilonEdge)
            {
                std::string epsilon_to_state = std::to_string(nfaToId.at(state->epsilonEdge));

                dotProgram +=
                    "\"" + from_state + "\"" +
                    " -> \"" + epsilon_to_state + "\"" +
                    "\n";
            }
            if (state->refEdge)
            {
                std::string ref_to_state = std::to_string(nfaToId.at(state->refEdge));
                dotProgram +=
                    "\"" + from_state + "\"" +
                    " -> \"" + ref_to_state + "\"" +
                    " [label=\"ref\"]" +
                    "\n";
            }
            if (state->valueEdge)
            {
                std::string value_to_state = std::to_string(nfaToId.at(state->valueEdge));

                dotProgram +=
                    "\"" + from_state + "\"" +
                    " -> \"" + value_to_state + "\"" +
                    " [label=\"" + (state->value ? state->value->ToString() : "") + "\"]" +
                    "\n";
            }
            if (!state->epsilonEdge && !state->valueEdge)
            {
                dotProgram +=
                    "\"" + from_state + "\"" +
                    " -> \"end\"" +
                    " [label=\"" + (state->value ? state->value->ToString() : "") + "\"]" +
                    "\n";
            }
        }

        void AddNfaState(NfaState * state)
        {
            if (nfaToId.try_emplace(state, id).second)
                ++id;
        }

        std::string dotProgram;
        std::map<NfaState *, int> nfaToId;
        int id = 0;
    };

    // NFA -> dot format
    Nfa2DotConverter converter;
    Visit(nfa, converter);

    static int graph_id = 0;
    DrawGraph(L"nfa" + std::to_wstring(graph_id++), converter.Get(label));
}

// ==== Test ====

void Test_REExpression()
{
    REScopedContext context;

#define TEST_CASE(expr) PrintRE(RETree() = (expr))

    TEST_CASE( Ascii('a')                                           );
    TEST_CASE( Ascii('a') & Ascii('b') | Ascii('c') & Ascii('d')    );
    TEST_CASE( +Ascii('a') & *Ascii('b') & ~Ascii('c')              );

#undef TEST_CASE
}

void Test_RETree()
{
    REScopedContext context;

    RETree S, A, B, C;

    S = REF(A) & REF(B) & REF(C);
    A = Ascii('a');
    B = Ascii('b');
    C = Ascii('c');

    PrintNamedRE(S);
    PrintNamedRE(A);
    PrintNamedRE(B);
    PrintNamedRE(C);
}

void Test_Conv_RE_NFA()
{
    std::pair<RETree, std::string> cases[] = {
#define TEST_CASE(re) {RETree() = re, #re}

    TEST_CASE( Ascii('a') & Ascii('b') & Ascii('c')                 ),
    TEST_CASE( Ascii('a') | Ascii('b') | Ascii('c')                 ),
    TEST_CASE( +Ascii('a')                                          ),
    TEST_CASE( *Ascii('a')                                          ),
    TEST_CASE( ~Ascii('a')                                          ),

#undef TEST_CASE
    };
    for (auto & p : cases)
    {
        REScopedContext context;

        RETree & re = p.first;
        std::string & label = p.second;

        RE2NFAConverter re2nfa;
        RE::Visit(re, re2nfa);

        Nfa_Build nfa = re2nfa.Get();

        ShowNFA(nfa, label);
    }
}

void Test_CF()
{
    CFGrammar grammar;

    CFRule S = grammar.DeclareRule();
    CFRule A = grammar.DeclareRule();
    CFRule B = grammar.DeclareRule();
    CFRule C = grammar.DeclareRule();

    S = REF(A) & REF(B) & REF(C);
    A = Ascii('a');
    B = Ascii('b');
    C = Ascii('c');

    PrintCF(grammar);
}

void Test_Conv_CF_NFAWithRef()
{
    {
        CFGrammar grammar;

        CFRule S = grammar.DeclareRule();
        CFRule A = grammar.DeclareRule();
        CFRule B = grammar.DeclareRule();
        CFRule C = grammar.DeclareRule();

        S = REF(A) & REF(B) & REF(C);
        A = Ascii('a');
        B = Ascii('b');
        C = Ascii('c');

        PrintCF(grammar);

        Nfa_Ref nfa = ConvertCF2NFA(grammar);

        ShowNFA(nfa, "A&B&C");
    }

    {
        CFGrammar grammar;

        CFRule S = grammar.DeclareRule();
        CFRule A = grammar.DeclareRule();
        CFRule B = grammar.DeclareRule();
        CFRule C = grammar.DeclareRule();

        S = REF(A) | REF(B) | REF(C);
        A = Ascii('a');
        B = Ascii('b');
        C = Ascii('c');

        PrintCF(grammar);

        Nfa_Ref nfa = ConvertCF2NFA(grammar);

        ShowNFA(nfa, "A|B|C");
    }

    {
        CFGrammar grammar;

        CFRule S = grammar.DeclareRule();
        CFRule A = grammar.DeclareRule();

        S = +REF(A);
        A = Ascii('a');

        PrintCF(grammar);

        Nfa_Ref nfa = ConvertCF2NFA(grammar);

        ShowNFA(nfa, "+A");
    }

    {
        CFGrammar grammar;

        CFRule S = grammar.DeclareRule();
        CFRule A = grammar.DeclareRule();

        S = *REF(A);
        A = Ascii('a');

        PrintCF(grammar);

        Nfa_Ref nfa = ConvertCF2NFA(grammar);

        ShowNFA(nfa, "*A");
    }

    {
        CFGrammar grammar;

        CFRule S = grammar.DeclareRule();
        CFRule A = grammar.DeclareRule();

        S = ~REF(A);
        A = Ascii('a');

        PrintCF(grammar);

        Nfa_Ref nfa = ConvertCF2NFA(grammar);

        ShowNFA(nfa, "~A");
    }
}

void Test_Automata()
{
    Test_REExpression();
    Test_RETree();
    // Test_NFA_Basic
    // Test_Conv_RE_NFA();

    // Test_RE_WithRef();
    // Test_NFA_WithRef
    // Test_CF();
    // Test_Conv_CF_NFAWithRef();


    // TODO: DFA
}
