[ ] refactor: make FunctionContext an extention to FunctionDefinition
        Function Type = function signature
        Function Definition = {id, Function Type, hasBody}
        Function Context = {id, Function Type, Definition Context, Body IR}
[ ] refactor: Division of duty: Definition vs Type vs Function
[ ] refactor: asm: 封装指令生成

[ ] feature: Translation: 实现 shortcut in boolean expression
[ ] feature: IR: 处理 Function.cpp 中的 intention 机制，是否保留？
[ ] feature: Type: "const" concept
[ ] feature: Type: "assignable" concept
[ ] feature: Type: add sign support for integral: IsPositiveIntegral(), fix EXPR_CVT_?I2?I
[ ] feature: "copy" source or "view" source, how this affect all modules
[ ] feature: Ast: support 'typedef'
    [ ] abstract-declarator > type-name
    [ ] cast-expr context-sensitive case:
        option 1: forbid "(id)" for now
        option 2: introduce keyword "typename", use "(typename id)" for typedef id

[ ] bug: Translation 处理 参数寄存器 被修改 后再用来 读取参数。
[ ] bug: Translation if-else 会导致没有 ret 情况
[ ] bug: Definition x AstCompiler 处理 tag 的声明和定义
[ ] bug: save spill should not assume rcx, call use rax

[ ] unittest: Lexer
[ ] unittest: Ast
    [ ] branch: function-definition vs declaration
        void foo();
        void foo() {}
    [ ] branch: direct-abstract-declarator: abstract-declarator vs [parameter-type-list]
        int*, int(*)
        int[3], int([3])
        int(), int(())
        int(int), int((int))
    [ ] context sensitive branch: cast-expr
        typedef int i8; int a; (i8)+a;
        int i8; int a; (i8)+a;
[ ] unittest: Type

[ ] fdd: io
[ ] fdd: pointer
[ ] fdd: linked list
[ ] fdd: function call
