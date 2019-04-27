int puts(const char *str);
int putchar(int ch);
int printf(const char *fmt);

int puti(int i)
{
    int d;
    int m;

    m = 10;
    if (i > 0)
    {
        d = i % m;
        i = i / m;
        puti(i);
        putchar(d + '0');
    }

    return 0;
}

int main()
{
    puts("hello, world!");
    printf("value = ");
    puti(123456);
    return 0;
}
