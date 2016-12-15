#include "codegen.h"

#include <cstdarg>
#include <cstdio>
#include <iostream>
using namespace std;

static string __storage;

void Emit(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    char buffer[512];
    vsnprintf(buffer, 512, fmt, args);
    if (buffer[0] != '_')
        __storage += '\t';
    __storage += buffer;
    __storage += '\n';
    cout << "Emit: " << buffer << endl;

    va_end(args);
}

string Emitted()
{
    return __storage;
}
