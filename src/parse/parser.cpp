#include "parser.h"

//#define DEBUG
#include "logging/logging.h"

#include <algorithm>
#include <cassert>

TokenMatcher & Production::symbol() {
    assert(type_ == SYM);
    return symbol_;
}
const TokenMatcher & Production::symbol() const {
    assert(type_ == SYM);
    return symbol_;
}
CodeObject & Production::code() {
    assert(type_ == CODE);
    return code_;
}
Production & Production::production() {
    assert(type_ == PROD);
    return *production_;
}
const Production & Production::production() const {
    assert(type_ == PROD);
    return *production_;
}
ProductionList & Production::children() {
    assert(type_ == AND || type_ == OR || type_ == REP || type_ == OPT);
    return children_;
}
const ProductionList & Production::children() const {
    assert(type_ == AND || type_ == OR || type_ == REP || type_ == OPT);
    return children_;
}
TokenMatcherSet & Production::FIRST() {
    assert(type_ != CODE);
    return FIRST_;
}
TokenMatcherSet & Production::FOLLOW() {
    assert(type_ != CODE);
    return FOLLOW_;
}

ProductionTreeView::postorder_iterator::postorder_iterator() {
}
ProductionTreeView::postorder_iterator::postorder_iterator(Production & root) {
    next_.push_back(&root.production());
}
ProductionTreeView::postorder_iterator &
ProductionTreeView::postorder_iterator::operator++() {
    if (next_.empty())
        return (*this);

    Production * p = next_.front();
    switch (p->type())
    {
        case Production::SYM:
        case Production::CODE:
        case Production::PROD:
            break;
        case Production::AND:
        case Production::OR:
        case Production::OPT:
        case Production::REP:
            for (Production * child : p->children())
            {
                next_.push_back(child);
            }
            break;
    }
    next_.pop_front();
    return (*this);
}
Production & ProductionTreeView::postorder_iterator::operator*() {
    assert(!next_.empty());
    return *next_.front();
}
bool ProductionTreeView::postorder_iterator::operator==(
    const ProductionTreeView::postorder_iterator & other) const {
    if (next_.size() != other.next_.size())
        return false;
    for (size_t i = 0; i < next_.size(); ++i)
    {
        if (next_[i] != other.next_[i])
            return false;
    }
    return true;
}
bool ProductionTreeView::postorder_iterator::operator!=(
    const ProductionTreeView::postorder_iterator & other) const {
    return !(*this == other);
}
ProductionTreeView::postorder_iterator ProductionTreeView::begin() {
    return postorder_iterator(p_);
}
ProductionTreeView::postorder_iterator ProductionTreeView::end() {
    return postorder_iterator();
}
void iterate_and_do_impl(
    const Production * p,
    std::function<void(const Production &)> before_children,
    std::function<void(const Production &)> before_child,
    std::function<void(const Production &)> after_child,
    std::function<void(const Production &)> after_children) {
    assert(p);
    before_children(*p);
    switch (p->type())
    {
        case Production::AND:
        case Production::OR:
        case Production::OPT:
        case Production::REP:
            for (Production * child : p->children())
            {
                before_child(*p);
                iterate_and_do_impl(child,
                                    before_children,
                                    before_child,
                                    after_child,
                                    after_children);
                after_child(*p);
            }
            break;
        default:
            break;
    }
    after_children(*p);
}
std::string ProductionTreeView::str() const {
    std::string s;
    iterate_and_do_impl(&p_.production(),
                        [&s](const Production & p) { // before children
                            switch (p.type())
                            {
                                case Production::SYM:
                                    s += '\'';
                                    s += p.symbol().toString();
                                    s += '\'';
                                    break;
                                case Production::PROD:
                                    s += p.name();
                                    break;
                                case Production::CODE:
                                    s += "<code>";
                                    break;
                                case Production::AND:
                                    s += "(and";
                                    break;
                                case Production::OR:
                                    s += "(or";
                                    break;
                                case Production::OPT:
                                    s += "(opt";
                                    break;
                                case Production::REP:
                                    s += "(rep";
                                    break;
                                default:
                                    break;
                            }
                        },
                        [&s](const Production & p) { // before each child
                            switch (p.type())
                            {
                                case Production::AND:
                                case Production::OR:
                                case Production::OPT:
                                case Production::REP:
                                    s += ' ';
                                    break;
                                default:
                                    break;
                            }
                        },
                        [&s](const Production & p) { // after each child
                        },
                        [&s](const Production & p) { // after children
                            switch (p.type())
                            {
                                case Production::AND:
                                case Production::OR:
                                case Production::OPT:
                                case Production::REP:
                                    s += ')';
                                    break;
                                default:
                                    break;
                            }
                        });
    return s;
}
void ProductionTreeView::print() const {
    std::cout << str() << std::endl;
}

ProductionListView::adjacent_iterator::adjacent_iterator(ProductionList & PL,
                                                         size_t second)
    : pl_(PL)
    , second_(std::min(second, PL.size())) {
}
ProductionListView::adjacent_iterator & ProductionListView::adjacent_iterator::
operator++() {
    if (second_ < pl_.size())
        ++second_;
    return (*this);
}
std::pair<Production &, Production &> ProductionListView::adjacent_iterator::
operator*() {
    assert(second_ > 0 && second_ < pl_.size());
    return std::pair<Production &, Production &>(*pl_[second_ - 1],
                                                 *pl_[second_]);
}
bool ProductionListView::adjacent_iterator::operator==(
    const adjacent_iterator & other) const {
    assert(pl_ == other.pl_);
    return second_ == other.second_;
}
bool ProductionListView::adjacent_iterator::operator!=(
    const adjacent_iterator & other) const {
    return !(*this == other);
}
ProductionListView::iterator::iterator(ProductionList & PL, size_t pos)
    : pl_(PL)
    , i_(std::min(pos, PL.size())) {
}
ProductionListView::iterator & ProductionListView::iterator::operator++() {
    if (i_ < pl_.size())
        ++i_;
    return (*this);
}
Production & ProductionListView::iterator::operator*() {
    assert(i_ < pl_.size());
    return *pl_[i_];
}
bool ProductionListView::iterator::operator==(const iterator & other) const {
    assert(pl_ == other.pl_);
    return i_ == other.i_;
}
bool ProductionListView::iterator::operator!=(const iterator & other) const {
    return !(*this == other);
}
ProductionListView::adjacent_iterator ProductionListView::adjacent_begin() {
    return adjacent_iterator(pl_, 1);
}
ProductionListView::adjacent_iterator ProductionListView::adjacent_end() {
    return adjacent_iterator(pl_, pl_.size());
}
ProductionListView::iterator ProductionListView::begin() {
    return iterator(pl_, 0);
}
ProductionListView::iterator ProductionListView::end() {
    return iterator(pl_, pl_.size());
}
Production & ProductionListView::first() {
    assert(pl_.size() > 0);
    return *pl_.front();
}
Production & ProductionListView::last() {
    assert(pl_.size() > 0);
    return *pl_.back();
}

ProductionListView ProductionListView::find_all_except(
    Production::Type type) const {
    ProductionList pl;
    for (Production * p : pl_)
    {
        if (!p->is(type))
            pl.push_back(p);
    }
    return ProductionListView(pl);
}
size_t ProductionListView::count(Production::Type type) const {
    size_t cnt = 0;
    for (Production * p : pl_)
    {
        if (p->is(type))
            ++cnt;
    }
    return cnt;
}

PRODUCTION::PRODUCTION(const PRODUCTION & p)
    : p_(p.p_) {
    assert(p_);
}
PRODUCTION::PRODUCTION(TokenMatcher symbol) {
    p_ = ProductionFactory::CreateWithName(Production::SYM, "SYM").p();
    p_->symbol_ = symbol;
}
PRODUCTION::PRODUCTION(CodeObject code) {
    p_ = ProductionFactory::CreateWithName(Production::SYM, "SYM").p();
    p_->code_ = std::move(code);
}
PRODUCTION PRODUCTION::operator*() {
    return ProductionBuilder::REP(*this);
}
PRODUCTION PRODUCTION::operator~() {
    return ProductionBuilder::OPT(*this);
}
PRODUCTION operator&(PRODUCTION p1, PRODUCTION p2) {
    if (p1->is(Production::AND))
    {
        p1->children().push_back(p2.p());
        return p1;
    }
    else
    { return ProductionBuilder::AND(p1, p2); }
}
PRODUCTION operator|(PRODUCTION p1, PRODUCTION p2) {
    if (p1->is(Production::OR))
    {
        p1->children().push_back(p2.p());
        return p1;
    }
    else
    { return ProductionBuilder::OR(p1, p2); }
}

void __compute_FIRST_FOLLOW(std::vector<PRODUCTION> & rules) {
    TopoMap<TokenMatcherSet *> D;
    for (PRODUCTION & root : rules)
    {
        assert(root->is(Production::PROD));
        DLOG(INFO) << root->production().name() << ".FIRST -> " << root->name()
                   << ".FIRST" << std::endl;
        D.add_dependency(&root->production().FIRST(), &root->FIRST());
        DLOG(INFO) << root->name() << ".FOLLOW -> " << root->production().name()
                   << ".FOLLOW" << std::endl;
        D.add_dependency(&root->FOLLOW(), &root->production().FOLLOW());

        for (Production & p : ProductionTreeView(*root.p()))
        {
            ProductionListView solid_children;
            switch (p.type())
            {
                case Production::SYM:
                    CHECK(p.FIRST().empty());
                    CHECK(p.FOLLOW().empty());
                    p.FIRST().addMatcher(p.symbol());
                    break;
                case Production::CODE:
                    break;
                case Production::PROD:
                    break;
                case Production::OPT:
                case Production::REP:
                    DLOG(INFO) << p.name() << ".FOLLOW -> " << p.name()
                               << ".FIRST" << std::endl;
                    D.add_dependency(&p.FOLLOW(), &p.FIRST());
                    // go down
                case Production::AND:
                    solid_children =
                        ProductionListView(p).find_all_except(Production::CODE);
                    DLOG(INFO) << solid_children.first().name() << ".FIRST -> "
                               << p.name() << ".FIRST" << std::endl;
                    D.add_dependency(&solid_children.first().FIRST(),
                                     &p.FIRST());
                    DLOG(INFO) << p.name() << ".FOLLOW -> "
                               << solid_children.last().name() << ".FOLLOW"
                               << std::endl;
                    D.add_dependency(&p.FOLLOW(),
                                     &solid_children.last().FOLLOW());
                    for (auto adj = solid_children.adjacent_begin();
                         adj != solid_children.adjacent_end();
                         ++adj)
                    {
                        DLOG(INFO)
                            << (*adj).second.name() << ".FIRST -> "
                            << (*adj).first.name() << ".FOLLOW" << std::endl;
                        D.add_dependency(&(*adj).second.FIRST(),
                                         &(*adj).first.FOLLOW());
                    }
                    break;
                case Production::OR:
                    for (Production & child : ProductionListView(p))
                    {
                        DLOG(INFO) << child.name() << ".FIRST -> " << p.name()
                                   << ".FIRST" << std::endl;
                        D.add_dependency(&child.FIRST(), &p.FIRST());
                        DLOG(INFO) << p.name() << ".FOLLOW -> " << child.name()
                                   << ".FOLLOW" << std::endl;
                        D.add_dependency(&p.FOLLOW(), &child.FOLLOW());
                    }
                    break;
            }
        }
    }

    std::vector<TokenMatcherSet *> FL = D.sort();
    for (TokenMatcherSet * F : FL)
    {
        for (TokenMatcherSet * Fd : D.dependents(F))
        {
            Fd->addMatchers(*F);
        }
    }
}

bool __match_FIRST(Production & p, Token t) {
    return p.FIRST().match(t);
}

void __run_impl(Production & p, TokenIterator & ti) {
    bool matched;
    switch (p.type())
    {
        case Production::SYM:
            CHECK(p.symbol().match(ti.next()));
            break;
        case Production::PROD:
            __run_impl(p.production(), ti);
            break;
        case Production::CODE:
            p.code()(nullptr);
            break;
        case Production::AND:
            for (Production & child : ProductionListView(p))
            {
                __run_impl(child, ti);
            }
            break;
        case Production::OR:
            matched = false;
            for (Production & child : ProductionListView(p))
            {
                if (ti.has() && __match_FIRST(child, ti.peek()))
                {
                    __run_impl(child, ti);
                    matched = true;
                    break;
                }
            }
            assert(matched);
            break;
        case Production::OPT:
            if (ti.has() &&
                __match_FIRST(ProductionListView(p).first(), ti.peek()))
            {
                for (Production & child : ProductionListView(p))
                {
                    __run_impl(child, ti);
                }
            }
            break;
        case Production::REP:
            while (ti.has() &&
                   __match_FIRST(ProductionListView(p).first(), ti.peek()))
            {
                for (Production & child : ProductionListView(p))
                {
                    __run_impl(child, ti);
                }
            }
            break;
    }
}

void __build_impl(Production & p,
                  TokenIterator & ti,
                  Ast * parent_ast,
                  CodeContext * parent_context) {
    assert(parent_ast);
    assert(parent_context);

    Ast * ast;
    CodeContext context;
    bool matched;
    size_t count;
    switch (p.type())
    {
        case Production::SYM:
            CHECK_EQ(p.symbol(), ti.next());
            ast = new Ast(p.name());
            // PROP TokenMatcher
            parent_ast->add_child(ast);
            parent_context->push_child(ast);
            break;
        case Production::PROD:
            ast = new Ast(p.name());
            context.set_ast_current(ast);
            __build_impl(p.production(), ti, ast, &context);
            parent_ast->add_child(ast);
            parent_context->push_child(ast);
            break;
        case Production::CODE:
            // Code can only access production root node and existing children.
            p.code()(parent_context);
            break;
        case Production::AND:
            for (Production & child : ProductionListView(p))
            {
                __build_impl(child, ti, parent_ast, parent_context);
            }
            break;
        case Production::OR:
            matched = false;
            for (Production & child : ProductionListView(p))
            {
                if (ti.has() && __match_FIRST(child, ti.peek()))
                {
                    __build_impl(child, ti, parent_ast, parent_context);
                    matched = true;
                    break;
                }
            }
            assert(matched);
            break;
        case Production::OPT:
            if (ti.has() &&
                __match_FIRST(ProductionListView(p).first(), ti.peek()))
            {
                for (Production & child : ProductionListView(p))
                {
                    __build_impl(child, ti, parent_ast, parent_context);
                }
            }
            break;
        case Production::REP:
            while (ti.has() &&
                   __match_FIRST(ProductionListView(p).first(), ti.peek()))
            {
                count = p.children().size();
                for (Production & child : ProductionListView(p))
                {
                    --count;
                    if (count != 0)
                        parent_context->save();
                    __build_impl(child, ti, parent_ast, parent_context);
                    if (count != 0)
                        parent_context->load();
                }
            }
            break;
    }
}

void DebugPrint(const TokenMatcherSet & matcher_set) {
    std::cout << '[';
    for (const auto & matcher : matcher_set.matchers())
    {
        std::cout << matcher.toString() << ',';
    }
    std::cout << ']' << std::endl;
}

void Grammer::compile() {
    __compute_FIRST_FOLLOW(rules_);
    compiled_ = true;
}
void Grammer::run(TokenIterator & tokens) {
    assert(compiled_);
    for (PRODUCTION & P : rules_)
    {
        std::cout << P->name() << " = " << ProductionTreeView(*P.p()).str()
                  << std::endl
                  << "\tFIRST: ";
        DebugPrint(P->FIRST());
        std::cout << "\tFOLLOW: ";
        DebugPrint(P->FOLLOW());
    }
    __run_impl(*rules_[0].p(), tokens);
}
Ast * Grammer::match(TokenIterator & tokens) {
    assert(compiled_);
    Ast * root = new Ast(rules_[0]->name());
    CodeContext context;
    context.set_ast_current(root);
    __build_impl(*rules_[0].p(), tokens, root, &context);
    return root;
}
