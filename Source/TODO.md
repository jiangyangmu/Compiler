IR
[x] AstCompiler.cpp 从 Ast 构建 DefinitionContext, FunctionContext, ...
[x] Function.cpp 完成 表达式树 的 type checking
[x] Function.cpp 完成 表达式树 的 location allocation (stack, register, ...)
[x] Function.cpp 计算 id expression 的 location
[ ] 处理 Function.cpp 中的 intention 机制，是否保留？

Translation
[x] Translation.cpp x64 PROC non-volatile 寄存器 保存/恢复
[x] Translation.cpp 实现 function calling 的汇编翻译
[ ] Translation.cpp 实现 shortcut in boolean expression


[ ] Translation.cpp 实现 pointer indirection 的汇编翻译
