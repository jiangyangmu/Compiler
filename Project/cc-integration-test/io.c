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

    putchar('\n');

    return 0;
}

int main()
{
    printf("hello, world!\n");
    PrintInt(123456);
    return 0;
}
