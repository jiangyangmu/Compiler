#include <stdio.h>


// ignore a, b, c, d
int f1(int a, int b, int c, int **d);

// should ignore 'b' in parameter-list of returned function type
int (*f2(int a))(int b)
{
    void * b = (void *)a;
    return b;
}

// k
int (*f3())(int k())
{
    return (void *)0;
}

int main()
{
    printf("%p\n", f2(1));
    return 0;
}
