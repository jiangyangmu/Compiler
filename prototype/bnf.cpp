#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

typedef char Token;
typedef std::set<char> TokenSet;

struct N;

struct P
{
    struct MD
    {
        enum Type
        {
            AND,
            OR,
            NODE
        } type;

        // node
        N *n;
        // and, or
        struct
        {
            MD *right_neigh;
            MD *first_child;
            MD *last_child;
        } tree;

        void add_child(MD *child)
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

        static MD *Create(N *n)
        {
            MD *m = new MD;
            m->type = NODE;
            m->n = n;
            m->tree.right_neigh = m->tree.first_child = m->tree.last_child =
                nullptr;
            return m;
        }
        static MD *Create(Type type)
        {
            assert(type != NODE);
            MD *m = new MD;
            m->type = type;
            m->n = nullptr;
            m->tree.right_neigh = m->tree.first_child = m->tree.last_child =
                nullptr;
            return m;
        }
    } * md;

    void add_child(P child)
    {
        md->add_child(child.md);
    }
};

struct PNODE : public P
{
    // for iteration
    explicit PNODE(MD *md_)
    {
        md = md_;
        assert(md->type == MD::NODE);
        assert(md->n);
    }
    explicit PNODE(N &n)
    {
        md = MD::Create(&n);
    }

    N *N()
    {
        return md->n;
    }
};
struct PAND : public P
{
    // for iteration
    explicit PAND(MD *md_)
    {
        md = md_;
        assert(md->type == MD::AND);
        assert(md->tree.first_child);
    }
    explicit PAND(PNODE pn)
    {
        md = MD::Create(MD::AND);
        md->add_child(pn.md);
    }
    PAND(PNODE pn1, PNODE pn2)
    {
        md = MD::Create(MD::AND);
        md->add_child(pn1.md);
        md->add_child(pn2.md);
    }
    PAND(PAND pa1, PNODE pn2)
    {
        md = pa1.md;
        md->add_child(pn2.md);
    }
};
struct EPS
{
} EPSILON;
struct POR : public P
{
    bool eps;

    explicit POR(PAND pa)
    {
        md = MD::Create(MD::OR);
        md->add_child(pa.md);
        eps = false;
    }
    explicit POR(EPS e)
    {
        md = MD::Create(MD::OR);
        eps = true;
    }
    POR(EPS e, PAND pa2)
    {
        md = MD::Create(MD::OR);
        md->add_child(pa2.md);
        eps = true;
    }
    POR(PAND pa1, EPS e)
    {
        md = MD::Create(MD::OR);
        md->add_child(pa1.md);
        eps = true;
    }
    POR(PAND pa1, PAND pa2)
    {
        md = MD::Create(MD::OR);
        md->add_child(pa1.md);
        md->add_child(pa2.md);
        eps = false;
    }
    POR(POR po1, EPS e)
    {
        md = po1.md;
        assert(!po1.eps);
        eps = true;
    }
    POR(POR po1, PAND pa2)
    {
        md = po1.md;
        md->add_child(pa2.md);
        eps = po1.eps;
    }
};

struct N
{
    enum Type
    {
        TERMINAL,
        NON_TERMINAL
    } type;

    union {
        // terminal
        Token t;
        // non-terminal
        POR po;
    };

    TokenSet first;
    TokenSet follow;

    N() {}
    N(Token tk) : type(TERMINAL), t(tk) {}
    explicit N(POR po_) : type(NON_TERMINAL), po(po_) {}

    N &operator=(Token tk);
    N &operator=(N &n);
    N &operator=(EPS e);
    N &operator=(PAND pa);
    N &operator=(POR po);
};

static PAND operator&(N &n1, N &n2)
{
    return PAND(PNODE(n1), PNODE(n2));
}
static PAND operator&(PAND pa, N &n2)
{
    pa.add_child(PNODE(n2));
    return pa;
}
static POR operator|(N &n1, N &n2)
{
    return POR(PAND(PNODE(n1)), PAND(PNODE(n2)));
}
static POR operator|(N &n1, EPS e)
{
    return POR(PAND(PNODE(n1)), e);
}
static POR operator|(N &n1, PAND pa2)
{
    return POR(PAND(PNODE(n1)), pa2);
}
static POR operator|(EPS e, N &n2)
{
    return POR(e, PAND(PNODE(n2)));
}
static POR operator|(EPS e, PAND pa2)
{
    return POR(e, pa2);
}
static POR operator|(PAND pa1, N &n2)
{
    return POR(pa1, PAND(PNODE(n2)));
}
static POR operator|(PAND pa1, EPS e)
{
    return POR(pa1, e);
}
static POR operator|(PAND pa1, PAND pa2)
{
    return POR(pa1, pa2);
}
static POR operator|(POR po1, N &n2)
{
    return POR(po1, PAND(PNODE(n2)));
}
static POR operator|(POR po1, EPS e)
{
    return POR(po1, e);
}
static POR operator|(POR po1, PAND pa2)
{
    return POR(po1, pa2);
}

N &N::operator=(Token tk)
{
    new (this) N(tk);
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

typedef std::vector<N *> Grammer;

#define FOREACH_PAND(pa, po)                             \
    for (P::MD *__md = (po).md->tree.first_child;        \
         (__md ? (new (pa) PAND(__md)) : nullptr), __md; \
         __md = __md->tree.right_neigh)

#define FOREACH_PNODE_PAIR(pn1, pn2, pa)                               \
    for (P::MD *__md1 = (pa).md->tree.first_child,                     \
               *__md2 = (__md1 ? __md1->tree.right_neigh : nullptr);   \
         (__md2 ? ((new (pn1) PNODE(__md1)), (new (pn2) PNODE(__md2))) \
                : nullptr),                                            \
               __md2;                                                  \
         __md1 = __md2, __md2 = __md2->tree.right_neigh)

#define FOREACH_PAND_HEAD(head, po)                                           \
    for (P::MD *__md = (po).md->tree.first_child;                             \
         (__md ? (new (head) PNODE(__md->tree.first_child)) : nullptr), __md; \
         __md = __md->tree.right_neigh)

#define FOREACH_PAND_TAIL(tail, po)                                          \
    for (P::MD *__md = (po).md->tree.first_child;                            \
         (__md ? (new (tail) PNODE(__md->tree.last_child)) : nullptr), __md; \
         __md = __md->tree.right_neigh)

std::map<N *, std::string> BNF;
#define BNF_REGISTER(n) BNF[&(n)] = (#n)
#define BNF_NAME(n) BNF[&(n)]
#define BNF_FOREACH(n, name)                                              \
    for (auto it = BNF.begin();                                           \
         it != BNF.end() && ((n) = it->first, (name) = it->second, true); \
         ++it)
std::string BNF_FIRST_FOLLOW(void *addr)
{
    size_t first_offset =
        (char *)&((N *)nullptr->*(&N::first)) - (char *)nullptr;
    size_t follow_offset =
        (char *)&((N *)nullptr->*(&N::follow)) - (char *)nullptr;
    N *first = (N *)((char *)addr - first_offset);
    N *follow = (N *)((char *)addr - follow_offset);
    if (BNF.find(first) != BNF.end())
        return BNF[first] + ".first";
    else if (BNF.find(follow) != BNF.end())
        return BNF[follow] + ".follow";
    else
        return "";
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
        //     std::cout << '\t' << BNF_FIRST_FOLLOW(kv.first) << ": " <<
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

            // std::cout << "Add seq: " << BNF_FIRST_FOLLOW(node) << std::endl;

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

std::ostream &operator<<(std::ostream &o,
                         std::map<TokenSet *, std::set<TokenSet *>> &m);

void compute_first_follow(Grammer &G)
{
    // collect eval dependancies.
    std::map<TokenSet *, std::set<TokenSet *>> M;
    std::map<TokenSet *, int> D;
    PNODE *pn1, *pn2;
    PAND *pa;
    pn1 = (PNODE *)(::operator new(sizeof(PNODE)));
    pn2 = (PNODE *)(::operator new(sizeof(PNODE)));
    pa = (PAND *)(::operator new(sizeof(PAND)));
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
            /* std::cout << "Add " << (name) << ": " << BNF_FIRST_FOLLOW(to) \
             * << " <- " << BNF_FIRST_FOLLOW(from) << std::endl;  */         \
        }                                                                    \
    } while (false)

        // first rule
        FOREACH_PAND_HEAD(pn1, n->po)
        {
            ADD_RULE("rule 1", (&pn1->N()->first), (&n->first));
        }
        if (n->po.eps)
        {
            ADD_RULE("rule 2", (&n->follow), (&n->first));
        }
        // follow rule
        FOREACH_PAND(pa, n->po)
        {
            FOREACH_PNODE_PAIR(pn1, pn2, *pa)
            {
                ADD_RULE("rule 3", (&pn2->N()->first), (&pn1->N()->follow));
            }
        }
        FOREACH_PAND_TAIL(pn1, n->po)
        {
            if (pn1->N() != n)
            {
                ADD_RULE("rule 4", (&n->follow), (&pn1->N()->follow));
            }
        }

#undef ADD_RULE
    }
    delete pa;
    delete pn2;
    delete pn1;

    // compute eval sequence
    std::vector<TokenSet *> Seq = topo_sort(M, D);

    // std::cout << "Eval seq: ";
    // for (auto ts : Seq)
    //     std::cout << '\t' << BNF_FIRST_FOLLOW(ts) << std::endl;

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

std::ostream &operator<<(std::ostream &o, P::MD &m)
{
    switch (m.type)
    {
        case P::MD::NODE: o << BNF_NAME(*m.n); break;
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
        o << BNF_FIRST_FOLLOW(kv.first) << " ->" << std::endl;
        for (auto ts : kv.second)
            o << '\t' << BNF_FIRST_FOLLOW(ts) << std::endl;
    }
    return o;
}

int main(void)
{
    N fact, add, mul, term, expr, start;
    BNF_REGISTER(fact);
    BNF_REGISTER(add);
    BNF_REGISTER(mul);
    BNF_REGISTER(term);
    BNF_REGISTER(expr);
    BNF_REGISTER(start);
    Grammer g = {&fact, &add, &mul, &term, &expr, &start};

    fact = '1';
    add = '+';
    mul = '*';
    term = fact | fact & add & term;
    expr = term | term & mul & expr;
    start = expr | EPSILON;

    compute_first_follow(g);

    N *n;
    std::string name;
    BNF_FOREACH(n, name)
    {
        if (n)
        {
            std::cout << name << " -> " << (*n) << std::endl
                      << "\tFIRST: " << n->first << std::endl
                      << "\tFOLLOW: " << n->follow << std::endl;
        }
    }

    return 0;
}
