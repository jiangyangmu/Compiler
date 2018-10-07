SDT based on EBNF parser
===
































































1. Type, Symbol, Environment

Every symbol belongs to one environment.

A symbol can refer to...
    an object   (scalar, function, enum-constant, etc.),
    a type      (struct definition, typedef-ed type, etc.),
    a label.

2. Construct a type.


Know the types...

[Type]

    void
        semantics:
            void is an incomplete type that cannot be completed.
            void means "nothing", so it makes no sense to...
                use the value of void expression in any way, and
                allow implicit/explicit conversion from void. (Note: convert to void is ok)
            any expression pass to a context that requires void expression...
                it gets evaluated for side effect, but
                its value is discarded.
            "0" or "(void *)0" is a null pointer constant, it is...
                guaranteed to compare unequal to a pointer to any object or function.
        usage:
            1. no parameter - int foo(void);
            2. return nothing - void bar(int);
            3. null pointer constant - ((void *)0)
            4. disable warning for unused return value - (void)foo();
        constrains:
            1. pointer to void shall have the same repr and alignment requirement as pointer to char.
            2. see "pointer" for cast involving pointer to void.

    char
        semantics:
            large enough to store any member of the basic execution character set.
            signed-ness:
                If a member of the required source character set enumerated in 2.2.1 is stored in a char object, its value is guaranteed to be positive.
                If other quantities are stored in a char object, the behavior is implementation-defined: the values are treated as either signed or nonnegative integers.
        repr,align: the same as "unsigned char"

    integral:
        unsigned char, signed char, (signed) short (int), unsigned short (int),
        (signed) (int), unsigned (int), (signed) long (int), unsigned long (int),
        repr,align: binary number system, 1/2/4/8

    floating: float, double, long double
        repr,align: ieee-754, 4/8/10

    pointer
        semantics:
            an address.
        parameters:
            referenced type
        constrains:
            1. convert pointer to void to pointer to type A, and back again, is ok.
            2. convert pointer to type A to pointer to void, and back again, is ok.

    array
        semantics:
            an array of objects on continuous memory.
        parameters:
            element type
            size
        constrains:
            element type must be complete.
        completeness:
            size is known.

    enum
        semantics:
            named integer constant.
        repr,align:
            the same as integral
        completeness:
            finish definition.
        impact to environment:
            1. add new enum tag to current environment.
            2. add new enum type to current environment if it's definition.
            3. enum constants introduce new identifiers into current environment.

    struct/union
        semantics:
            a bundle of heterogeneous objects.
        parameters:
            member types and names
        constrains:
            members must have complete type.
        completeness:
            finish definition.
        usage:
        impact to environment:
            1. add new struct tag to current environment.
            2. add new struct type to current environment if it's definition.

    function
        semantics:
        parameters:
            return type
            name
            parameter types and names
            var list
        constrains:
            return type is not array or function.
            in definition, return type and param types must be complete.
            array parameter decays to pointer.
            tag declaration in parameter can trigger namespace conflict with file scope tag.
            tag declaration in parameter is only visible in function definition, which makes the function un-callable.
        usage:
        impact to environment:
            1. add a function object to current environment (which is always global environment).

[Categories of Type]

    basic = char/signed integer/unsigned integer/floating
    character = char/signed char/unsigned char
    enumerated = enum
    derived = array/struct/union/function/pointer
    derived declarator = array/function/pointer
        array of T, function returning T, pointer to T
    aggregate = array/struct
    integral = char/signed integer/unsigned integer/enumerated
    arithmetic = integral/floating
    scalar = arithmetic/pointer

[Properties of Type]

    incomplete = size unknown (...)
        array no size
        struct/union tag no content
    qualified = top type is specified with a type qualifier
        unqualified type = top type remove type qualifiers
        'const'/'volatile'
    lvalue = an expression (with an object type or an incomplete
                type other than void) that designates an object.
        create:
            # identifer (in primary-expr)
            # identifer (in function-definition)
            # string-literal
                with type 'array of char'
            # postfix-expr '->' identifer
            # '*' cast-expr (in unary-expr)
            # postfix-expr '[' expr ']'
                equal to: '*' (postfix-expr '+' (expr))
        transfer:
            # '(' expr ')'
                e.isLvalue() = expr.isLvalue()
            # postfix-expr '.' identifer
                e.isLvalue() = postfix-expr.isLvalue()

[Operations of Type]

    > Scalar: cast, logical-op, inc/dec
        pointer: sub, array-subscripting [], func-call (), member-access ->, indirection *, rel-op, eq-op, simple-assign
        pointer & Integral: add/sub
        > Arithmetic: sign-op, add/sub, mul, div, rel-op, eq-op, simple-assign
            > Integral: bitwise-op, mod, repr-pure-binary-number-system
                char
                unsigned char
                signed char
                (signed) short (int)
                unsigned short (int)
                (signed) (int)
                unsigned (int)
                (signed) long (int)
                unsigned long (int)
                enum
            > Floating: repr-unspecified
                float
                double
                long double

    array: can only appear at context { &T, sizeof T, T = "..." }

    struct/union: member-access ., simple-assign

    function: can only appear at context { &T, sizeof T }

    > Object: get-address &

    
[Properties between Types]

    compatible:
        Two types are compatible if their types are the same.

        Additional rules for determining whether two types are compatible are
        described in 3.5.2 for type specifiers, in 3.5.3 for type qualifiers,
        and in 3.5.4 for declarators.

        Two structure, union, or enumeration types declared in separate
        translation units are compatible if they have the same number of
        members, the same member names, and compatible member types;
        for two structures, the members shall be in the same order;
        for two enumerations, the members shall have the same values.

    composite:
        A type is composite if it's constructed from two types that are compatible.

        If one type is an array of known size, the composite type is an array
        of that size.
        If only one type is a function type with a parameter type list (a function
        prototype), the composite type is a function prototype with the parameter
        type list.
        If both types have parameter type lists, the type of each parameter in
        the composite parameter type list is the composite type of the
        corresponding parameters.
        These rules apply recursively to the types from which the two types are derived.

        For an identifier with external or internal linkage declared in the
        same scope as another declaration for that identifier, the type of the
        identifier becomes the composite type. <used by linkage merge>

    * undefined: All declarations that refer to the same object or function
                 shall have compatible type; otherwise the behavior is
                 undefined.

[Conversion between Types]






















Then, design construct process...








3. Construct a symbol.

4. Organize an environment tree.
