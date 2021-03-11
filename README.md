# C Compiler

A C89 to x64 assembly compiler.

Yet another project to learn compilation and programming techniques.

# Supported Features

* Preprocessing: `#include` directive
* Types: primitives, enum, struct, pointer, function [test code](Test/Program.c)
* Statements: componend, if-else, while, for, break, continue, switch-case, return. [test code](Test/Program.c)
* Expressions: invocation, type casting, boolean/integer/bit/float/pointer operations, condition, comma, scalar/struct assignment. [test code](Test/Program.c)
* Recursive function. [test code](Test/fibonacci.c)

# Build

Open Visual Studio solution [Project/cc.sln](Project/cc.sln), build project `cc` with `Debug` configuration and `x64` platform. This should generate `Build/cc.exe`.

# Usage

```
$ cc.exe <source-file>
```

# Example

```
$ type hello_world.c
int printf(const char * fmt, ...);
int main()
{
        printf("hello world.\n");
}

$ cc.exe hello_world.c
...

$ type hello_world.asm
PUBLIC main
EXTERN printf:PROC
CONST SEGMENT
$str0 DB 68H,65H,6cH,6cH,6fH,20H,77H,6fH,72H,6cH,64H,2eH,0aH,00H ; "hello world.\n"
ALIGN 16
$flt_80000000 DD 080000000r,080000000r,080000000r,080000000r ; -0.000000
$flt_00000000 DD 000000000r,000000000r,000000000r,000000000r ; 0.000000
CONST ENDS
_DATA SEGMENT
$sp0 DQ $str0
_DATA ENDS
_TEXT SEGMENT
main PROC
; return address:        [rbp + 8]
; non-volatile register: [rsp + 48, rbp + 8)
; local zone:            [rsp + 48, rsp + 48)
; temp  zone:            [rsp + 36, rsp + 48)
; spill zone:            [rsp + 16, rsp + 32)
; call  zone:            [rsp + 0, rsp + 8)
push rbp
mov rbp, rsp
push rsi
push rdi
sub rsp, 00000000000000030h
; mdup
; call
; pnew
; data
lea rax, QWORD PTR printf
mov QWORD PTR [rsp + 40], rax
; data
mov rax, QWORD PTR $sp0
mov rcx, rax
call printf
mov eax, eax
mov QWORD PTR [rsp + 24], rcx ; save spill
mov eax, eax
mov DWORD PTR [rsp + 36], eax
main ENDP
_TEXT ENDS
END

$ ml64 hello_world.asm hello_world.obj /link /OUT:hello_world.exe legacy_stdio_definitions.lib msvcrtd.lib
...

$ hello_world.exe
hello world.
```

Check this [page](https://docs.microsoft.com/en-us/cpp/build/how-to-enable-a-64-bit-visual-cpp-toolset-on-the-command-line?view=msvc-160) for error `'ml64' is not recognized as an internal or external command`.

# Project Structre

* `Build/` - cc.exe
* `Source/` - compiler source code
* `Test/` - testing C89 programs
