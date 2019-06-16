> debug
    1. forget return ?
    2. multiple pointer indirection in an expression ?
        a->b = c->d;
        f(*a, *b);
    3. bad free ?
> debug: dump lib symbols
    dumpbin /SYMBOLS legacy_stdio_definitions.lib msvcrtd.lib vcruntimed.lib ucrtd.lib | findstr fopen
> debug compiler
    1. stack aligned?
