#pragma once

#include <iostream>
#include <string>
#include <vector>
using namespace std;

enum InstType
{
    INST_ADD,
    INST_SUB,
    INST_MUL,
    INST_DIV,
    INST_MOD,
    INST_AND,
    INST_OR,
    INST_XOR,
    INST_NOT,
    INST_DUP,
    INST_PUSH,
    INST_POP,
    INST_CMP,
    INST_JMP,
    INST_JMP_IF,
    INST_CALL,
    INST_CALL_BUILTIN,
    INST_RET
};

struct Instruction
{
    InstType type;
    long larg;
    bool barg;
    const char *sarg;
    friend ostream & operator<< (ostream &out, const Instruction &s) {
        switch (s.type)
        {
            case INST_ADD: out << "add"; break;
            case INST_SUB: out << "sub"; break;
            case INST_MUL: out << "mul"; break;
            case INST_DIV: out << "div"; break;
            case INST_MOD: out << "mod"; break;
            case INST_AND: out << "and"; break;
            case INST_OR: out << "or"; break;
            case INST_XOR: out << "xor"; break;
            case INST_NOT: out << "not"; break;
            case INST_DUP: out << "dup"; break;
            case INST_PUSH: out << "push " << s.larg; break;
            case INST_POP: out << "pop"; break;
            case INST_CMP: out << "cmp"; break;
            case INST_JMP: out << "jmp " << s.larg; break;
            case INST_JMP_IF: out << "jmp_if " << s.larg; break;
            case INST_CALL: out << "call " << s.sarg; break;
            case INST_CALL_BUILTIN: out << "call " << s.sarg; break;
            case INST_RET: out << "ret";
            default: break;
        }
        return out;
    }
};

class StackVM
{
    long memory[8192];
    int sp, bp, hp, ip;

    void VMError(string msg)
    {
        cout << "VM Error: " << msg << endl;
        exit(1);
    }

#define VM_OP(name, op) \
    void name() { memory[sp] = op memory[sp]; }
#define VM_OP2(name, op)                                                \
    void name()                                                         \
    {                                                                   \
        ++sp;                                                           \
        memory[sp] = memory[sp] op memory[sp - 1]; /* left associate */ \
    }
    VM_OP2(add, +)
    VM_OP2(sub, -)
    VM_OP2(mul, *)
    VM_OP2(div, /)
    VM_OP2(mod, %)
    VM_OP2(bit_and, &)
    VM_OP2(bit_or, |)
    VM_OP2(bit_xor, ^)
    VM_OP(bit_not, ~)
#undef VM_OP
#undef VM_OP2
    void dup()
    {
        --sp;
        memory[sp] = memory[sp + 1];
    }
    void push(long val)
    {
        --sp;
        memory[sp] = val;
    }
    void pop() { ++sp; }
    void cmp()
    {
        ++sp;
        memory[sp] -= memory[sp - 1];
    }
    void jmp(long addr, bool abs)
    {
        if (abs)
            ip = addr;
        else
            ip += addr;
    }
    void jmp_if(long addr, bool abs)
    {
        ++sp;
        if (memory[sp - 1] != 0)
        {
            jmp(addr, abs);
        }
    }
    void call(string name)
    {
        char *base;
        long size;
        if (name == "write")
        {
            base = (char *)memory;
            size = memory[sp + 1];
            std::cout.write(base + memory[sp], size);
        }
        else if (name == "read")
        {
            base = (char *)memory;
            size = memory[sp + 1];
            std::cin.read(base + memory[sp], size);
        }
        else if (name == "alloc")
        {
            VMError("unsupport built-in function: " + name);
        }
        else if (name == "free")
        {
            VMError("unsupport built-in function: " + name);
        }
        else if (name == "getchar")
        {
            VMError("unsupport built-in function: " + name);
        }
        else if (name == "putchar")
        {
            VMError("unsupport built-in function: " + name);
        }
        else if (name == "exit")
        {
            cout << "Program exit with " << memory[sp] << endl;
            debug();
            exit(memory[sp]);
        }
        else
        {
            VMError("unknown built-in function: " + name);
        }
    }
    void call(long addr) { jmp(addr, true); }
    void ret()
    {
        ip = memory[sp];
        ++sp;
    }

    void debug()
    {
        long base = sp, length = 8192 - sp;
        printf("[");
        for (long i = 0; i < length; ++i)
        {
            printf("%ld ", memory[base + i]);
        }
        printf("]\n");
    }

   public:
    long add_global(const char *str)
    {
        char *base = (char *)memory;
        long addr, offset;
        addr = offset = hp * sizeof(long);
        while (*str != '\0')
        {
            base[offset++] = *(str++);
        }
        hp += (offset + sizeof(long) - 1) / sizeof(long);
        return addr;
    }

   public:
    StackVM() : sp(8192), bp(0), hp(0), ip(0) { fill_n(memory, 8192, 0l); }
    void run(const vector<Instruction> &code)
    {
        while (true)
        {
            Instruction i = code[ip];
            cout << ip << ": " << i << endl;
            switch (i.type)
            {
                case INST_ADD:
                    add();
                    break;
                case INST_SUB:
                    sub();
                    break;
                case INST_DUP:
                    dup();
                    break;
                case INST_PUSH:
                    push(i.larg);
                    break;
                case INST_POP:
                    pop();
                    break;
                case INST_JMP:
                    jmp(i.larg, false);
                    break;
                case INST_JMP_IF:
                    jmp_if(i.larg, false);
                    break;
                case INST_CMP:
                    cmp();
                    break;
                case INST_CALL:
                    call(i.larg);
                    break;
                case INST_CALL_BUILTIN:
                    call(string(i.sarg));
                    break;
                case INST_RET:
                    ret();
                    break;
                default:
                    VMError("Unknown instruction type");
                    break;
            }
            debug();
            ++ip;
        }
    }
};
