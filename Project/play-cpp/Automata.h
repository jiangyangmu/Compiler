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
// tree
// 1. data structure
//    * parent points to children (better for human)
//    * postfix nodes (better for computer)
// 2. build
//    * class/namespace + static methods
// 3. visit
//    * class member/memfunc access

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

class RE_Ref;

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

// > repr

class Node
{
public:
    virtual ~Node() = default;
    virtual bool IsLeaf() const = 0;

    ACCEPT_VISIT_BASE
};
class HeadNode : public Node
{
public:
    HeadNode() : body(nullptr) {}

    bool IsLeaf() const override { return false; }

    Node * GetBody() { return body; }
    void SetBody(Node * a0) { body = a0; }

public:
    NOT_ACCEPT_VISIT

private:
    Node * body;
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
public:
    Concatenation(Node * a0, Node * a1)
    {
        AddChild(a0);
        AddChild(a1);
    }

public:
    ACCEPT_VISIT(Concatenation)

private:
};
class Alternation : public InternalNode
{
public:
    Alternation(Node * a0, Node * a1)
    {
        AddChild(a0);
        AddChild(a1);
    }

public:
    ACCEPT_VISIT(Alternation)

private:
};
class Repeation : public InternalNode
{
public:
    enum Type { ZERO_OR_MORE, ONE_OR_MORE, ZERO_OR_ONE };
    Repeation(Node * a0, Type a1) : type(a1)
    {
        AddChild(a0);
    }

    Type GetType() const { return type; }

public:
    ACCEPT_VISIT(Repeation)

private:
    Type type;
};

class Character : public LeafNode {};
class ASCIICharacter : public Character
{
public:
    explicit ASCIICharacter(char c) : value(c) {}

    char GetValue() const { return value; }

public:
    ACCEPT_VISIT(ASCIICharacter)

private:
    char value;
};
class UnicodeCharacter : public Character
{
public:
    explicit UnicodeCharacter(wchar_t c) : value(c) {}

    wchar_t GetValue() const { return value; }

public:
    ACCEPT_VISIT(UnicodeCharacter)

private:
    wchar_t value;
};

class ActionNode : public LeafNode
{
public:
    explicit ActionNode(std::function<void()> a0) : action(std::move(a0)) {}

    std::function<void()> & GetAction() { return action; }

public:
    ACCEPT_VISIT(ActionNode)

private:
    std::function<void()> action;
};

class RE_Build;

class RE_Ref
{
public:
    RE_Ref() : reHead(new HeadNode()) {}

    HeadNode * Dereference() { return reHead.get(); }

    void operator= (RE_Build && build);

    bool operator== (const RE_Ref & other) const { return reHead == other.reHead; }
    bool operator!= (const RE_Ref & other) const { return reHead != other.reHead; }

    bool operator< (const RE_Ref & other) const { return reHead.get() < other.reHead.get(); }

private:
    std::shared_ptr<HeadNode> reHead;
};

class Reference : public LeafNode
{
public:
    explicit Reference(RE_Ref a0, const char * a1) : re(a0), tag(a1) {}
    Reference & SetTag(const char * a0) { tag = a0; return *this; }
    const char * GetTag() const { return tag; }

    RE_Ref Get() { return re; }

public:
    ACCEPT_VISIT(Reference)

private:
    RE_Ref re;
    const char * tag;
};

// > build

class RE_Build
{
public:
    RE_Build operator& (RE_Build);
    RE_Build operator| (RE_Build);
    RE_Build operator~ ();
    RE_Build operator* ();
    RE_Build operator+ ();
    friend RE_Build Ref(RE_Ref origin, const char * tag);
    friend RE_Build Ascii(char c);
    friend RE_Build Unicode(wchar_t c);
    friend RE_Build Action(std::function<void()> action);

    friend void RE_Ref::operator= (RE_Build && build);;
    friend void Visit(RE_Build & re, REVisitor & visitor);

private:
    Node * reBody;
};
// Operator nodes: operator based
RE_Build RE_Build::operator& (RE_Build other)
{
    RE_Build re;
    re.reBody = new Concatenation(reBody, other.reBody);
    return re;
}
RE_Build RE_Build::operator| (RE_Build other)
{
    RE_Build re;
    re.reBody = new Alternation(reBody, other.reBody);
    return re;
}
RE_Build RE_Build::operator~ ()
{
    RE_Build re;
    re.reBody = new Repeation(reBody, Repeation::ZERO_OR_ONE);
    return re;
}
RE_Build RE_Build::operator* ()
{
    RE_Build re;
    re.reBody = new Repeation(reBody, Repeation::ZERO_OR_MORE);
    return re;
}
RE_Build RE_Build::operator+ ()
{
    RE_Build re;
    re.reBody = new Repeation(reBody, Repeation::ONE_OR_MORE);
    return re;
}
// Element nodes: function based
RE_Build Ref(RE_Ref origin, const char * tag = "")
{
    RE_Build re;
    re.reBody = new Reference(origin, tag);
    return re;
}
#define REF(origin) Ref(origin,#origin)
RE_Build Ascii(char c)
{
    RE_Build re;
    re.reBody = new ASCIICharacter(c);
    return re;
}
RE_Build Unicode(wchar_t c)
{
    RE_Build re;
    re.reBody = new UnicodeCharacter(c);
    return re;
}
// Action node
RE_Build Action(std::function<void()> action)
{
    RE_Build re;
    re.reBody = new ActionNode(std::move(action));
    return re;
}

void RE_Ref::operator= (RE_Build && build)
{
    reHead->SetBody(build.reBody);
}

// > visit

void Visit(Node * reBody, REVisitor & visitor);

void Visit(RE_Ref & re, REVisitor & visitor)
{
    Visit(re.Dereference()->GetBody(), visitor);
}
void Visit(RE_Build & re, REVisitor & visitor)
{
    Visit(re.reBody, visitor);
}
void Visit(Node * reBody, REVisitor & visitor)
{
    assert(reBody);
    std::vector<Node *> nodeStack = { reBody };
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
// graph
// 1. data structure
//    * node points to node
// 2. build
//    * function based
// 3. simulate
// 4. to DFA helper
//    * see RegexMatcher.cpp

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
// graph
// 1. repr in memory
//    * table

// ==== CF ====
// A group of RE with ref.

// > repr

using CFRule = RE_Ref;
class CFGrammar
{
public:
    CFRule DeclareRule() { rules.emplace_back(); return rules.back(); }

    const std::vector<CFRule> & GetAllRules() const { return rules; }

    CFRule & StartRule() { return rules.front(); }

private:
    std::vector<CFRule> rules;
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
    RE2NFAConverter(std::map<RE_Ref, Nfa_Ref> * a0 = nullptr) : reRef2NfaRef(a0) {}
    
    Nfa_Build Get()
    {
        assert(stackNfa.size() == 1);
        return End(stackNfa.back());
    }

private:
    std::vector<Nfa_Build> stackNfa;
    std::map<RE_Ref, Nfa_Ref> * reRef2NfaRef;
};

// > CF -> NFA with ref
Nfa_Ref ConvertCF2NFA(CFGrammar & grammar)
{
    std::map<RE_Ref, Nfa_Ref> reRef2NfaRef;

    for (const CFRule & rule : grammar.GetAllRules())
    {
        reRef2NfaRef.emplace(rule, Nfa_Ref());
    }

    for (auto & p : reRef2NfaRef)
    {
        RE_Ref reRef = p.first;
        Nfa_Ref & nfaRef = p.second;

        RE2NFAConverter conv(&reRef2NfaRef);
        Visit(reRef, conv);

        nfaRef = conv.Get();
    }

    return reRef2NfaRef.at(grammar.StartRule());
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
        s.append(ToWString(r.GetTag()));
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

void PrintRE(RE_Ref re)
{
    REStringConverter converter;
    std::wcout << (Visit(re, converter), converter.Get()) << std::endl;
}
void PrintRE(RE_Build re)
{
    REStringConverter converter;
    std::wcout << (Visit(re, converter), converter.Get()) << std::endl;
}
#define PrintNamedRE(re) (std::wcout << L#re L" = ", PrintRE(re))
void PrintCF(CFGrammar & grammar)
{
    for (CFRule rule : grammar.GetAllRules())
    {
        std::wcout << rule.Dereference() << L" = ", PrintRE(rule);
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

void Test_RE_Basic()
{
    PrintRE(Ascii('a'));
    PrintRE(Ascii('a') & Ascii('b') | Ascii('c') & Ascii('d'));
    PrintRE(+Ascii('a') & *Ascii('b') & ~Ascii('c'));
}

// Test_NFA_Basic

void Test_Conv_RE_NFA()
{
#define REWithLabel(re) {re, #re}

    // RE -> NFA
    std::pair<RE_Build, std::string> cases[] = {
        REWithLabel(Ascii('a') & Ascii('b') & Ascii('c')),
        REWithLabel(Ascii('a') | Ascii('b') | Ascii('c')),
        REWithLabel(+Ascii('a')),
        REWithLabel(*Ascii('a')),
        REWithLabel(~Ascii('a')),
    };
    for (auto & p : cases)
    {
        RE_Build & re = p.first;
        std::string & label = p.second;

        RE2NFAConverter re2nfa;
        Visit(re, re2nfa);

        Nfa_Build nfa = re2nfa.Get();

        ShowNFA(nfa, label);
    }
}

void Test_RE_WithRef()
{
    RE_Ref S, A, B, C;

    S = REF(A) & REF(B) & REF(C);
    A = Ascii('a');
    B = Ascii('b');
    C = Ascii('c');

    PrintNamedRE(S);
    PrintNamedRE(A);
    PrintNamedRE(B);
    PrintNamedRE(C);
}

// Test_NFA_WithRef

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
    // Test_RE_Basic();
    // Test_NFA_Basic
    // Test_Conv_RE_NFA();

    // Test_RE_WithRef();
    // Test_NFA_WithRef
    // Test_CF();
    // Test_Conv_CF_NFAWithRef();

    CFGrammar grammar;

    CFRule S = grammar.DeclareRule();
    CFRule A = grammar.DeclareRule();
    CFRule B = grammar.DeclareRule();
    CFRule C = grammar.DeclareRule();

    S = +(REF(A) | REF(B) | REF(C));
    A = Ascii('a');
    B = Ascii('b');
    C = Ascii('c');

    PrintCF(grammar);

    Nfa_Ref nfa = ConvertCF2NFA(grammar);

    ShowNFA(nfa, "+(A|B|C)");

    // TODO: DFA
}
