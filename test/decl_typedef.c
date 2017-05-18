
// 1. typedef basic test
typedef void V;
typedef char I1, _I1, __I1;
typedef short I2;
typedef int I4;
typedef float F4;
typedef double F8;
typedef int *IPtr;
typedef struct StructA_s { int i, j, k; } StructA;
typedef struct SIncomplete_s SIncomplete;
// typedef struct { int a, b, c; } SNoName;
typedef void (*FUNCPtr)(int i);
typedef void FUNC(int i);
FUNC *fun_point[3];
StructA sa;
// SIncomplete sin;

// 2. typedef of incomplete type
typedef int ArrayN[];
// ArrayN an; // Error: incomplete type

// 3. typedef of typedef
typedef void (*FP)(int i);
typedef FP *FPP;
typedef void HighF(int i, FP f);
typedef FPP (*SuperF[3])(int i, FP f);
FPP f1, f2, f3;
FPP fa[3];
SuperF sf;
// typedef FP *FPP;

// 4. typedef in different scope
typedef int Integer_file;
void func(void *p) {
    typedef int Integer_func;
    Integer_func i;
    // i = 3;
    // return i;
}
// Integer_func ig; // Error: undefined typedef in current scope
// struct A
// {
    // typedef int Integer; // Error: typedef in struct/union
// };

// 5. typedef scope tree
void func_st()
{
    typedef char VLInteger;
    VLInteger i1;
    {
        VLInteger _i2;
        typedef short VLInteger;
        VLInteger i2;
        {
            VLInteger _i4;
            typedef int VLInteger;
            VLInteger i4;
        }
    }
}

// 6. typedef scope hide
// int func_sh(void *Integer_file)
// {
    // Integer_file i = 4; // Error: treat Integer_file as parameter name.
    // return i;
// }

