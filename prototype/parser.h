#pragma once

#include "lexer.h"
#include "util.h"

#include <deque>
#include <list>
#include <set>
#include <string>
#include <utility>
#include <vector>

class Production;
typedef std::deque<Production *> ProductionList;

class Production {
    friend class ProductionFactory;
    friend class ProductionBuilder;
    friend class PRODUCTION;
public:
    enum Type { SYM, CODE, PROD, AND, OR, OPT, REP };

    std::string name() const { return name_; }
    Type type() const { return type_; }
    Token & symbol();
    const Token & symbol() const;
    std::function<void()> & code();
    Production & production();
    const Production & production() const;
    ProductionList & children();
    const ProductionList & children() const;
    TokenSet & FIRST();
    TokenSet & FOLLOW();
    bool is(Type type) { return type_ == type; }

private:
    explicit Production(Type type) : type_(type) {}
    explicit Production(Type type, const char *name) : name_(name), type_(type) {}

    std::string name_;
    // type {SYM, CODE, PROD, AND, OR, OPT, REP}
    Type type_;
    // data for all types
    // union {
        Token symbol_; // SYM
        std::function<void()> code_; // CODE
        Production *production_; // PROD
        ProductionList children_; // AND, OR, OPT, REP
    // };
    // FIRST,FOLLOW for all types (except CODE)
    TokenSet FIRST_;
    TokenSet FOLLOW_;
};

class ProductionTreeView {
public:
    class postorder_iterator {
        friend class ProductionTreeView;
    public:
        postorder_iterator & operator++();
        Production & operator*();
        bool operator==(const postorder_iterator &other) const;
        bool operator!=(const postorder_iterator &other) const;
    private:
        explicit postorder_iterator();
        explicit postorder_iterator(Production &root);
        std::deque<Production *> next_;
    };

    ProductionTreeView(Production &p) : p_(p) {}

    postorder_iterator begin();
    postorder_iterator end();
    std::string str() const;
    void print() const;

private:
    Production &p_;
};

class ProductionListView {
public:
    class adjacent_iterator {
        friend class ProductionListView;
    public:
        adjacent_iterator & operator++();
        std::pair<Production &, Production &> operator*();
        bool operator==(const adjacent_iterator &other) const;
        bool operator!=(const adjacent_iterator &other) const;
    private:
        explicit adjacent_iterator(ProductionList &PL, size_t second);
        ProductionList &pl_;
        size_t second_;
    };
    class iterator {
        friend class ProductionListView;
    public:
        iterator & operator++();
        Production & operator*();
        bool operator==(const iterator &other) const;
        bool operator!=(const iterator &other) const;
    private:
        explicit iterator(ProductionList &PL, size_t pos);
        ProductionList &pl_;
        size_t i_;
    };

    ProductionListView() {}
    ProductionListView(Production &p) : pl_(p.children()) {}
    adjacent_iterator adjacent_begin();
    adjacent_iterator adjacent_end();
    iterator begin();
    iterator end();
    Production & first();
    Production & last();
    ProductionListView find_all_except(Production::Type type) const;
    size_t count(Production::Type type) const;
    // void print() const;
    // std::string str() const;

private:
    ProductionListView(ProductionList &pl) : pl_(pl) {}

    ProductionList pl_;
};


class PRODUCTION {
    friend class ProductionFactory;
    Production *p_;
private:
// for creation
    PRODUCTION(Production *p) : p_(p) { assert(p_); }
public:
// for grammer
    PRODUCTION(const PRODUCTION &p) : p_(p.p_) { assert(p_); }
    void operator=(const PRODUCTION & prod)
    {
        assert(p_ && p_->is(Production::PROD));
        // assert(p_->production_ == nullptr);
        p_->production_ = prod.p_;
    }
    void operator=(PRODUCTION && prod)
    {
        assert(p_ && p_->is(Production::PROD));
        // assert(p_->production_ == nullptr);
        p_->production_ = prod.p_;
    }
// like POD
// like Production *
    Production * operator->()
    {
        assert(p_);
        return p_;
    }
    Production * p()
    {
        return p_;
    }
};

class ProductionFactory {
public:
    static PRODUCTION CreateWithName(Production::Type type, const char *name)
    {
        return PRODUCTION(new Production(type, name));
    }
    static PRODUCTION Create(Production::Type type)
    {
        return PRODUCTION(new Production(type));
    }
};

class ProductionBuilder {
public:
    static PRODUCTION SYM(Token symbol)
    {
        PRODUCTION p = ProductionFactory::Create(Production::SYM);
        p->symbol_ = symbol;
        return p;
    }
    static PRODUCTION CODE(std::function<void()> code)
    {
        PRODUCTION p = ProductionFactory::Create(Production::CODE);
        p->code_ = code;
        return p;
    }
    static PRODUCTION AND(PRODUCTION p1, PRODUCTION p2)
    {
        PRODUCTION p = ProductionFactory::Create(Production::AND);
        p->children_ = {};
        p->children_.push_front(p2.p());
        p->children_.push_front(p1.p());
        return p;
    }
    template <class... TL>
    static PRODUCTION AND(PRODUCTION p, TL ...ps)
    {
        PRODUCTION tail = AND(ps...);
        tail->children_.push_front(p.p());
        return tail;
    }
    // template <class... TL>
    // static PRODUCTION OR(TL ...pbs);
    // template <class... TL>
    // static PRODUCTION OPT(TL ...pbs);
    // template <class... TL>
    // static PRODUCTION REP(TL ...pbs);
    static PRODUCTION REP(PRODUCTION p1)
    {
        PRODUCTION p = ProductionFactory::Create(Production::REP);
        p->children_.push_front(p1.p());
        return p;
    }
    template <class... TL>
    static PRODUCTION REP(PRODUCTION p, TL ...ps)
    {
        PRODUCTION tail = REP(ps...);
        tail->children_.push_front(p.p());
        return tail;
    }
};

class Grammer {
public:
    void add(PRODUCTION p) { rules_.push_back(p); }
    void compile() { __compute_FIRST_FOLLOW(); }
    void run(TokenIterator & tokens);
    // Ast * match(TokenIterator tokens);

private:
    void __compute_FIRST_FOLLOW();
    std::vector<PRODUCTION> rules_;
};

/*
class Ast {
public:
    enum Type { SYM, PROD };

private:
// fields
    std::string name_;
    // type {SYM, PROD}
    Type type_;
    // data for all types
    union {
        Token * symbol_; // SYM
        std::vector<Ast*> * children_; // PROD
    };

public:
    void add_child(Ast &child);
};

// API
#define GM_BEGIN(PL) ProductionList PL
#define GM_ADD(PL, name)                               \
    (PL).add(new Production(#name, Production::PROD)); \
    Production &name = ((PL).last())
#define GM_END(PL)                \
    do                            \
    {                             \
        sanity_check(PL);         \
        compute_FIRST_FOLLOW(PL); \
        print_grammer(PL);        \
    } while (false)

// Ast * match(ProductionList & PL, TokenIterator & TL);
// void run(ProductionList &PL, TokenIterator &TL);
Ast * match_run(ProductionList & PL, TokenIterator & TL);
class AstFactory {
public:
    Ast * create(std::string name, Ast::Type type, Token symbol) { return nullptr; }
    Ast * create(std::string name, Ast::Type type) { return nullptr; }
};


// Impl
void sanity_check(ProductionList &PL);
void compute_FIRST_FOLLOW(ProductionList &PL);
Ast * match_run_impl(Production & P, TokenIterator & TL, Ast * AP, AstFactory & factory);
bool match_FIRST(Production &P, Token T);
void print_grammer(ProductionList &PL);
*/