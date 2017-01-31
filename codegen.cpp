#include "codegen.h"

#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <list>
#include <string>
#include <utility>
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
    if (buffer[0] != '_' && buffer[0] != 'L')
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
    if (buffer[0] != '_' && buffer[0] != 'L')
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
    if (buffer[0] != '_' && buffer[0] != 'L')
        __data += '\t';
    __data += buffer;
    __data += '\n';
    // cout << "Data: " << buffer << endl;

    va_end(args);
}

StringRef CreateLabel(const char *fmt, ...)
{
    static list<string> labels;
    static int lid = 0;
    va_list args;
    va_start(args, fmt);

    char buffer[512];
    vsnprintf(buffer, 512, fmt, args);

    string s(buffer);
    s += '.';
    s += to_string(lid++);
    labels.push_back(std::move(s));

    va_end(args);

    return StringRef(labels.back().data(), labels.back().size());
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
