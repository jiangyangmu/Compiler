#pragma once

#include <string>
using namespace std;

void Emit(const char *fmt, ...);
void EmitDecl(const char *fmt, ...);
void EmitData(const char *fmt, ...);
string Emitted();

class Environment;

enum EEmitGoal
{
    FOR_NOTHING,
    FOR_VALUE,
    FOR_ADDRESS
};
class CodeGenerator
{
   public:
    virtual void emit(const Environment *env, EEmitGoal goal) const {};
};
