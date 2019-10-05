int fib(int ii)
{
    int i;
    i = ii;

    if (i <= 0)
    {
        return 0;
    }
    else
    {
        if (i <= 2)
            return 1;
        else
            return fib(i - 1) + fib(i - 2);
    }
}

int main()
{
    return fib(6);
}
