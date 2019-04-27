#include <iostream>
#include <string>
#include <cstdio>

// variable definition
// int 

extern "C"
{

int IntegerFromASM();
int ASMAdd(int a, int b);
int ASMDiv(int a, int b);
int ASMMul(int a, int b);
int ASMSum(int first, int last);
int ASMFibonacci(int i);
int ASMStrCmp(const char *s1, const char *s2, int count);
int ASMCallCFun();
int ASMIfElse(bool expr, int ifVal, int elseVal);

// float math

float FNeg(float a);
float SSE_FInc(float a);
float SSE_FAdd(float a, float b);
float SSE_FMul(float a, float b);
float SSE_FDiv(float a, float b);
int   SSE_Feq(float a, float b);
int   SSE_Flt(float a, float b);
int   FtoI(float a);
float ItoF(int i);

int IndIndPtr(int **p);

// movss xmm0, DWORD PTR [rcx]
float IndFloat(float * p);

int ASMIndirectCall(int(*)(int,int), int, int);

int CAdd(int a, int b) { return a + b; }
int CInt = 11;

extern const char ASMStringConst[];

int run();
int ASMfib(int i);

int puts(const char *);
int putchar(int ch);
int puti(int i);

}

std::string FloatToHexString(float f)
{
    char hex[9];
    int * ip = reinterpret_cast<int *>(&f);
    sprintf_s(hex, 9, "%0.8x", *ip);
    return std::string(hex, hex + 8);
}

#define TEST(expr) (void)(std::cout << (#expr) << " returns " << (expr) << std::endl)


int main(int argc, char *argv[])
{
    std::cout << "IntegerFromASM returns " << IntegerFromASM() << std::endl;
    TEST(ASMAdd(7, 11));
    TEST(ASMDiv(8, 2));
    TEST(ASMMul(4, 7));
    TEST(ASMSum(1, 100));
    TEST(ASMFibonacci(1));
    TEST(ASMFibonacci(3));
    TEST(ASMFibonacci(5));
    TEST(ASMFibonacci(7));
    TEST(ASMStrCmp("b", "a", 2));
    TEST(ASMStrCmp("b", "b", 2));
    TEST(ASMStrCmp("b", "c", 2));
    TEST(ASMStrCmp("b", "aa", 2));
    TEST(ASMStrCmp("b", "ba", 2));
    TEST(ASMStrCmp("b", "ca", 2));
    TEST(ASMCallCFun());
    TEST(ASMIfElse(true, 1, 2));
    TEST(ASMIfElse(false, 1, 2));
    TEST(ASMIndirectCall(ASMMul, 3, 7));
    TEST(FloatToHexString(1.0f));
    TEST(ASMStringConst);

    TEST(FNeg(1.0f));
    TEST(FNeg(-1.0f));
    TEST(SSE_FInc(2.0f));
    TEST(SSE_FAdd(3.0f, 5.0f));
    TEST(SSE_FMul(3.0f, 5.0f));
    TEST(SSE_FDiv(12.0f, 3.0f));
    TEST(SSE_Feq(2.0f, 3.0f));
    TEST(SSE_Feq(3.0f, 3.0f));
    TEST(SSE_Feq(4.0f, 3.0f));
    TEST(SSE_Flt(2.0f, 3.0f));
    TEST(SSE_Flt(3.0f, 3.0f));
    TEST(SSE_Flt(4.0f, 3.0f));

    TEST(FtoI(3.0f));
    TEST(ItoF(7));

    int i = 23;
    int *p = &i;
    int **pp = &p;
    TEST(IndIndPtr(pp));

    TEST(run());
    //TEST(ASMfib(1));
    //TEST(ASMfib(2));
    //TEST(ASMfib(3));
    //TEST(ASMfib(4));
    //TEST(ASMfib(5));
    //TEST(ASMfib(6));
    //TEST(ASMfib(7));
    //TEST(ASMfib(8));
    //TEST(ASMfib(9));
    //TEST(ASMfib(10));

    float f = 4.0f;
    TEST(IndFloat(&f));

    puts("this is C puts().");

    puti(1);

    printf("this is C printf()\n");

    return 0;
}