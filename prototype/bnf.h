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
typedef int C_Type;
typedef std::set<char> TokenSet;

struct N;
typedef std::vector<N *> Grammer;

// TODO: better iteration
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

        void add_child(MD *child);

        static MD *Create(N *n);
        static MD *Create(Type type);
    } * md;

    template <typename T>
    struct iterator
    {
        friend struct PAND;
        friend struct POR;

       public:
        iterator<T>(const iterator<T> &other) : md(other.md) {}

        iterator<T> &operator++()
        {
            if (md)
                md = md->tree.right_neigh;
            return (*this);
        }

        T operator*()
        {
            return T(md);
        }

        bool operator==(const iterator &other) const
        {
            return md == other.md;
        }
        bool operator!=(const iterator &other) const
        {
            return !(*this == other);
        }

       private:
        explicit iterator<T>() : md(nullptr) {}
        explicit iterator<T>(MD *md_) : md(md_) {}

        MD *md;
    };

    void add_child(P child);
};

struct PNODE : public P
{
    // for iteration
    explicit PNODE(MD *md_);
    explicit PNODE(N &n);

    N *N_();
};
struct PAND : public P
{
    // for iteration
    explicit PAND(MD *md_);
    explicit PAND(PNODE pn);
    PAND(PNODE pn1, PNODE pn2);
    PAND(PAND pa1, PNODE pn2);

    static PAND PAND_NULL;
    iterator<PNODE> begin()
    {
        return iterator<PNODE>(md->tree.first_child);
    }
    iterator<PNODE> end()
    {
        return iterator<PNODE>();
    }
};
extern struct EPS
{
} EPSILON;
struct POR : public P
{
    bool eps;

    explicit POR(PAND pa);
    explicit POR(EPS e);
    POR(EPS e, PAND pa2);
    POR(PAND pa1, EPS e);
    POR(PAND pa1, PAND pa2);
    POR(POR po1, EPS e);
    POR(POR po1, PAND pa2);

    iterator<PAND> begin()
    {
        return iterator<PAND>(md->tree.first_child);
    }
    iterator<PAND> end()
    {
        return iterator<PAND>();
    }

    std::vector<N *> flatten(Token tk);
};

struct N
{
    enum Type
    {
        TERMINAL,
        NON_TERMINAL,
        CODE
    } type;

    union {
        // terminal
        Token t;
        // non-terminal
        POR po;
        // code
        std::function<void()> *func;
    };

    TokenSet first;
    TokenSet follow;

    N(){};
    explicit N(Token tk);
    explicit N(std::function<void()> f);
    explicit N(POR po_);

    N &operator=(Token tk);
    N &operator=(std::function<void()> f);
    N &operator=(N &n);
    N &operator=(EPS e);
    N &operator=(PAND pa);
    N &operator=(POR po);
};

struct A
{
    enum Type
    {
        A_TERMINAL,
        A_NON_TERMINAL
    } type;

    struct Prop
    {
        C_Type *ctype;
        Token tk;
    } prop;

    // non-terminal
    struct
    {
        A *right_neigh;
        A *first_child;
        A *last_child;
    } tree;

    void add_child(A *child);

    static A *Create(Type type);
};

PAND operator&(N &n1, N &n2);
PAND operator&(PAND pa, N &n2);
POR operator|(N &n1, N &n2);
POR operator|(N &n1, EPS e);
POR operator|(N &n1, PAND pa2);
POR operator|(EPS e, N &n2);
POR operator|(EPS e, PAND pa2);
POR operator|(PAND pa1, N &n2);
POR operator|(PAND pa1, EPS e);
POR operator|(PAND pa1, PAND pa2);
POR operator|(POR po1, N &n2);
POR operator|(POR po1, EPS e);
POR operator|(POR po1, PAND pa2);

class TokenIterator
{
   public:
    TokenIterator(std::string s_) : s(s_), i(0) {}

    bool has() const
    {
        return i < s.size();
    }

    char next()
    {
        assert(has());
        return s[i++];
    }

    char peek()
    {
        assert(has());
        return s[i];
    }

   private:
    std::string s;
    size_t i;
};

void compute_first_follow(Grammer &G);
A *generate_ast(N *n, TokenIterator &T);

class BNFDebugger
{
   public:
    static BNFDebugger instance;

    void register_node(N &n, std::string name);
    std::string get_name(N *n);
    std::string guess_first_follow(void *addr);
    void print_grammer() const;

   private:
    std::map<N *, std::string> m;
};

#define BNF_REGISTER(n) BNFDebugger::instance.register_node((n), (#n))
#define BNF_PRINT_GRAMMER() BNFDebugger::instance.print_grammer()
std::ostream &operator<<(std::ostream &o, P::MD &m);
std::ostream &operator<<(std::ostream &o, POR &po);
std::ostream &operator<<(std::ostream &o, N &n);
std::ostream &operator<<(std::ostream &o, TokenSet &ts);
std::ostream &operator<<(std::ostream &o,
                         std::map<TokenSet *, std::set<TokenSet *>> &m);