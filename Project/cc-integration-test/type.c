int putchar(int ch);
int puts(const char * fmt);
int printf(const char * fmt, ...);
int PrintInt(int i)
{
    int d;
    int m;
    int ten;

    ten = 10;

    m = 1000000000;
    d = 0;

    while (m >= 1 && d == 0)
    {
        d = i / m;
        i = i % m;
        m = m / ten;
    }

    putchar(d + '0');

    while (m >= 1)
    {
        d = i / m;
        i = i % m;
        m = m / ten;
        putchar(d + '0');
    }

    return 0;
}
int PrintHex(int * p)
{
    long int i;
    long int d;
    long int s;

    putchar('0');
    putchar('x');

    i = (long int)p;
    s = 60;
    d = 0;

    while (s >= 0)
    {
        d = (i >> s) & 15;
        s = s - 4;
        if (d < 10)
            putchar(d + '0');
        else
            putchar(d - 10 + 'a');
    }

    return 0;
}

int PrintFloat(float f)
{
    int i;
    if (f < 0.0)
    {
        putchar('-');
        f = -f;
    }
    i = (int)f;
    PrintInt(i);
    putchar('.');
    putchar('0');
    return 0;
}

int show(const char * msg)
{
    puts(msg);

    return 0;
}

int expect(const char * name, int act, int exp)
{
    if (act != exp)
    {
        printf("\n FAIL: ");
        printf(name);
        printf(" is ");
        PrintInt(act);
        printf(", expect ");
        PrintInt(exp);
        printf("\n\n");
    }

    return 0;
}
int expectp(const char * name, int * act, int * exp)
{
    if (act != exp)
    {
        printf("\n FAIL: ");
        printf(name);
        printf(" is ");
        PrintHex(act);
        printf(", expect ");
        PrintHex(exp);
        printf("\n\n");
    }

    return 0;
}
int expectf(const char * name, float act, float exp)
{
    if (act != exp)
    {
        printf("\n FAIL: ");
        printf(name);
        printf(" is ");
        PrintFloat(act);
        printf(", expect ");
        PrintFloat(exp);
        printf("\n\n");
    }

    return 0;
}

/* void */
int void_operations()
{
    return 0;
}

/* char */

/* int */
int int_operations()
{
    int x;
    int y;

    /* write, read */
    x = 1;
    y = 2;
    expect("x", x, 1);
    expect("y", y, 2);
    x = 3;
    y = 4;
    expect("x", x, 3);
    expect("y", y, 4);

    /* arith */
    x = 1;
    expect("-x", -x, -1);
    expect("++x", ++x, 2);
    expect("x", x, 2);
    expect("--x", --x, 1);
    expect("x", x, 1);
    x = 11;
    y = 3;
    expect("x + y", x + y, 14);
    expect("x - y", x - y, 8);
    expect("x * y", x * y, 33);
    expect("x / y", x / y, 3);
    expect("x % y", x % y, 2);
    expect("x * y + x / y - x % y", x * y + x / y - x % y, 34);

    /* bit */
    x = 1;
    expect("~x", ~x, -2);
    x = 3;
    y = 5;
    expect("x & y", x & y, 1);
    expect("x | y", x | y, 7);
    expect("x ^ y", x ^ y, 6);
    x = 1;
    expect("x << 1", x << 1, 2);
    expect("x << 31", x << 31, -2147483647 - 1);
    x = 1;
    /* UB */
    expect("x << 32", x << 32, 1);

    /* compare */
    x = 2;
    y = 2;
    expect("x == y", x == y, 1);
    expect("x != y", x != y, 0);
    expect("x < y", x < y, 0);
    expect("x <= y", x <= y, 1);
    expect("x > y", x > y, 0);
    expect("x >= y", x >= y, 1);
    x = 1;
    y = 3;
    expect("x == y", x == y, 0);
    expect("x != y", x != y, 1);
    expect("x < y", x < y, 1);
    expect("x <= y", x <= y, 1);
    expect("x > y", x > y, 0);
    expect("x >= y", x >= y, 0);
    x = 3;
    y = 1;
    expect("x == y", x == y, 0);
    expect("x != y", x != y, 1);
    expect("x < y", x < y, 0);
    expect("x <= y", x <= y, 0);
    expect("x > y", x > y, 1);
    expect("x >= y", x >= y, 1);

    return 0;
}

/* enum */

/* float */
int float_operations()
{
    float x;
    float y;

    /* write, read */
    x = 1.0;
    y = 2.0;
    expectf("x", x, 1.0);
    expectf("y", y, 2.0);
    x = 3.0;
    y = 4.0;
    expectf("x", x, 3.0);
    expectf("y", y, 4.0);

    /* arith */
    x = 1.0;
    expectf("-x", -x, -1.0);
    expectf("++x", ++x, 2.0);
    expectf("x", x, 2.0);
    expectf("--x", --x, 1.0);
    expectf("x", x, 1.0);
    x = 9.0;
    y = 3.0;
    expectf("x + y", x + y, 12.0);
    expectf("x - y", x - y, 6.0);
    expectf("x * y", x * y, 27.0);
    expectf("x / y", x / y, 3.0);
    expectf("x * y + x / y - x", x * y + x / y - x, 21.0);

    /* compare */
    x = 2.0;
    y = 2.0;
    expect("x == y", x == y, 1);
    expect("x != y", x != y, 0);
    expect("x < y", x < y, 0);
    expect("x <= y", x <= y, 1);
    expect("x > y", x > y, 0);
    expect("x >= y", x >= y, 1);
    x = 1.0;
    y = 3.0;
    expect("x == y", x == y, 0);
    expect("x != y", x != y, 1);
    expect("x < y", x < y, 1);
    expect("x <= y", x <= y, 1);
    expect("x > y", x > y, 0);
    expect("x >= y", x >= y, 0);
    x = 3.0;
    y = 1.0;
    expect("x == y", x == y, 0);
    expect("x != y", x != y, 1);
    expect("x < y", x < y, 0);
    expect("x <= y", x <= y, 0);
    expect("x > y", x > y, 1);
    expect("x >= y", x >= y, 1);

    return 0;
}

/* array */
int array_operations()
{
    int a[3];
    int *p;

    /* write, read */
    a[0] = 1;
    a[1] = 2;
    a[2] = 3;
    expect("a[0]", a[0], 1);
    expect("a[1]", a[1], 2);
    expect("a[2]", a[2], 3);
    0[a] = 4;
    1[a] = 5;
    2[a] = 6;
    expect("0[a]", 0[a], 4);
    expect("1[a]", 1[a], 5);
    expect("2[a]", 2[a], 6);
    *(a + 0) = 7;
    *(a + 1) = 8;
    *(a + 2) = 9;
    expect("*(a + 0)", *(a + 0), 7);
    expect("*(a + 1)", *(a + 1), 8);
    expect("*(a + 2)", *(a + 2), 9);

    /* to pointer */
    p = a;
    *p = 10;
    expect("a[0]", a[0], 10);

    return 0;
}

/* pointer */
int pointer_operations()
{
    int i;
    int *p;
    int **pp;

    /* write, read */
    p = &i;
    pp = &p;
    expectp("p", p, &i);
    expectp("*pp", *pp, p);

    /* target write, read */
    i = 1;
    expect("i", i, 1);
    expect("*p", *p, 1);
    expect("**pp", **pp, 1);
    *p = 2;
    expect("i", i, 2);
    expect("*p", *p, 2);
    expect("**pp", **pp, 2);
    **pp = 3;
    expect("i", i, 3);
    expect("*p", *p, 3);
    expect("**pp", **pp, 3);

    return 0;
}

/* struct/union */
int struct_operations()
{
    struct S { int x; char y; long int z; } s, s2;

    /* member access */
    s.x = 1;
    s.y = 2;
    s.z = 3;
    expect("s.x", s.x, 1);
    expect("s.y", s.y, 2);
    expect("s.z", s.z, 3);

    /* copy */
    s2.x = 4;
    s2.y = 5;
    s2.z = 6;
    s = s2;
    expect("s.x", s.x, 4);
    expect("s.y", s.y, 5);
    expect("s.z", s.z, 6);

    /* sizeof */
    expect("sizeof(S)", sizeof(s), 16);

    return 0;
}
int union_operations()
{
    union U { int x; char y; long int z; } u;

    /* member access */
    u.x = 300;
    expect("u.x", u.x, 300);
    u.y = 2;
    expect("u.y", u.y, 2);
    u.z = 999;
    expect("u.z", u.z, 999);

    /* sizeof */
    expect("sizeof(U)", sizeof(u), 8);

    return 0;
}

/* function */
int _2power(int e)
{
    if (e == 0)
        return 1;
    else
        return _2power(e - 1) + _2power(e - 1);
}
int function_operations()
{
    /* argument passing */

    /* result returning */

    /* recursion */
    expect("2^7", _2power(7), 128);

    return 0;
}

int main()
{
    int_operations();
    float_operations();
    array_operations();
    pointer_operations();
    struct_operations();
    union_operations();
    function_operations();

    return 0;
}
