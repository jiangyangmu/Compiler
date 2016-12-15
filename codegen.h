#pragma once

#include <string>
using namespace std;

void Emit(const char *fmt, ...);
string Emitted();

class Environment;

class CodeGenerator
{
   public:
    virtual void emit(const Environment *env) const {};
};
