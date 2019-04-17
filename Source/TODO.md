

[ ] bug: Translation 处理 参数寄存器 被修改 后再用来 读取参数。
[ ] bug: Translation if-else 会导致没有 ret 情况
[ ] bug: Definition x AstCompiler 处理 tag 的声明和定义
[ ] test: cc-integration-test 不能直接 debug 编译器
[ ] feature: Translation: 实现 shortcut in boolean expression
[ ] feature: IR: 处理 Function.cpp 中的 intention 机制，是否保留？
[ ] refactor: make FunctionContext an extention to FunctionDefinition
        Function Type = function signature
        Function Definition = {id, Function Type, hasBody}
        Function Context = {id, Function Type, Definition Context, Body IR}
[ ] feature: Type: "const" concept
[ ] feature: Type: "assignable" concept
[ ] refactor: Division of duty: Definition vs Type vs Function
[ ] feature: Translation: 利用 intention 实现 pointer indirection
[ ] refactor: asm: 封装指令生成
[ ] feature: Type: add sign support for integral: IsPositiveIntegral(), fix EXPR_CVT_?I2?I

