char c;
short s;
int i;
long l;
float f;
double d;
void * p;

enum EN { HAHA, XIXI } en;

struct ST { int a; char b; } st, *pst;

void (*func)(int a, int b);

int main()
{
    int a;
    int * ip;

    if (1) return 1;
    if (2) return 2;
    else return 3;

L1:
    goto L1;

    switch (4)
    {
        case 5: break;
        case 6: break;
        default: break;
    }

    ++a;
    --a;
    &a;
    *ip;
    +a;
    -a;
    ~a;
    !a;

    a;
    a++;
    a--;
    st.a;
    pst->a;
    func(a, a);
    ip[0];

    7, 7;
    a = 7;
    7 ? 7 : 7;
    7 || 7;
    7 && 7;
    7 | 7;
    7 ^ 7;
    7 & 7;
    7 == 7;
    7 > 7;
    7 << 7;
    7 + 7;
    7 % 7;

    while (8) continue;
    do {} while (9);

    for (;;) {}

    return 0;
}
