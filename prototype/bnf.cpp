#include <cassert>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <vector>

// Support node: TERMINAL, NON_TERMINAL, [CODE]
// Support grammer: AND, OR, RECURSION, [EMPTY-PRODUCTION]
// Support parse: FIRST(), FOLLOW()

typedef char TOKEN;

void ERROR(const char *msg)
{
    std::cout << "ERROR: " << msg << std::endl;
    exit(1);
}

enum PT
{
    AND,
    OR,
    NODE,
};

struct N;

struct Production
{
    PT type;
    // for AND, OR
    Production *right_neigh;
    Production *first_child;
    Production *last_child;
    // for NODE
    N *n;

    static Production *Create(PT type)
    {
        Production *p = new Production();
        p->type = type;
        p->right_neigh = p->first_child = p->last_child = nullptr;
        p->n = nullptr;
        return p;
    }
};

// guide the generation of Production
struct PRODUCTION
{
    Production *p;

    // Is move-only
    // PRODUCTION() : p(Production::Create()) {}
    PRODUCTION(PRODUCTION &&o) : p(o.p)
    {
        o.p = nullptr;
    }
    PRODUCTION &operator=(PRODUCTION &&o)
    {
        p = o.p;
        o.p = nullptr;
        return (*this);
    }

    // Act like Production*
    PRODUCTION(Production *p_) : p(p_) {}
    operator Production *()
    {
        return p;
    }
    const operator Production *() const
    {
        return p;
    }
    Production *operator->()
    {
        return p;
    }
    const Production *operator->() const
    {
        return p;
    }

    // Support implicit "PRODUCTION(N)"
    PRODUCTION(N &n) : p(Production::Create(NODE))
    {
        p->n = &n;
    }
};

#define PRODUCTION_FOREACH(child, production)                     \
    for ((child) = (production)->first_child; (child) != nullptr; \
         (child) = (child)->right_neigh)

#define PRODUCTION_FOREACH_ADJ(p1, p2, production)                   \
    for ((p1) = (production)->first_child, (p2) = (p1)->right_neigh; \
         (p2) != nullptr; (p1) = (p2), (p2) = (p2)->right_neigh)

enum T
{
    UNKNOWN,
    TERMINAL,
    NON_TERMINAL,
    // CODE,
};

struct N
{
    T type;
    union {
        TOKEN c;
        PRODUCTION p;
    };
    mutable std::set<TOKEN> *first;
    mutable std::set<TOKEN> *follow;

    N() : type(UNKNOWN), first(nullptr), follow(nullptr) {}

    // Support "N = PRODUCTION"
    void operator=(PRODUCTION &&p);

    // Support "N = N"
    void operator=(N &n);

    // Support "N = char"
    void operator=(TOKEN c);
};

struct Ast
{
    T type;
    char c;
    Ast *right_neigh;
    Ast *first_child;
    Ast *last_child;

    Ast(T t) : type(t), c('\0'), right_neigh(nullptr), first_child(nullptr), last_child(nullptr) {}
    void add_child(Ast *child)
    {
        if (first_child == nullptr)
        {
            first_child = last_child = child;
        }
        else
        {
            last_child->right_neigh = child;
            last_child = child;
        }
        child->right_neigh = nullptr;
    }
};

#define CHECK_NO_RECURSION(p)                     \
    do                                            \
    {                                             \
        static std::set<void *> seen;             \
        if (!seen.insert((void *)p).second)       \
        {                                         \
            ERROR("Recursion found in grammer."); \
        }                                         \
    } while (false)

bool defined(const Production *p)
{
    assert(p != nullptr);

    bool yes;
    const Production *child;
    switch (p->type)
    {
        case AND: yes = defined(p->first_child); break;
        case OR:
            yes = true;
            PRODUCTION_FOREACH(child, p)
            {
                yes &= defined(child);
            };
            break;
        case NODE:
            assert(p->n != nullptr);
            yes = (p->n->type != UNKNOWN);
            break;
    }
    return yes;
}

template <class T>
bool merge_right(std::set<T> &l, const std::set<T> &r)
{
    bool inserted = false;
    for (auto const &item : r)
    {
        inserted |= l.insert(item).second;
    }
    return inserted;
}

void compute_first(std::vector<N *> grammer);
void compute_follow(std::vector<N *> grammer);
// FIRST()
void compute_first(Production *p, std::set<TOKEN> *first);
void compute_first(N *n)
{
    // CHECK_NO_RECURSION(n);

    if (n->first != nullptr)
        return;
    n->first = new std::set<TOKEN>();

    switch (n->type)
    {
        case TERMINAL: n->first->insert(n->c); break;
        case NON_TERMINAL: compute_first(n->p, n->first); break;
        default: assert(false); break;
    }
}
void compute_first(Production *p, std::set<TOKEN> *first)
{
    Production *child;
    switch (p->type)
    {
        case AND: compute_first(p->first_child, first); break;
        case OR:
            PRODUCTION_FOREACH(child, p)
            {
                compute_first(child, first);
            }
            break;
        case NODE:
            compute_first(p->n);
            first->insert(p->n->first->begin(), p->n->first->end());
            break;
    }
}
void compute_first(std::vector<N *> grammer)
{
    // N = N1 & N2 | N3 & N4 | N5;
    // FIRST(N) = FIRST(N1,N3,N5)
    for (auto n : grammer)
        compute_first(n);
}
// FOLLOW()
void compute_follow_1(Production *p);
void compute_follow_1(N *n)
{
    if (n->type == NON_TERMINAL)
        compute_follow_1(n->p);
}
void compute_follow_1(Production *p)
{
    Production *child, *p1, *p2;
    switch (p->type)
    {
        case AND:
            PRODUCTION_FOREACH_ADJ(p1, p2, p)
            {
                assert(p1->type == NODE && p2->type == NODE);
                p1->n->follow->insert(p2->n->first->begin(),
                                      p2->n->first->end());
            }
            break;
        case OR:
            PRODUCTION_FOREACH(child, p)
            {
                compute_follow_1(child);
            }
            break;
        case NODE: break;
    }
}
void compute_follow_2(Production *p, const std::set<TOKEN> *follow);
void compute_follow_2(N *n)
{
    if (n->type == NON_TERMINAL)
        compute_follow_2(n->p, n->follow);
}
void compute_follow_2(Production *p, const std::set<TOKEN> *follow)
{
    Production *child;
    switch (p->type)
    {
        case AND: compute_follow_2(p->last_child, follow); break;
        case OR:
            PRODUCTION_FOREACH(child, p)
            {
                compute_follow_2(child, follow);
            }
            break;
        case NODE:
            if (merge_right(*p->n->follow, *follow))
                compute_follow_2(p->n);
            break;
    }
}
void compute_follow(std::vector<N *> grammer)
{
    // N = N1 & N2 | N3 & N4 | N5;
    // rule 1:
    //  FOLLOW(N1) += FIRST(N2)
    //  FOLLOW(N3) += FIRST(N4)
    // rule 2:
    //  FOLLOW(N2,N4,N5) += FOLLOW(N)

    // initialize follow set of all N
    for (auto n : grammer)
    {
        assert(n->follow == nullptr);
        n->follow = new std::set<TOKEN>();
    }
    // compute rule 1: N1,N3
    for (auto n : grammer)
    {
        compute_follow_1(n);
    }
    // compute rule 2: N -> N2,N4,N5
    for (auto n : grammer)
    {
        compute_follow_2(n);
    }
}

bool is_first(TOKEN t, const N *n)
{
    return n->first->find(t) != n->first->end();
}
bool is_first(TOKEN t, const Production *p)
{
    bool is;
    switch (p->type)
    {
        case AND:
            is = is_first(t, p->first_child);
            break;
        case NODE:
            is = is_first(t, p->n);
            break;
        default: assert(false); break;
    }
    return is;
}

class TokenIter
{
   public:
    TokenIter(std::string s) : tokens(s)
    {
        token_it = tokens.cbegin();
    }
    bool has() const
    {
        return token_it != tokens.cend();
    }
    char next()
    {
        return *(token_it++);
    }
    char peak() const
    {
        assert(has());
        return *token_it;
    }
    void save()
    {
        its.emplace_back(token_it);
    }
    void load(bool remove = false)
    {
        assert(!its.empty());
        token_it = its.back();
        if (remove)
            its.pop_back();
    }

   private:
    std::string tokens;
    std::string::const_iterator token_it;
    std::vector<std::string::const_iterator> its;
};

Ast *parse(const Production *p, TokenIter &tk);
Ast *parse(const N *n, TokenIter &tk)
{
    if (!tk.has())
        ERROR("Expect token.");

    TOKEN next = tk.next();
    Ast *result;
    switch (n->type)
    {
        case TERMINAL:
            if (n->c != next)
                ;
            ERROR("Unexpected token.");
            result = new Ast(TERMINAL);
            result->c = n->c;
            break;
        case NON_TERMINAL:
            if (n->first->find(next) == n->first->end())
                ERROR("Unexpected token.");
            result = parse(n->p, tk);
            break;
        default: assert(false); break;
    }
    return result;
}
Ast *parse(const Production *p, TokenIter &tk)
{
    Production *child;

    if (p->type == OR)
    {
        bool found = false;
        PRODUCTION_FOREACH(child, p)
        {
            // greedy match
            if (is_first(tk.peak(), child))
            {
                p = child;
                break;
            }
        }
        if (!found)
            ERROR("Unmatched production.");
        assert(p->type != OR);
    }

    Ast *result;
    switch (p->type)
    {
        case AND:
            result = new Ast(NON_TERMINAL);
            PRODUCTION_FOREACH(child, p)
            {
                result->add_child(parse(child, tk));
            }
            break;
        case NODE: result = parse(p->n, tk); break;
        default: assert(false); break;
    }
    return result;
}

void N::operator=(PRODUCTION &&p)
{
    if (type != UNKNOWN)
        ERROR("N already defined.");
    if (!defined(p))
        ERROR("Production has undefined N.");

    type = NON_TERMINAL;
    this->p = std::move(p);
}

void N::operator=(N &n)
{
    (*this) = PRODUCTION(n);
}

void N::operator=(TOKEN ch)
{
    if (type != UNKNOWN)
        ERROR("N already defined.");

    type = TERMINAL;
    c = ch;
}

PRODUCTION operator&(PRODUCTION pl, PRODUCTION pr)
{
    if (pl->type == AND)
    {
        pl->last_child->right_neigh = pr;
        pl->last_child = pr;
        return pl;
    }
    else
    {
        PRODUCTION p = Production::Create(AND);
        p->first_child = pl;
        p->last_child = pr;
        pl->right_neigh = pr;
        return p;
    }
}

PRODUCTION operator|(PRODUCTION pl, PRODUCTION pr)
{
    if (pl->type == OR)
    {
        pl->last_child->right_neigh = pr;
        pl->last_child = pr;
        return pl;
    }
    else
    {
        PRODUCTION p = Production::Create(OR);
        p->first_child = pl;
        p->last_child = pr;
        pl->right_neigh = pr;
        return p;
    }
}

std::ostream &operator<<(std::ostream &o, const PRODUCTION &p)
{
    Production *child;
    const char *conn;
    switch (p->type)
    {
        case AND:
        case OR:
            o << '(';
            conn = "";
            PRODUCTION_FOREACH(child, p)
            {
                o << conn;
                o << PRODUCTION(child);
                conn = (p->type == AND ? " & " : " | ");
            };
            o << ')';
            break;
        case NODE: o << 'N'; break;
    }
    return o;
}

std::ostream &operator<<(std::ostream &o, const N &n)
{
    switch (n.type)
    {
        case NON_TERMINAL: o << n.p; break;
        case TERMINAL: o << "'" << n.c << "'"; break;
        default: o << '?'; break;
    }
    return o;
}

std::ostream &operator<<(std::ostream &o, const Ast &ast)
{
    const Ast *child;
    switch (ast.type)
    {
        case TERMINAL:
            o << ast.c;
            break;
        case NON_TERMINAL:
            o << '(';
            PRODUCTION_FOREACH(child, &ast)
            {
                o << *child;
            }
            o << ')';
            break;
        case UNKNOWN:
            o << '?';
            break;
    }
    return o;
}

template <class T>
std::ostream &operator<<(std::ostream &o, const std::set<T> &s)
{
    o << '{';
    for (const T &v : s)
    {
        o << v << ',';
    }
    o << '}';
    return o;
}

void simple_grammer()
{
    // N = TOKEN
    // N = N & N
    // N = TOKEN & N
    // N = N | N
    // N = N & N | N
    // A = B & A

    N a, b, c, d, e;
    a = 'a';
    b = 'b';
    c = a | b & a;
    d = c & a;

    std::vector<N *> grammer = {&a, &b, &c, &d};
    compute_first(grammer);
    compute_follow(grammer);
    for (auto n : grammer)
        std::cout << "first: " << *(n->first) << '\t'
                  << "follow: " << *(n->follow) << std::endl;
}

int main(int argc, char *argv[])
{
    simple_grammer();
    // Expr ::= Term ('+' Term | '-' Term)*
    // Term ::= Factor ('*' Factor | '/' Factor)*
    // Factor ::= ['-'] (Number | '(' Expr ')')
    // Number ::= Digit+
    //
    // N expr, term, fact, num;
    // expr = term | term & '+' & expr;
    // term = fact & '*' & term;
    // fact = '1';
}
