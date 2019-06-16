#include "api.h"

int  expectc(const char * name, char act, char exp)
{
    if (act != exp)
    {
        StdPrintf("\n FAIL: %s is '%c', expect '%c'\n\n", name, act, exp);
        return -1;
    }
    return 0;
}
int  expecti(const char * name, int act, int exp)
{
    if (act != exp)
    {
        StdPrintf("\n FAIL: %s is %d, expect %d\n\n", name, act, exp);
        return -1;
    }
    return 0;
}
void asserti(const char * name, int act, int exp)
{
    if (expecti(name, act, exp) != 0)
    {
        ProcExit(-1);
    }

    return;
}
void expectp(const char * name, int * act, int * exp)
{
    if (act != exp)
    {
        StdPrintf("\n FAIL: %s is 0x%016p, expect %0x%016p\n\n", name, act, exp);
    }

    return;
}
void expectf(const char * name, float act, float exp)
{
    if (act != exp)
    {
        StdPrintf("\n FAIL: %s is %f, expect %f\n\n", name, act, exp);
    }

    return;
}
void expectf_near(const char * name, float act, float exp)
{
    if ((act - exp) < -0.0001 || 0.0001 < (act - exp))
    {
        StdPrintf("\n FAIL: %s is %f, expect %f\n\n", name, act, exp);
    }

    return;
}
void expects(const char * name, const char * act, const char * exp)
{
    if (CStrCmp(act, exp) != 0)
    {
        StdPrintf("\n FAIL: %s is \"%s\", expect \"%s\"\n\n", name, act, exp);
    }

    return;
}

int test_file_io()
{
    struct FileHandle * file;
    unsigned long long size;
    char data[14];

    file = OpenFile("foo.txt", "w+");

    size = GetFileSize(file);
    asserti("file size", size, 0);

    WriteFile(file, "hello, world!", 13);
    SeekFileBegin(file);

    size = GetFileSize(file);
    asserti("file size", size, 13);

    ReadFile(file, data, size);
    data[size] = 0;

    CloseFile(file);

    expects("file content", data, "hello, world!");

    return 0;
}

/* test_stream_io */

int test_memory_op()
{
    char from[10];
    char to[10];
    int i;
    char c;

    /* compare */
    i = MemCmp("1", "1", 1);
    expecti("memcmp 1 1", i, 0);
    i = MemCmp("1", "2", 1);
    expecti("memcmp 1 2", i, -1);
    i = MemCmp("2", "1", 1);
    expecti("memcmp 2 1", i, 1);

    /* set */
    MemSet(from, from + 10, 'a');
    for (i = 0; i < 10; ++i)
    {
        c = from[i];
        expectc("memset", c, 'a');
    }

    /* copy, move */
    MemCpy(from, to, 10);
    for (i = 0; i < 10; ++i)
    {
        c = to[i];
        expectc("memset", c, 'a');
    }
    
    MemCpy("123456789", to, 10);
    MemMov(to + 2, to, 3);
    expects("memmove", to, "345456789");

    MemCpy("123456789", to, 10);
    MemMov(to + 2, to + 4, 3);
    expects("memmove", to, "123434589");

    return 0;
}
int test_string_op()
{
    char str[10];
    int i;

    /* compare */
    i = CStrCmp("1", "1");
    expecti("CStrCmp 1 1", i, 0);
    i = CStrCmp("1", "2");
    expecti("CStrCmp 1 2", i, -1);
    i = CStrCmp("2", "1");
    expecti("CStrCmp 2 1", i, 1);
    i = CStrCmp("11", "1");
    expecti("CStrCmp 11 1", i, 1);
    i = CStrCmp("1", "11");
    expecti("CStrCmp 1 11", i, -1);
    i = CStrCmp("21", "1");
    expecti("CStrCmp 21 1", i, 1);
    i = CStrCmp("01", "1");
    expecti("CStrCmp 01 1", i, -1);

    /* copy */
    MemSet(str, str + 10, '?');
    str[9] = 0;
    CStrCpy("123", str);
    expects("CStrCpy", str, "123");

    /* len */
    expecti("strlen \"\"", CStrLen(""), 0);
    expecti("strlen \"abc\"", CStrLen("abc"), 3);

    return 0;
}

int test_math_op()
{
    int i;
    int cnt;
    float f, pi, e;
    float sf, cf;

    expectf("ceil(0.5)", FltCeil(0.5), 1.0);
    expectf("floor(0.5)", FltFloor(0.5), 0.0);
    expectf("mod(3.5, 2)", FltMod(3.5, 2), 1.5);
    expectf("abs(1.0)", FltAbs(1.0), 1.0);
    expectf("abs(-1.0)", FltAbs(-1.0), 1.0);

    cnt = 180;
    pi = 3.1415926;
    f = 0.0;
    for (i = 0; i < cnt; ++i)
    {
        f = pi * i / cnt;
        sf = FltSin(f);
        cf = FltCos(f);
        expectf_near("sin(x)^2 + cos(x)^2",
                     sf * sf + cf * cf, 1.0);
        /*
        printf("sin(%d * pi / %d) = ", i, cnt);
        print_flt(FltSin(f));
        printf(", cos(%d * pi / %d) = ", i, cnt);
        print_flt(FltCos(f));
        printf(", tan(%d * pi / %d) = ", i, cnt);
        print_flt(FltTan(f));
        printf("\n");
        */
    }

    e = 2.71828;
    expectf_near("log(e)", FltLog(e), 1.0);
    expectf_near("log(e * e)", FltLog(e * e), 2.0);
    expectf("pow(2.0, 10.0)", FltPow(2.0, 10.0), 1024.0);
    expectf("sqrt(16.0)", FltSqrt(16.0), 4.0);

    return 0;
}

int test_data_conversion_op()
{
    /*
    test printf, scanf
    */
    return 0;
}

int test_time_op()
{
    StdPrintf("clock() = %d\n", GetClock());
    StdPrintf("time() = %d\n", GetCurrentTime());
    return 0;
}

void _custom_at_exit()
{
    StdPrintf("at_exit callback called.\n");
    return;
}
int test_process_op()
{
    StdPrintf("test exit(42)\n");
    ProcAtExit(_custom_at_exit);
    ProcExit(42);
    /*
    printf("test abort()\n");
    ProcAbort();
    */
    return 0;
}

int main()
{
    test_file_io();
    test_memory_op();
    test_string_op();
    test_math_op();
    test_data_conversion_op();
    test_time_op();
    test_process_op();

    return 0;
}
