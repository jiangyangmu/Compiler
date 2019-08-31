#include "Lexer.h"

NfaStateFactory * gFactory = nullptr;

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

std::string NfaGraphToString(NfaGraph g)
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

void PrintDfaCompileContext(DfaCompileContext & context)
{
    for (const auto & n : context.nfaStates.States())
    {
        std::cout
            << n->id << (n->done ? "(done)" : (n->c ? "(non-empty)" : "")) << ":" << std::endl
            << "  --" << (n->c ? *n->c : '-') << "--> " << (n->next ? std::to_string(n->next->id) : "null") << std::endl
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
        std::cout << "\t" << NfaStateSetToString(dfa.dfa2nfa[state]) << std::endl;

        ++state;
    }
}

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

NfaStateSet Init(DfaCompileContext & context)
{
    NfaStateSet nss;
    for (const NfaGraph & g : context.nfaGraphs)
        nss.set(g.in->id);
    return nss;
}

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

Dfa Compile(DfaInput & input)
{
    std::vector<DfaTableRow> dfaTable;

    DfaCompileContext context = { input.nfaGraphs, input.nfaStateFactory };
    NfaStateSetToDfaState nfa2dfa;
    NfaStateSet nss;
    DfaState ds;

    // PrintNfaGraphAndEpsilonClosure(context);

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

    Dfa dfa;

    dfa.table = std::move(dfaTable);
    dfa.action.resize(dfa.table.size());
    dfa.dfa2nfa.resize(dfa.table.size());
    for (auto nss_ds : nfa2dfa.AsMap())
    {
        dfa.action[nss_ds.second] = GetAction(nss_ds.first, context);
        dfa.dfa2nfa[nss_ds.second] = nss_ds.first;
    }

    return dfa;
}

DfaMatchResult Dfa::Run(std::string & text)
{
    std::cout << "match \"" << text << "\"..." << std::endl;

    DfaState ds = 0;
    size_t i = 0;

    size_t matchedBranch = 0;
    size_t matchedLength = 0;

    if (this->action[ds].cont)
    {
        for (;
             i < text.size();
             ++i)
        {
            char ch = text.data()[i];
            size_t ds2 = this->table[ds][CharSet::CharIdx(ch)];

            std::cout
                << ds << NfaStateSetToString(this->dfa2nfa[ds])
                << " --" << CharSet::CharStr(ch) << "-> "
                << ds2 << NfaStateSetToString(this->dfa2nfa[ds2]) << std::endl;

            ds = ds2;

            if (this->action[ds].bad)
                break;

            if (matchedBranch == 0 ||
                (0 < this->action[ds].goodBranch && this->action[ds].goodBranch <= matchedBranch))
            {
                matchedBranch = this->action[ds].goodBranch;
                matchedLength = i + 1;
            }
        }
    }

    if (matchedBranch > 0)
    {
        if (matchedLength == text.size())
            std::cout << "matched pattern #" << matchedBranch << "." << std::endl;
        else
            std::cout << "prefix \"" << text.substr(0, matchedLength) << "\" matched pattern #" << matchedBranch << "." << std::endl;
    }
    else
    {
        std::cout << "not matched." << std::endl;
    }

    return { matchedLength, matchedBranch };
}
