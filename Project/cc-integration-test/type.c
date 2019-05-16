int putchar(int ch);
int puts(const char * fmt);
int printf(const char * fmt);
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

/* void */

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

/* float: property, operation */

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

    /* write, read */
    p = &i;
    expectp("p", p, &i);

    /* target write, read */

    return 0;
}

/* struct/union */

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
    array_operations();
    pointer_operations();
    function_operations();

    return 0;
}
