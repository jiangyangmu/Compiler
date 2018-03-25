#include "bnf.h"

N::N(Token tk) : type(TERMINAL), t(tk) {}
N::N(std::function<void()> f) : type(CODE)
{
    func = new std::function<void()>(std::move(f));
}
N::N(POR po_) : type(NON_TERMINAL), po(po_) {}

N &N::operator=(Token tk)
{
    new (this) N(tk);
    return (*this);
}
N &N::operator=(std::function<void()> f)
{
    new (this) N(f);
    return (*this);
}
N &N::operator=(N &n)
{
    (*this) = POR(PAND(PNODE(n)));
    return (*this);
}
N &N::operator=(EPS e)
{
    (*this) = POR(e);
    return (*this);
}
N &N::operator=(PAND pa)
{
    (*this) = POR(pa);
    return (*this);
}
N &N::operator=(POR po)
{
    new (this) N(po);
    return (*this);
}

void A::add_child(A *child)
{
    assert(type != A_TERMINAL);
    assert(child);
    if (tree.first_child == nullptr)
    {
        assert(!tree.last_child);
        tree.first_child = tree.last_child = child;
    }
    else
    {
        assert(tree.first_child && tree.last_child);
        assert(!child->tree.right_neigh);
        tree.last_child->tree.right_neigh = child;
        tree.last_child = child;
    }
}

A *A::Create(Type type, const char *name)
{
    A *a = new A;
    a->type = type;
    a->prop.ctype = nullptr;
    a->prop.tk = '\0';
    assert(name);
    a->prop.name = name;
    a->tree.right_neigh = a->tree.first_child = a->tree.last_child = nullptr;
    return a;
}
std::string A::DebugString()
{
    std::string s;

    s += "(";
    s += prop.name;
    s += " ";
    if (type == A_TERMINAL)
    {
        s += prop.tk;
    }
    else
    {
        for (A *child = tree.first_child; child;
             child = child->tree.right_neigh)
        {
            s += child->DebugString();
        }
    }
    s += ")";
    return s;
}

void P::MD::add_child(P::MD *child)
{
    assert(type != NODE);
    assert(child);
    if (tree.first_child == nullptr)
    {
        assert(!tree.last_child);
        tree.first_child = tree.last_child = child;
    }
    else
    {
        assert(tree.first_child && tree.last_child);
        assert(!child->tree.right_neigh);
        tree.last_child->tree.right_neigh = child;
        tree.last_child = child;
    }
}
P::MD *P::MD::Create(N *n)
{
    MD *m = new MD;
    m->type = NODE;
    m->n = n;
    m->tree.right_neigh = m->tree.first_child = m->tree.last_child = nullptr;
    return m;
}
P::MD *P::MD::Create(Type type)
{
    assert(type != NODE);
    MD *m = new MD;
    m->type = type;
    m->n = nullptr;
    m->tree.right_neigh = m->tree.first_child = m->tree.last_child = nullptr;
    return m;
}
void P::add_child(P child)
{
    md->add_child(child.md);
}

PNODE::PNODE(MD *md_)
{
    md = md_;
    assert(md->type == MD::NODE);
    assert(md->n);
}
PNODE::PNODE(N &n)
{
    md = MD::Create(&n);
}
N *PNODE::N_()
{
    return md->n;
}

PAND::PAND(MD *md_)
{
    md = md_;
    assert(md->type == MD::AND);
    assert(md->tree.first_child);
}
PAND::PAND(PNODE pn)
{
    md = MD::Create(MD::AND);
    md->add_child(pn.md);
}
PAND::PAND(PNODE pn1, PNODE pn2)
{
    md = MD::Create(MD::AND);
    md->add_child(pn1.md);
    md->add_child(pn2.md);
}
PAND::PAND(PAND pa1, PNODE pn2)
{
    md = pa1.md;
    md->add_child(pn2.md);
}

POR::POR(PAND pa)
{
    md = MD::Create(MD::OR);
    md->add_child(pa.md);
    eps = false;
}
POR::POR(EPS e)
{
    md = MD::Create(MD::OR);
    eps = true;
}
POR::POR(EPS e, PAND pa2)
{
    md = MD::Create(MD::OR);
    md->add_child(pa2.md);
    eps = true;
}
POR::POR(PAND pa1, EPS e)
{
    md = MD::Create(MD::OR);
    md->add_child(pa1.md);
    eps = true;
}
POR::POR(PAND pa1, PAND pa2)
{
    md = MD::Create(MD::OR);
    md->add_child(pa1.md);
    md->add_child(pa2.md);
    eps = false;
}
POR::POR(POR po1, EPS e)
{
    md = po1.md;
    assert(!po1.eps);
    eps = true;
}
POR::POR(POR po1, PAND pa2)
{
    md = po1.md;
    md->add_child(pa2.md);
    eps = po1.eps;
}

PAND operator&(N &n1, N &n2)
{
    return PAND(PNODE(n1), PNODE(n2));
}
PAND operator&(PAND pa, N &n2)
{
    pa.add_child(PNODE(n2));
    return pa;
}
POR operator|(N &n1, N &n2)
{
    return POR(PAND(PNODE(n1)), PAND(PNODE(n2)));
}
POR operator|(N &n1, EPS e)
{
    return POR(PAND(PNODE(n1)), e);
}
POR operator|(N &n1, PAND pa2)
{
    return POR(PAND(PNODE(n1)), pa2);
}
POR operator|(EPS e, N &n2)
{
    return POR(e, PAND(PNODE(n2)));
}
POR operator|(EPS e, PAND pa2)
{
    return POR(e, pa2);
}
POR operator|(PAND pa1, N &n2)
{
    return POR(pa1, PAND(PNODE(n2)));
}
POR operator|(PAND pa1, EPS e)
{
    return POR(pa1, e);
}
POR operator|(PAND pa1, PAND pa2)
{
    return POR(pa1, pa2);
}
POR operator|(POR po1, N &n2)
{
    return POR(po1, PAND(PNODE(n2)));
}
POR operator|(POR po1, EPS e)
{
    return POR(po1, e);
}
POR operator|(POR po1, PAND pa2)
{
    return POR(po1, pa2);
}

template <class T>
std::vector<T *> topo_sort(std::map<T *, std::set<T *>> &map,
                           std::map<T *, int> &ind)
{
    std::vector<T *> seq;
    assert(ind.size() == map.size());

    auto ind1 = ind, ind2 = ind;
    while (seq.size() < map.size())
    {
        // std::cout << "Ind: ";
        // for (auto kv : ind1)
        // {
        //     std::cout << '\t' << BNF_GUESS(kv.first) << ": " <<
        //     kv.second
        //               << std::endl;
        // }

        auto count = seq.size();
        for (auto kv : ind1)
        {
            T *node = kv.first;
            int &in = kv.second;
            if (in != 0)
                continue;

            seq.push_back(node);

            // std::cout << "Add seq: " << BNF_GUESS(node) << std::endl;

            for (T *node2 : map[node])
            {
                assert(node2 != node);
                assert(ind2[node2] > 0);
                --ind2[node2];
            }

            ind2[node] = -1;
        }
        if (seq.size() == count)
        {
            seq.clear();
            break;
        }
        ind1 = ind2;
    }

    return seq;
}

void compute_first_follow(Grammer &G)
{
    // collect eval dependancies.
    std::map<TokenSet *, std::set<TokenSet *>> M;
    std::map<TokenSet *, int> D;
    for (N *n : G)
    {
        if (M.find(&n->first) == M.end())
            M[&n->first] = {};
        if (M.find(&n->follow) == M.end())
            M[&n->follow] = {};
        if (D.find(&n->first) == D.end())
            D[&n->first] = 0;
        if (D.find(&n->follow) == D.end())
            D[&n->follow] = 0;

        if (n->type == N::TERMINAL)
        {
            n->first.insert(n->t);
            continue;
        }

#define ADD_RULE(name, from, to)                                             \
    do                                                                       \
    {                                                                        \
        if (M[from].count(to) == 0)                                          \
        {                                                                    \
            M[from].insert(to);                                              \
            D[to]++;                                                         \
            std::cout << "Add " << (name) << ": " << BNF_GUESS(to) << " <- " \
                      << BNF_GUESS(from) << std::endl;                       \
        }                                                                    \
    } while (false)

        // first rule
        for (PAND pa : n->po)  // for each non-code NODE head
        {
            for (PNODE pn : pa)
            {
                if (pn.N_()->type != N::CODE)
                {
                    ADD_RULE("rule 1", (&pn.N_()->first), (&n->first));
                    break;
                }
            }
        }
        if (n->po.eps)
        {
            ADD_RULE("rule 2", (&n->follow), (&n->first));
        }
        // follow rule
        for (PAND pa : n->po)  // for each adjacent non-code NODE pair
        {
            auto pn1 = pa.begin();
            for (; pn1 != pa.end(); ++pn1)
            {
                if ((*pn1).N_()->type != N::CODE)
                    break;
            }
            auto pn2 = pn1;
            for (++pn2; pn2 != pa.end(); ++pn2)
            {
                if ((*pn2).N_()->type != N::CODE)
                {
                    ADD_RULE("rule 3", (&(*pn2).N_()->first),
                             (&(*pn1).N_()->follow));
                    pn1 = pn2;
                }
            }
        }
        for (PAND pa : n->po)  // for each non-code NODE tail
        {
            N *pn_n = nullptr;
            for (PNODE pn : pa)
            {
                if (pn.N_()->type != N::CODE)
                {
                    pn_n = pn.N_();
                }
            }
            if (pn_n && pn_n != n)
            {
                ADD_RULE("rule 4", (&n->follow), (&pn_n->follow));
            }
        }

#undef ADD_RULE
    }

    // compute eval sequence
    std::vector<TokenSet *> Seq = topo_sort(M, D);

    // std::cout << "Eval seq: ";
    // for (auto ts : Seq)
    //     std::cout << '\t' << BNF_GUESS(ts) << std::endl;

    if (!M.empty() && Seq.empty())
    {
        std::cerr << "ERROR: find loop in BNF." << std::endl << M << std::endl;
        assert(false);
    }

    // do eval
    for (TokenSet *ff : Seq)
    {
        for (TokenSet *ffc : M[ff])
        {
            ffc->insert(ff->begin(), ff->end());
        }
    }
}

std::vector<N *> POR::flatten(Token tk)
{
    std::vector<N *> result;

    for (PAND pa : *this)
    {
        // try next branch.
        bool is_first = true;
        for (PNODE pn : pa)
        {
            // verify we are in the correct branch.
            N *n = pn.N_();
            if (is_first && n->type != N::CODE)
            {
                if (n->first.find(tk) == n->first.end())
                {
                    result.clear();
                    break;
                }
                is_first = false;
            }
            result.push_back(n);
        }
    }

    if (result.empty() && !eps)
    {
        std::cerr << "ERROR: first() not matched." << std::endl;
        assert(false);
    }

    return result;
}

A *generate_ast(N *n, TokenIterator &T)
{
    if (n->type == N::CODE)
    {
        (*n->func)();
        return nullptr;
    }
    else if (n->type == N::TERMINAL)
    {
        A *a = A::Create(A::A_TERMINAL, BNF_NAME(n).data());

        a->prop.tk = T.next();

        return a;
    }
    else
    {
        if (n->po.eps && !T.has())
            return nullptr;

        A *a = A::Create(A::A_NON_TERMINAL, BNF_NAME(n).data());

        std::vector<N *> vn = n->po.flatten(T.peek());

        for (N *n_child : vn)
        {
            A *a_child = generate_ast(n_child, T);
            if (a_child)
                a->add_child(a_child);
        }

        return a;
    }
}

BNFDebugger BNFDebugger::instance;
void BNFDebugger::register_node(N &n, std::string name)
{
    m[&n] = std::move(name);
}
const std::string &BNFDebugger::get_name(N *n)
{
    return m[n];
}
std::string BNFDebugger::guess_first_follow(void *addr)
{
    size_t first_offset =
        (char *)&((N *)nullptr->*(&N::first)) - (char *)nullptr;
    size_t follow_offset =
        (char *)&((N *)nullptr->*(&N::follow)) - (char *)nullptr;
    N *first = (N *)((char *)addr - first_offset);
    N *follow = (N *)((char *)addr - follow_offset);
    if (m.find(first) != m.end())
        return m[first] + ".first";
    else if (m.find(follow) != m.end())
        return m[follow] + ".follow";
    else
        return "";
}
void BNFDebugger::print_grammer() const
{
    for (auto kv : m)
    {
        N *n = kv.first;
        std::string &name = kv.second;
        if (n)
        {
            std::cout << name << " -> " << (*n) << std::endl
                      << "\tFIRST: " << n->first << std::endl
                      << "\tFOLLOW: " << n->follow << std::endl;
        }
    }
}
void BNFDebugger::clear()
{
    m.clear();
}
std::ostream &operator<<(std::ostream &o, P::MD &m)
{
    switch (m.type)
    {
        case P::MD::NODE: o << BNFDebugger::instance.get_name(m.n); break;
        case P::MD::AND:
            o << "(&";
            for (P::MD *child = m.tree.first_child; child;
                 child = child->tree.right_neigh)
            {
                o << ' ' << *child;
            }
            o << ")";
            break;
        default: break;
    }
    return o;
}
std::ostream &operator<<(std::ostream &o, POR &po)
{
    assert(po.md && po.md->type == P::MD::OR);
    switch (po.md->type)
    {
        case P::MD::OR:
            o << "(|";
            for (P::MD *child = po.md->tree.first_child; child;
                 child = child->tree.right_neigh)
            {
                o << ' ' << *child;
            }
            if (po.eps)
                o << " EPSILON";
            o << ")";
            break;
        default: break;
    }
    return o;
}
std::ostream &operator<<(std::ostream &o, N &n)
{
    switch (n.type)
    {
        case N::TERMINAL: o << "'" << n.t << "'"; break;
        case N::NON_TERMINAL: o << n.po; break;
        case N::CODE: o << "{}"; break;
    }
    return o;
}
std::ostream &operator<<(std::ostream &o, TokenSet &ts)
{
    for (char c : ts)
        o << "'" << c << "',";
    return o;
}
std::ostream &operator<<(std::ostream &o,
                         std::map<TokenSet *, std::set<TokenSet *>> &m)
{
    for (auto kv : m)
    {
        o << BNFDebugger::instance.guess_first_follow(kv.first) << " ->"
          << std::endl;
        for (auto ts : kv.second)
            o << '\t' << BNFDebugger::instance.guess_first_follow(ts)
              << std::endl;
    }
    return o;
}