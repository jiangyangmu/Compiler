#pragma once

#include "lexer.h"
#include "util.h"

#include <deque>
#include <functional>
#include <list>
#include <set>
#include <string>
#include <utility>
#include <vector>

class Production;
typedef std::deque<Production *> ProductionList;
class CodeContext;
typedef void CodeFunc(CodeContext *);

class Production {
    friend class ProductionFactory;
    friend class ProductionBuilder;
    friend class PRODUCTION;

public:
    enum Type { SYM, CODE, PROD, AND, OR, OPT, REP };

    std::string name() const {
        return name_;
    }
    Type type() const {
        return type_;
    }
    Token & symbol();
    const Token & symbol() const;
    std::function<CodeFunc> & code();
    Production & production();
    const Production & production() const;
    ProductionList & children();
    const ProductionList & children() const;
    TokenSet & FIRST();
    TokenSet & FOLLOW();
    bool is(Type type) {
        return type_ == type;
    }

private:
    explicit Production(Type type)
        : type_(type) {
    }
    explicit Production(Type type, const char * name)
        : name_(name)
        , type_(type) {
    }

    std::string name_;
    // type {SYM, CODE, PROD, AND, OR, OPT, REP}
    Type type_;
    // data for all types
    // union {
    Token symbol_; // SYM
    std::function<CodeFunc> code_; // CODE
    Production * production_; // PROD
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
        bool operator==(const postorder_iterator & other) const;
        bool operator!=(const postorder_iterator & other) const;

    private:
        explicit postorder_iterator();
        explicit postorder_iterator(Production & root);
        std::deque<Production *> next_;
    };

    explicit ProductionTreeView(Production & p)
        : p_(p) {
    }

    postorder_iterator begin();
    postorder_iterator end();
    std::string str() const;
    void print() const;

private:
    Production & p_;
};

class ProductionListView {
public:
    class adjacent_iterator {
        friend class ProductionListView;

    public:
        adjacent_iterator & operator++();
        std::pair<Production &, Production &> operator*();
        bool operator==(const adjacent_iterator & other) const;
        bool operator!=(const adjacent_iterator & other) const;

    private:
        explicit adjacent_iterator(ProductionList & PL, size_t second);
        ProductionList & pl_;
        size_t second_;
    };
    class iterator {
        friend class ProductionListView;

    public:
        iterator & operator++();
        Production & operator*();
        bool operator==(const iterator & other) const;
        bool operator!=(const iterator & other) const;

    private:
        explicit iterator(ProductionList & PL, size_t pos);
        ProductionList & pl_;
        size_t i_;
    };

    explicit ProductionListView() {
    }
    explicit ProductionListView(Production & p)
        : pl_(p.children()) {
    }
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
    explicit ProductionListView(ProductionList & pl)
        : pl_(pl) {
    }

    ProductionList pl_;
};

class PRODUCTION {
    friend class ProductionFactory;
    Production * p_;

private:
    // for creation
    PRODUCTION(Production * p)
        : p_(p) {
        assert(p_);
    }

public:
    // for grammer
    PRODUCTION(const PRODUCTION & p)
        : p_(p.p_) {
        assert(p_);
    }
    void operator=(const PRODUCTION & prod) {
        assert(p_ && p_->is(Production::PROD));
        // assert(p_->production_ == nullptr);
        p_->production_ = prod.p_;
    }
    void operator=(PRODUCTION && prod) {
        assert(p_ && p_->is(Production::PROD));
        // assert(p_->production_ == nullptr);
        p_->production_ = prod.p_;
    }
    // like POD
    // like Production *
    Production & operator*() {
        assert(p_);
        return *p_;
    }
    Production * operator->() {
        assert(p_);
        return p_;
    }
    Production * p() {
        return p_;
    }
    // for debug
    std::string DebugString() const {
        return ProductionTreeView(*p_).str();
    }
};

class ProductionFactory {
public:
    static PRODUCTION CreateWithName(Production::Type type, const char * name) {
        return PRODUCTION(new Production(type, name));
    }
    static PRODUCTION Create(Production::Type type) {
        return PRODUCTION(new Production(type));
    }
};

class ProductionBuilder {
public:
    static PRODUCTION SYM(Token symbol) {
        PRODUCTION p =
            ProductionFactory::CreateWithName(Production::SYM, "SYM");
        p->symbol_ = symbol;
        return p;
    }
    static PRODUCTION CODE(std::function<CodeFunc> code) {
        PRODUCTION p =
            ProductionFactory::CreateWithName(Production::CODE, "CODE");
        p->code_ = code;
        return p;
    }
    static PRODUCTION AND(PRODUCTION p1, PRODUCTION p2) {
        PRODUCTION p =
            ProductionFactory::CreateWithName(Production::AND, "AND");
        p->children_ = {};
        p->children_.push_front(p2.p());
        p->children_.push_front(p1.p());
        return p;
    }
    template <class... TL>
    static PRODUCTION AND(PRODUCTION p, TL... ps) {
        PRODUCTION tail = AND(ps...);
        tail->children_.push_front(p.p());
        return tail;
    }
    static PRODUCTION OR(PRODUCTION p1, PRODUCTION p2) {
        PRODUCTION p = ProductionFactory::CreateWithName(Production::OR, "OR");
        p->children_ = {};
        p->children_.push_front(p2.p());
        p->children_.push_front(p1.p());
        return p;
    }
    template <class... TL>
    static PRODUCTION OR(PRODUCTION p, TL... ps) {
        PRODUCTION tail = OR(ps...);
        tail->children_.push_front(p.p());
        return tail;
    }
    static PRODUCTION OPT(PRODUCTION p1) {
        PRODUCTION p =
            ProductionFactory::CreateWithName(Production::OPT, "OPT");
        p->children_.push_front(p1.p());
        return p;
    }
    // template <class... TL>
    // static PRODUCTION OPT(PRODUCTION p, TL... ps)
    //{
    //	PRODUCTION tail = REP(ps...);
    //	tail->children_.push_front(p.p());
    //	return tail;
    //}
    static PRODUCTION REP(PRODUCTION p1) {
        PRODUCTION p =
            ProductionFactory::CreateWithName(Production::REP, "REP");
        p->children_.push_front(p1.p());
        return p;
    }
    // template <class... TL>
    // static PRODUCTION REP(PRODUCTION p, TL... ps)
    //{
    //    PRODUCTION tail = REP(ps...);
    //    tail->children_.push_front(p.p());
    //    return tail;
    //}
};

class Ast {
public:
    Ast(std::string name)
        : name_(name) {
    }

    const std::string & name() const {
        return name_;
    }

    void add_child(Ast * child) {
        children_.push_back(child);
    }

    std::string DebugString() const {
        std::string s = "(" + name_;
        for (Ast * child : children_)
        {
            s += ' ';
            s += child->DebugString();
        }
        s += ')';
        return s;
    }

private:
    std::string name_;
    std::vector<Ast *> children_;
    // prop
};

class CodeContext {
public:
    // set/get prop
    // set/get child prop
    // is child exists?
    CodeContext()
        : current_(nullptr) {
    }

    Ast * current() {
        assert(current_);
        return current_;
    }
    Ast * child(size_t i) {
        if (i < children_.size())
            return children_[i];
        else
            return nullptr;
    }
    void set_current(Ast * current) {
        current_ = current;
    }
    void save() {
        saved_.push_back(children_.size());
    }
    void push_child(Ast * child) {
        children_.push_back(child);
    }
    void load() {
        assert(!saved_.empty() && saved_.back() <= children_.size());
        children_.resize(saved_.back());
        saved_.pop_back();
    }

    std::string DebugString() const {
        std::string s;
        s += "current: " + current_->name();
        s += "\tchildren: [";
        for (Ast * child : children_)
        {
            assert(child);
            s += child->name();
            s += ',';
        }
        s += ']';
        return s;
    }

private:
    Ast * current_;
    std::vector<Ast *> children_;
    std::vector<size_t> saved_;
};

class Grammer {
public:
    void add(PRODUCTION p) {
        rules_.push_back(p);
    }
    void compile();
    void run(TokenIterator & tokens);
    Ast * match(TokenIterator & tokens);

private:
    std::vector<PRODUCTION> rules_;
};

#define GM_BEGIN(G) Grammer G
#define GM_ADD(G, name)                                             \
    PRODUCTION name =                                               \
        ProductionFactory::CreateWithName(Production::PROD, #name); \
    (G).add(name)
#define GM_END(G) (G).compile()
