#include "codegen.h"

#include <cstdarg>
#include <cstdio>
#include <iostream>
using namespace std;

static string __code;
static string __decl;
static string __data;

void Emit(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    char buffer[512];
    vsnprintf(buffer, 512, fmt, args);
    if (buffer[0] != '_')
        __code += '\t';
    __code += buffer;
    __code += '\n';
    // cout << "Code: " << buffer << endl;

    va_end(args);
}

void EmitDecl(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    char buffer[512];
    vsnprintf(buffer, 512, fmt, args);
    if (buffer[0] != '_')
        __decl += '\t';
    __decl += buffer;
    __decl += '\n';
    // cout << "Decl: " << buffer << endl;

    va_end(args);
}

void EmitData(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    char buffer[512];
    vsnprintf(buffer, 512, fmt, args);
    if (buffer[0] != '_')
        __data += '\t';
    __data += buffer;
    __data += '\n';
    // cout << "Data: " << buffer << endl;

    va_end(args);
}

string Emitted()
{
    string s = __decl;
    s += "\t.text\n";
    s += __code;
    s += "\t.data\n";
    s += __data;
    return s;
}
