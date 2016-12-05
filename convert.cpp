#include "convert.h"
#include <climits>

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


