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

int p4(int a, int b, int c, int d)
{
    return a + b + c + d;
}

int p5(int a, int b, int c, int d, int e)
{
    return a + b + c + d + e;
}

int p6(int a, int b, int c, int d, int e, int f)
{
    return a + b + c + d + e + f;
}

int main(void)
{
    return (p0()
            + p1(2)
            + p2(3, 4)
            + p3(5, 6, 7)
            + p4(8, 9, 10, 11)
            + p5(12, 13, 14, 15, 16)
            + p6(17, 18, 19, 20, 21, 22))
            == 253;
}
