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

/* void */

/* char */

/* int */
int int_property()
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
    expect("x++", x++, 2);
    expect("x", x, 3);
    expect("--x", --x, 2);
    expect("x--", x--, 2);
    expect("x", x, 1);
    x = 11;
    y = 3;
    expect("x + y", x + y, 14);
    expect("x - y", x - y, 8);
    expect("x * y", x * y, 33);
    expect("x / y", x / y, 3);
    expect("x % y", x % y, 2);

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

    /* all */


    return 0;
}

int int_operation()
{
}

int int_types()
{

}

/* float: property, operation */

/* array */

/* pointer */

/* function */

/* enum */

/* struct/union */

int main()
{
    int_property();

    return 0;
}
