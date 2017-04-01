#include "convert.h"

#include <climits>
#include <iostream>
using namespace std;

bool uadd_ok(unsigned int u1, unsigned int u2)
{
    unsigned int max_add = UINT_MAX - u1;
    return max_add >= u2;
}

bool usub_ok(unsigned int u1, unsigned int u2)
{
    return u1 >= u2;
}
bool umul_ok(unsigned int u, unsigned int v)
{
    unsigned int uv = u * v;
    return !u || (uv / u == v);
}

bool udiv_ok(unsigned int u1, unsigned int u2)
{
    return u2 != 0U;
}
bool umod_ok(unsigned int u1, unsigned int u2)
{
    return u2 != 0U;
}
bool tadd_ok(int i1, int i2)
{
    if (i2 > 0)
        return (i1 + i2) > i1;
    else if (i2 < 0)
        return (i1 + i2) < i1;
    else
        return true;
}

bool tsub_ok(int i1, int i2)
{
    return (i2 > INT_MIN) && tadd_ok(i1, -i2);
}
int tmul_ok(int i1, int i2)
{
    int result = i1 * i2;
    return !i1 || (result / i1 == i2);
}

int tdiv_ok(int i1, int i2)
{
    if (i2 == 0)
        return false;
    if (i1 == INT_MIN && i2 == -1)
        return false;
    return true;
}

/*
    11 %  7 == 4
   -11 %  7 == 3
    11 % -7 == -3
   -11 % -7 == -4
 */
int tmod_ok(int i1, int i2)
{
    return (i1 >= 0 && i2 > 0);
}
// warning-list: compare unsigned int with int

TypeBase *IntegralPromotion(TypeBase *left, TypeBase *right)
{
    assert(left != nullptr && right != nullptr);
    EXPECT_TYPE_IS(left, TC_INT);
    EXPECT_TYPE_IS(right, TC_INT);

    IntType *l = dynamic_cast<IntType *>(left);
    IntType *r = dynamic_cast<IntType *>(right);

    // No promotion needed
    if (l->equal(*r))
        return l;

    IntType *p = nullptr;

    // unsigned long int, * => unsigned long int
    if (l->equal(TypeFactory::ULong()))
        p = l;
    else if (r->equal(TypeFactory::ULong()))
        p = r;

    // long int, unsigned int => long int (or unsigned long int)
    else if (l->equal(TypeFactory::Long()) && r->equal(TypeFactory::UInt()))
        p = l;
    else if (r->equal(TypeFactory::Long()) && l->equal(TypeFactory::UInt()))
        p = r;

    // long int, *(not unsigned int) => long int
    else if (l->equal(TypeFactory::Long()))
        p = l;
    else if (r->equal(TypeFactory::Long()))
        p = r;

    // unsigned int, * => unsigned int
    else if (l->equal(TypeFactory::UInt()))
        p = l;
    else if (r->equal(TypeFactory::UInt()))
        p = r;

    // int, * => int
    else if (l->equal(TypeFactory::Int()))
        p = l;
    else if (r->equal(TypeFactory::Int()))
        p = r;

    assert(p != nullptr);
    // SyntaxWarning("Promotion: " + l->toString() + " + " + r->toString() +
    //               " = " + p->toString());

    return left;
}

/*
  Rules:
    (long double, *) => long double
    (double, *) => double
    (float, *) => float
    [Integer Promotions]
    if same signedness => choose bigger integer
    if unsigned has bigger integer => choose unsigned
    if signed has bigger integer & can represent unsigned => choose signed
    else (equal big integer, or signed not large enough) => choose unsigned with signed integer
*/
TypeBase * UsualArithmeticConversion(TypeBase *left, TypeBase *right)
{
    if (left && left->type() == TC_POINTER)
    {
        // EXPECT_TYPE_IS(right, TC_INT);
        return left;
    }
    if (right && right->type() == TC_POINTER)
    {
        // EXPECT_TYPE_IS(left, TC_INT);
        return right;
    }

    return IntegralPromotion(left, right);
}
