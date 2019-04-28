int puts(const char * str);
int putchar(int ch);
int printf(const char * fmt);

int PrintInt(int i)
{
    int d;
    int m;

    m = 1000000000;
    while (m >= 10)
    {
        d = i / m;
        i = i % m;
        m = m / 10;
        putchar(d + '0');
    }
    putchar('\n');

    return 0;
}

int PrintMessage(const char * msg)
{
    puts(msg);

    return 0;
}

int main()
{
    PrintMessage("hello, world!");
    PrintInt(123456);
    return 0;
}
