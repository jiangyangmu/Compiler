int foo(int offset)
{
    int i;
    int j;
    int * p;
    int ** pp;
    int * q;

    p = &i;
    pp = &p;

    i = 1;
    *p = 2;
    **pp = 3;

    pp = pp - 1;
    p = p - 1;

    *(*(pp + 1) + 1) = 5;

    pp = pp + 1;
    p = p + 1;

    j = 4;
    q = &j;
    *q = *p;

    return j + offset;
}

int main()
{
    return foo(100);
}