int p0()
{
    return 1;
}

int p1(int a)
{
    return a;
}

int p2(int a, int b)
{
    return a + b;
}

int p3(int a, int b, int c)
{
    return a + b + c;
}

int main(void)
{
    return (p0() + p1(2) + p2(3, 4) + p3(5, 6, 7)) == 28;
}
