Type System
===

"Represent C type system, that's it."

TODO
    * check Environment impact
    * support incomplete declaration/definition
    * support constrains check
    * support bit-field

DONE
    1. design repr API
    2. design build API


+ The elements of Type System

    memory layout:
        object size
        object alignment

    encoding:
        signed integer
        unsigned integer
        ieee-754 floating point
        memory address

    operations:
        for object: get addr, get size
        for scalar:
        for arithmetic:
        for integral:
        for floating:
        
    calling protocol:
        return type
        parameter type & location
        is var list

    [special]
        array   - an array of same type objects using continuous memory
        struct  - a bundle of objects
        union   - a bunch of objects sharing the same memory block
        enum    - integer constants with name
        func    - complex code object

+ Internals

// Repr

    MemoryLayout
        <fields>
            size : int
            align : int

    Encoding : enum

    Operation
        <fields>
            return_type : Type
            op_list : Type[]
        <methods>

    Property : enum
        CONST, VOLATILE

    Type
        <fields>
            // memory layout info
            // encoding info
            // operation info
            // properties info

    VoidType : Type
    CharType : Type
    IntType : Type
        <fields>
            sign : bool
    FloatType : Type
    PointerType : Type
        <fields>
            referenced_type : Type*
    ArrayType : Type
        <fields>
            element_count : int
            element_type : Type*
    StructType : Type
        <fields>
            members : {Name, Type*, Offset}[]
    UnionType : Type
        <fields>
            members : {Name, Type*, Offset}[]
    EnumType : Type
    FuncType : Type
        <fields>
            return_type : Type*
            parameter_list : Type[]
            is_varlist : bool

// State-based Builder

    SpecifierBits
        <fields>
            storage_bits : int
            qualifier_bits : int
            specifier_bits : int

    ParamList
        <fields>
            parameter_list : Type[]
            is_varlist : bool

    // Type specifier -> Type*
    ...struct...
    ...enum...
    ...typedef...

    // Declarator -> Type*
    PointerTo(Type *referenced_type, int pointer_qualifiers) -> Type*
    ArrayOf(Type *element_type, int size) -> Type*
    ArrayOf(Type *element_type) -> Type*
    FunctionReturns(Type *return_type, ParamList plist) -> Type*

+ API

    // build_qualifier  - "const", "volatile"

    // build_struct_member_list() -> name[],type[]
    // build_function_param_list() -> name[],type[],bool

    // build_void       - "void"
    // build_char       - "char"
    // build_int        - "char","int","short","long","signed","unsigned"
    // build_float      - "float","double","long"

    // build_enum       - NA.
    // build_struct     - In member declarations, collect "types" and "names"
    // build_union      - In member declarations, collect "types" and "names"

    // build_pointer    - In pointer, collect "referenced type", "const"
    // build_array      - In specifiers, collect "element type"
                          In (abstract-)declarators, collect "element type"
                          In (abstract-)declarators, collect "size"
    // build_funciton   - In function-definition, collect "return type", "parameter list", "var list info"
                          Or
                          In declaration, collect "return type", "parameter list", "var list info"
