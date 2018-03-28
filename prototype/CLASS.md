// ```c++

class Token;

class Production {
// fields
    string name;
    // type {SYM, CODE, PROD, AND, OR, OPT, REP}
    Type type;
    // data for all types
    union {
        ref<Token> symbol; // SYM
        ref<function> code; // CODE
        ref<Production> production; // PROD
        ref<ProductionList> children; // AND, OR
        ref<Production> child; // OPT, REP
    };
    // FIRST,FOLLOW for all types (except CODE)
    set<Token> FIRST;
    set<Token> FOLLOW;

// methods
    bool is(Type type);
    // iterate production tree, only for PROD
    iterator begin();
    iterator end();
};

typedef list<Token> TokenList;

class ProductionList : public list<Production> {
// methods
    int count(Type type);
    ProductionList filter(Type type);
    // adjacent iterator, pair<Production, Production>
    iterator adjacent_begin();
    iterator adjacent_end();
    // normal iterator
    iterator begin();
    iterator end();
};

class Ast {
// fields
    string name;
    // type {SYM, PROD}
    Type type;
    // data for all types
    union {
        ref<Token> symbol; // SYM
        ref<list<Ast>> children; // PROD
    };
};

// API
#define GRAMMER_BEGIN(PL, ...)
#define GRAMMER_END(PL)
// Ast * match(ProductionList & PL, TokenList & TL);
// void run(ProductionList &PL, TokenList &TL);
Ast * match_run(ProductionList & PL, TokenList & TL);
class AstFactory {
    Ast * Create(string name, Type type, Token symbol);
    Ast * Create(string name, Type type);
};


// Impl

#define CHECK()
#define CHECK_GT()
#define CHECK_EQ()

void sanity_check(ProductionList PL);
void compute_FIRST_FOLLOW(ProductionList PL);
Ast * match_run_impl(Production P, TokenList TL, Ast & AP);
bool match_FIRST(Production P, Token T);

template <class T>
class TopoMap {
// methods
    void add_dependency(T t, T dependent);
    list<T> dependents(T t);
    // dependent last
    list<T> sort();
};


// ```