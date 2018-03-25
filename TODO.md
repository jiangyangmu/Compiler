JCC TODO list
===

EBNF Parser

<!-- + Add BNF node. -->
<!-- + Add BNF and,or production. -->
<!-- + Add BNF epsilon production. -->
<!-- + Add first,follow computation. -->
<!-- + Add AST node. -->
<!-- + Add BNF embedded code node. -->
+ ADD unit-testing.
+ Add parse support.
    * what context should be provided in CODE node { ... } ?
        current node properties: SET_PROP(value), GET_PROP()
        direct child properties: SET_CHILD_PROP(i, value), GET_CHILD_PROP(i)

Debugging

+ logging library.

Code Formatting

+ A universal clang-format config file.