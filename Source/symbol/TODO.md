Symbol and Environment
===

"Everything about name and reference."


+ The elements of Symbol System
 
    symbol = { name, scope, linkage, namespace }

    name:
        object(arithmatic, pointer, array, func)
        struct/union/enum tag
        struct/union-member
        enum-constant
        function-param
        typedef name
        label name
        (or) macro name, macro parameter

    scope: visibility of identifier
        function - label only
            region: all function
        file - global decls, function returned types
            region: <start> -> file end
        block - in { ...here.. } or func( ...here... ) {...}
            region: <start> -> block end
        function prototype - in func( ...here... );
            region: decl end;
        <start>:
            tag/enum-const -> after itself
            id -> after its declarator
        * outer layer name hidden

    linkage: treat identifiers in different scopes or in the same scope as the same object or function
        external - the same in whole program
        internal - the same in one translation unit
        none - denotes unique entities (typedef names, func param names, local variables)
        rules
            # here [scope] can't be function
            ---------------------------------------------------------
             [object type]  [storage]  [scope]               linkage
            1   func/obj    'static'    file                 internal
            2   func/obj    'extern'     *              dup-file or external
            3    func          --       file                    2
            4     obj          --       file                 external
            5   ^func/obj      *         *                     none
            6      *           *     func-proto                none
            7     obj       ^'extern'   block                  none
            ---------------------------------------------------------
        * undefined: within a translation unit, the same identifier appears with both internal and external linkage.

    namespace: separate name spaces for various categories of identifiers
        label
        tag
        struct/union members
        id (object, enum-constant, typedef-name)

+ API

    Scope : enum
    Namespace: enum
    Linkage : enum

    // compute_scope - from syntax
    // compute_namespace - from syntax

    compute_linkage(ObjectType ot, Token sq, Scope sc) -> Linkage

