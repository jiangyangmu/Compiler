int puts(const char * str);
int putchar(int ch);
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

int PrintIntArray(int * a, int size)
{
    int i;

    i = 0;
    while (i < size)
    {
        printf("a[");
        PrintInt(i);
        printf("] = ");
        PrintInt(a[i]);
        printf("\n");

        ++i;
    }

    return 0;
}

int main()
{
    int a[3];

    a[0] = 100;
    a[1] = 200;
    a[2] = 300;

    PrintIntArray(a, 3);

    return 42;
}
