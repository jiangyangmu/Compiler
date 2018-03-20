#include "bnf.h"

int main(void)
{
    N fact, add, mul, term, term_tail, expr, expr_tail, start, code;
    BNF_REGISTER(fact);
    BNF_REGISTER(add);
    BNF_REGISTER(mul);
    BNF_REGISTER(term);
    BNF_REGISTER(term_tail);
    BNF_REGISTER(expr);
    BNF_REGISTER(expr_tail);
    BNF_REGISTER(start);
    BNF_REGISTER(code);
    Grammer g = {&fact,      &add,  &mul,       &term,
                 &term_tail, &expr, &expr_tail, &start};

    int counter = 0;
    code = [&counter] { std::cout << counter++ << std::endl; };
    fact = '1';
    add = '+';
    mul = '*';
    term = fact & term_tail;
    term_tail = EPSILON | add & term_tail;

    expr = term & expr_tail;
    expr_tail = EPSILON | mul & expr_tail;

    start = expr | EPSILON;

    compute_first_follow(g);

    BNF_PRINT_GRAMMER();

    TokenIterator t("1+1*1+1");
    A *a = generate_ast(&start, t);

    return 0;
}
