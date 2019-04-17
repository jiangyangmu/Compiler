GOAL

    Simulate everything in mind.

USAGE

    Represent data and computation.

CONSTRUCTION PROCESS

    1. Build operator tree
    2. Fill attributes

TODO

    [ ] LARGE_BLOB: support me
    [ ] COND, ELIST: support me

------------------------------------------------------------------------------------
VM Interface/Runtime

# program model

program     = data + code

# environment interface

1. Basic: executable file format
    * entry point, sections
    * exceptions
2. Interact with OS: calling convention, system call

------------------------------------------------------------------------------------
IR Operator Attributes

Type
    VOID

    BOOL
    SINT        // bool, char, int, enum
        i1,i2,i4,i8(ptrdiff)
    UINT        // unsigned int
        ui1,ui2,ui4,ui8
    FLT         // float, double
        f4, f8
    PTR
        p8

    BLOB        // struct, union
        bb1, bb2, bb4, bb8
    LARGE_BLOB  // struct, union
                // array - int a[3]; a[0]; sizeof(a); int (*p[3])(int); int aa[3][3];

    PROC        // function

Location (loc info & actual loc)
    REG
    REG_IND_MEM     // [rcx]
    SP_OFFSET_MEM   // [sp + const] current frame
    BP_OFFSET_MEM   // [bp + const] previous frame, arguments
    LABEL
    INLINE

------------------------------------------------------------------------------------
IR Rules

ASSERT: BOOL only exists as anon tmp value boolean expression
ASSERT: BOOL won't use dynamic adress

INFO: lvalue semantics (reuse location)
    REINTERP
INFO: reuse location
    IINC, IDEC, MCOPY
INFO: register node (output to register)
    PIND, CALL, REINTERP
ASSERT: reuse location always eliminate by MDUP immediately, except REINTERP

INFO: register spill for register node
    scan 1: compute spill zone
    scan 2: assign reg to spill zone
    convention: child i store to (spill zone base + 8 * i)

INFO: full input location
    = {REG, REG_IND_MEM, SP_OFFSET_MEM, BP_OFFSET_MEM, LABEL, INLINE}
INFO: full output location
    = {REG, REG_IND_MEM, SP_OFFSET_MEM}

------------------------------------------------------------------------------------
IR Operators

# 1. data source

    EXPR_ID
        ()
        => Type     { ?INT, FLT, PTR, BLOB, PROC }
        => Location { REG, SP_OFFSET_MEM, BP_OFFSET_MEM, LABEL }

    EXPR_CONSTANT
        ()
        => Type     { ?INT, FLT, PTR }
        => Location { LABEL, INLINE }

# 2. casting (size change x repr change)

    EXPR_CVT_SI2SI EXPR_CVT_SI2UI EXPR_CVT_UI2SI EXPR_CVT_UI2UI     - ?INT x ?INT
    EXPR_CVT_F2F                                                    - FLT x FLT
    EXPR_CVT_SI2F EXPR_CVT_F2SI                                     - SINT X FLT
    EXPR_CVT_?I2B EXPR_CVT_B2?I EXPR_CVT_F2B EXPR_CVT_B2F           - BOOL X {?INT, FLT}
        (in:BOOL|?INT|FLT, in:BOOL|?INT|FLT)
        => Type     { BOOL, ?INT, FLT }
        => Location { SP_OFFSET_MEM }

    EXPR_CVT_REINTERP           - reinterpret PTR target
        (in:PTR)
        => Type     { PTR }
        => Location in

    EXPR_CVT_DECAY              - create a temporary PTR
        (in:array|PROC)
        => Type     { PTR } to element/PROC
        => Location { SP_OFFSET_MEM }

# 3. calling

    EXPR_CALL
        (in:PTRToPROC|PROC, [in]*)
        => Type     { ?INT, FLT, PTR, BLOB }
        => Location { REG }

# 4. bool operations

    EXPR_BOOL_NOT
        (in:bool)
        => Type     { BOOL }
        => Location { SP_OFFSET_MEM }
    
    EXPR_BOOL_AND, EXPR_BOOL_OR
        (in:bool, in:bool)
        => Type     { BOOL }
        => Location { SP_OFFSET_MEM }

# 5. integer operations (size x signed-ness x arith)

    EXPR_SINEG EXPR_?INOT
        (in:?INT)
        => Type     { ?INT }
        => Location { SP_OFFSET_MEM }

    EXPR_?IINC EXPR_?IDEC
        (in:?INT)
        => Type     { ?INT }
        => Location in

    EXPR_?IADD EXPR_?ISUB
    EXPR_?IMUL EXPR_?IDIV EXPR_?IMOD
    EXPR_?IAND EXPR_?IOR EXPR_?IXOR
    EXPR_?ISHL EXPR_?ISHR
        (in:?INT, in:?INT)
        => Type     { ?INT }
        => Location { SP_OFFSET_MEM }

    EXPR_?IEQ EXPR_?INE EXPR_?ILT EXPR_?ILE EXPR_?IGE EXPR_?IGT
        (in:?INT, in:?INT)
        => Type     { BOOL }
        => Location { SP_OFFSET_MEM }

# 6. float operations (size x arith)

    EXPR_FNEG
        (in:FLT)
        => Type     { FLT }
        => Location { SP_OFFSET_MEM }

    EXPR_FADD EXPR_FSUB EXPR_FMUL EXPR_FDIV
        (in:FLT, in:FLT)
        => Type     { FLT }
        => Location { SP_OFFSET_MEM }

    EXPR_FEQ EXPR_FNE EXPR_FLT EXPR_FLE EXPR_FGE EXPR_FGT
        (in:FLT, in:FLT)
        => Type     { FLT }
        => Location { SP_OFFSET_MEM }

# 7. PTR operations (arith)

    EXPR_PADD?I EXPR_PSUB?I                                         - PTR x ?INT
        (in:PTR, in:?INT)
        => Type     { PTR }
        => Location { SP_OFFSET_MEM }

    EXPR_PDIFF                                                      - PTR x PTR
        (in:PTR, in:PTR)
        => Type     { ptrdiff }
        => Location { SP_OFFSET_MEM }

    EXPR_PEQ EXPR_PNE EXPR_PLT EXPR_PLE EXPR_PGE EXPR_PGT           - PTR x PTR
        (in:PTR, in:PTR)
        => Type     { BOOL }
        => Location { SP_OFFSET_MEM }

# 8. dynamic address

    EXPR_PIND
        (in:PTR)
        => Type     { ?INT, FLT, PTR, BLOB, PROC }
        => Location { REG_IND_MEM }

    EXPR_PNEW
        (in:*)
        => Type     PTR to { ?INT, FLT, PTR, BLOB, PROC }
        => Location { SP_OFFSET_MEM }

# 9. memory operations (size)

    EXPR_MCOPY
        (in:*, in:*)
        => Type     { ?INT, FLT, PTR, BLOB }
        => Location in

    EXPR_MDUP
        (in:*)
        => Type     { ?INT, FLT, PTR, BLOB }
        => Location { SP_OFFSET_MEM }

    EXPR_MADDSI EXPR_MADDUI
        (in:PTR, in:?INT)
        => Type     { ?INT, FLT, PTR, BLOB }
        => Location { SP_OFFSET_MEM }

# 10. control flow operations

# TODO: support

    EXPR_CONDITION
    EXPR_ELIST
