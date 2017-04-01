int a, b, c;
int *pa, *pb;
int ia[3];
int func(int a, int b, int c, int aa[3][4]);
float (*p)(int a);
float (*pf3[3])(int a);

struct AA _a;
struct AA { int mem; } inst;

struct BB { int val; struct BB *next; } bs;

enum AEnum { XIXI, HAHA } aenum;

// normal declarations
// char cval;
int ival;

// array declarations
int iarr[3];
int iarr2[3][3];
int iarr3[3][3][3];

// pointer declarations
int * iptr;
int ** ipptr;
int *** ippptr;

// function pointer declarations
int (*pf)();
int * (*pfip)();
int ** (*pfipp)();
int (**ppf)();
int (***pppf)();

// complex declarations
int (* (* (*pfpfaafp) (int _p)) [3][3])(int _p1,int _p2);
int (* (* (* apfpfaafp[3] ) (int _p)) [3][3])(int _p1,int _p2);

int ff(int (*pf)(int _p), long _lp);
