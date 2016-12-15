#include "svm.h"

#define _inst(name, p1, p2, p3) {name, p1, p2, p3}

#define _push(val) _inst(INST_PUSH, val, false, nullptr)
#define _pop() _inst(INST_POP, 0, false, nullptr)
#define _add() _inst(INST_ADD, 0, false, nullptr)
#define _cmp() _inst(INST_CMP, 0, false, nullptr)
#define _call(name) _inst(INST_CALL, 0, false, name)
#define _callb(name) _inst(INST_CALL_BUILTIN, 0, false, name)
#define _jmp(addr) _inst(INST_JMP, addr, false, nullptr)
#define _jmp_if(addr) _inst(INST_JMP_IF, addr, false, nullptr)

#define _str(s) vm.add_string(s)

int main(int argc, char * argv[])
{
    StackVM vm;
    vector<Instruction> code {
        _push(1),
        _push(2),
        _cmp(),
        _jmp_if(3),
        _push(4),
        _push(_str("1=2\n")),
        _jmp(2),
        _push(5),
        _push(_str("1!=2\n")),
        _callb("write"),
        _pop(),
        _pop(),
        _callb("exit")
        // {INST_PUSH, 1, false, nullptr},
        // {INST_PUSH, 2, false, nullptr},
        // {INST_ADD, 0, false, nullptr},
        // {INST_PUSH, 15, false, nullptr},
        // {INST_PUSH, vm.add_global("Hello, world!\n"), false, nullptr},
        // {INST_CALL_BUILTIN, 0, false, "write"},
        // {INST_POP, 0, false, nullptr},
        // {INST_POP, 0, false, nullptr},
        // {INST_CALL_BUILTIN, 0, false, "exit"}
    };

    vm.run(code);
    return 0;
}
