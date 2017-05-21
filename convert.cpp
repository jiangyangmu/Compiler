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

// TypeBase *BooleanConversion(const TypeBase *t)
// {
//     return nullptr;
// }
// TypeBase *PointerConversion(const TypeBase *from, const TypeBase *to)
// {
//     assert(from != nullptr && to != nullptr);
//     EXPECT_TYPE_IS(from, T_POINTER);
//     EXPECT_TYPE_IS(to, T_POINTER);
//     // (void *) can be implicitly converted to and from any pointer to object
//     (not function)
//     // const/volatile can be added
//     // 0(NULL) can be implicitly converted to any pointer type (object &
//     function)
//     return TypeFactory::newInstance();
// }

// char, short int, bit-field, enum => int/unsigned int
// TODO: if int not large enough, use unsigned int
const TypeBase *IntegerPromotion(const TypeBase *t)
{
    assert(t != nullptr);
    const TypeBase *ret = nullptr;
    switch (t->type())
    {
        case T_CHAR:
        case T_INT:
        case T_ENUM: ret = TypeFactory::Primitive_Int(); break;
        default: SyntaxError("IntegerPromotion: invalid type"); break;
    }

    return ret;
}

// TODO: implement this
const TypeBase *AssignmentConversion(const TypeBase *from, const TypeBase *to)
{
    return to;
    // SyntaxError("Not implemented.");
    // return nullptr;
}
// TypeBase *LvalueConversion(const TypeBase *t, const Environment *env)
// {
//     assert(t != nullptr);
//     if (t->type() == T_ARRAY)
//         SyntaxError("lvalue conversion doesn't support array.");
//     if (t->isIncomplete(env))
//         SyntaxError("lvalue conversion doesn't support incomplete type.");
//     // if object uninitialized
// }

/*
  Rules:
    (long double, *) => long double
    (double, *) => double
    (float, *) => float
    [Integer Promotions]
    if same signedness => choose bigger integer
    if unsigned has bigger integer => choose unsigned
    if signed has bigger integer & can represent unsigned => choose signed
    else (equal big integer, or signed not large enough) => choose unsigned with
  signed integer
*/
const TypeBase *UsualArithmeticConversion(const TypeBase *l, const TypeBase *r)
{
    assert(l != nullptr && r != nullptr);
    assert(l->isArithmetic());
    assert(r->isArithmetic());

    // No conversion needed
    if (l->equal(*r))
        return l;

    const TypeBase *p = nullptr;

    // unsigned long int, * => unsigned long int
    if (l->equal(*TypeFactory::Primitive_ULong()))
        p = l;
    else if (r->equal(*TypeFactory::Primitive_ULong()))
        p = r;

    // long int, unsigned int => long int (or unsigned long int)
    else if (l->equal(*TypeFactory::Primitive_Long()) && r->equal(*TypeFactory::Primitive_UInt()))
        p = l;
    else if (r->equal(*TypeFactory::Primitive_Long()) && l->equal(*TypeFactory::Primitive_UInt()))
        p = r;

    // long int, *(not unsigned int) => long int
    else if (l->equal(*TypeFactory::Primitive_Long()))
        p = l;
    else if (r->equal(*TypeFactory::Primitive_Long()))
        p = r;

    // unsigned int, * => unsigned int
    else if (l->equal(*TypeFactory::Primitive_UInt()))
        p = l;
    else if (r->equal(*TypeFactory::Primitive_UInt()))
        p = r;

    // int, * => int
    else if (l->equal(*TypeFactory::Primitive_Int()))
        p = l;
    else if (r->equal(*TypeFactory::Primitive_Int()))
        p = r;

    assert(p != nullptr);
    // SyntaxWarning("Promotion: " + l->toString() + " + " + r->toString() +
    //               " = " + p->toString());

    return p;
}

// both operands have arithmetic type;
// both operands have compatible structure or union types;
// both operands have void type;
// both operands are pointers to qualified or unqualified versions of compatible
// types;
// one operand is a pointer and the other is a null pointer constant; or
// one operand is a pointer to an object or incomplete type and the other is a
// pointer to a qualified or unqualified version of void .
const TypeBase *CondResultConversion(const TypeBase *left, const TypeBase *right)
{
    SyntaxError("Not implemented.");
    return nullptr;
}
