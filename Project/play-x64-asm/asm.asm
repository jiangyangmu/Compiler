PUBLIC IntegerFromASM
PUBLIC ASMAdd
PUBLIC ASMDiv
PUBLIC ASMMul
PUBLIC ASMSum
PUBLIC ASMFibonacci
PUBLIC ASMStrCmp
PUBLIC ASMCallCFun
PUBLIC ASMIfElse
PUBLIC ASMIndirectCall
PUBLIC ASMStringConst
PUBLIC FNeg
PUBLIC SSE_FInc
PUBLIC SSE_FAdd
PUBLIC SSE_FMul
PUBLIC SSE_FDiv
PUBLIC SSE_Feq
PUBLIC SSE_Flt
PUBLIC FtoI
PUBLIC ItoF
PUBLIC IndIndPtr
PUBLIC IndFloat
PUBLIC ASMfib
PUBLIC run
PUBLIC puti
EXTERN CAdd:PROC
EXTERN CInt:DWORD
EXTERN puts:PROC
EXTERN putchar:PROC

CONST   SEGMENT
__xmm@80000000800000008000000080000000 DB 00H, 00H, 00H, 080H, 00H, 00H, 00H
        DB      080H, 00H, 00H, 00H, 080H, 00H, 00H, 00H, 080H
CONST   ENDS
CONST   SEGMENT
__real@3f800000 DD 03f800000r             ; 1
ASMStringConst  DB 48H,65H,6cH,6cH,6fH,2cH,20H,77H,6fH,72H,6cH,64H,21H,00H ; Hello, world!
CONST   ENDS
CONST SEGMENT
$str0 DB '"hello, world!"',00H
CONST ENDS
_DATA SEGMENT
$sp0 DQ $str0
_DATA ENDS

_TEXT	SEGMENT

IntegerFromASM PROC
    mov rax, 42
    ret
IntegerFromASM ENDP

ASMAdd PROC
    mov rax, rcx
    add rax, rdx
    ret
ASMAdd ENDP

ASMDiv PROC
    mov rax, rcx
    mov rcx, rdx
    xor rdx, rdx
    idiv ecx
    ret
ASMDiv ENDP

ASMMul PROC
    mov rax, rcx
    imul rax, rdx
    ret
ASMMul ENDP

ASMSum PROC
    mov rax, 0
L0:
    cmp rcx, rdx
    jg L1
    add rax, rcx
    inc rcx
    jmp L0
L1:
    ret
ASMSum ENDP

ASMFibonacci PROC
    movsxd rax, ecx
    mov rcx, rax
    cmp rcx, 0
    jle L0
    cmp rcx, 2
    jle L1
    dec rcx
    push rcx
    call ASMFibonacci
    pop rcx
    push rax
    dec rcx
    call ASMFibonacci
    pop rcx
    add rax, rcx
    ret
L0:
    mov rax, 0
    ret
L1:
    mov rax, 1
    ret
ASMFibonacci ENDP

ASMStrCmp PROC
    mov rsi, rcx
    mov rdi, rdx
    mov rcx, r8
    repe cmpsb
    je L0
    jg L1
    mov rax, -1
    ret
L0:
    mov rax, 0
    ret
L1:
    mov rax, 1
    ret
ASMStrCmp ENDP

ASMCallCFun PROC
    push rbp
    mov rbp, rsp
    sub rsp, 32

    mov rcx, 13
    mov edx, DWORD PTR CInt
    movsxd rdx, edx
    call CAdd

    add rsp, 32
    pop rbp
    ret
ASMCallCFun ENDP

ASMIfElse PROC
    cmp rcx, 0
    je @L0
    mov rax, rdx
    jmp L1
@L0:
    mov rax, r8
L1:
    ret
ASMIfElse ENDP

FNeg PROC
    xorps xmm0, DWORD PTR __xmm@80000000800000008000000080000000
    ret
FNeg ENDP

SSE_FInc PROC
    movss DWORD PTR [rsp+8], xmm0
    movss xmm0, DWORD PTR [rsp+8]
    addss xmm0, DWORD PTR __real@3f800000
    ret
SSE_FInc ENDP

SSE_FAdd PROC
    addss xmm0, xmm1
    ret
SSE_FAdd ENDP

SSE_FMul PROC
    mulss xmm0, xmm1
    ret
SSE_FMul ENDP

SSE_FDiv PROC
    divss xmm0, xmm1
    ret
SSE_FDiv ENDP

SSE_Feq PROC
    cmpeqss xmm0, xmm1
    movss DWORD PTR [rsp+8], xmm0
    mov eax, DWORD PTR [rsp+8]
    ret
SSE_Feq ENDP

SSE_Flt PROC
    cmpltss xmm0, xmm1
    movss DWORD PTR [rsp+8], xmm0
    mov eax, DWORD PTR [rsp+8]
    ret
SSE_Flt ENDP

FtoI PROC
    cvtss2si eax, xmm0
    ret
FtoI ENDP

ItoF PROC
    cvtsi2ss xmm0, ecx
    ret
ItoF ENDP

IndIndPtr PROC
    mov rcx, QWORD PTR [rcx]
    mov rax, QWORD PTR [rcx]
    ret
IndIndPtr ENDP

IndFloat PROC
    movss xmm0, DWORD PTR [rcx]
    ret
IndFloat ENDP

ASMIndirectCall PROC
    mov r11, rcx
    mov rcx, rdx
    mov rdx, r8
    call r11
    ret
ASMIndirectCall ENDP

ASMfib PROC
    push rbp
	mov rbp, rsp
	push rsi
	push rdi
	sub rsp, 0000000000000030h
	mov eax, ecx
	mov DWORD PTR [rsp + 44], eax
	mov eax, DWORD PTR [rsp + 44]
	mov DWORD PTR [rsp + 36], eax
	mov eax, DWORD PTR [rsp + 44]
	mov edx, 0000000000000000h
	cmp eax, edx
	mov al, 00000000h
	mov cx, 00000001h
	cmovle ax, cx
	mov BYTE PTR [rsp + 39], al
	cmp al, 0
	je @L0
	; copy return value
	mov eax, 0000000000000000h
	add rsp, 0000000000000030h
	pop rdi
	pop rsi
	pop rbp
	ret
	jmp @L1
	@L0:
	mov eax, DWORD PTR [rsp + 44]
	mov edx, 0000000000000002h
	cmp eax, edx
	mov al, 00000000h
	mov cx, 00000001h
	cmovle ax, cx
	mov BYTE PTR [rsp + 39], al
	cmp al, 0
	je @L2
	; copy return value
	mov eax, 0000000000000001h
	add rsp, 0000000000000030h
	pop rdi
	pop rsi
	pop rbp
	ret
	jmp @L3
	@L2:
	mov eax, DWORD PTR [rsp + 44]
	sub eax, 0000000000000001h
	mov DWORD PTR [rsp + 36], eax
	mov eax, DWORD PTR [rsp + 36]
	mov ecx, eax
	call ASMfib
	mov eax, eax
	mov eax, eax
	mov DWORD PTR [rsp + 32], eax
	mov eax, DWORD PTR [rsp + 44]
	sub eax, 0000000000000002h
	mov DWORD PTR [rsp + 28], eax
	mov eax, DWORD PTR [rsp + 28]
	mov ecx, eax
	call ASMfib
	mov eax, eax
	mov eax, eax
	mov DWORD PTR [rsp + 24], eax
	mov eax, DWORD PTR [rsp + 32]
	add eax, DWORD PTR [rsp + 24]
	mov DWORD PTR [rsp + 20], eax
	; copy return value
	mov eax, DWORD PTR [rsp + 20]
	add rsp, 0000000000000030h
	pop rdi
	pop rsi
	pop rbp
	ret
	@L3:
	@L1:
ASMfib ENDP

puti PROC
    ; return address:        [rbp + 8]
    ; non-volatile register: [rsp + 40, rbp + 8)
    ; local zone:            [rsp + 32, rsp + 40)
    ; temp  zone:            [rsp + 24, rsp + 32)
    ; spill zone:            [rsp + 8, rsp + 24)
    ; call  zone:            [rsp + 0, rsp + 8)
    ; m: [rsp + 32]
    ; d: [rsp + 36]
    push rbp
    mov rbp, rsp
    mov DWORD PTR [rbp + 16], ecx ; copy register argument
    push rsi
    push rdi
    sub rsp, 00000000000000028h
    ; mdup
    ; mcopy
    ; id
    ; constant
    mov eax, 0000000000000000ah
    mov DWORD PTR [rsp + 32], eax
    mov eax, DWORD PTR [rsp + 32]
    mov DWORD PTR [rsp + 28], eax
    ; igt
    ; id
    ; constant
    mov eax, DWORD PTR [rbp + 16]
    mov edx, 00000000000000000h
    cmp eax, edx
    mov al, 000000000h
    mov cx, 000000001h
    cmovg ax, cx
    mov BYTE PTR [rsp + 31], al
    cmp al, 0
    je @L0
    ; mdup
    ; mcopy
    ; id
    ; imod
    ; id
    ; id
    mov rdx, 000000000h
    mov eax, DWORD PTR [rbp + 16]
    div DWORD PTR [rsp + 32]
    mov DWORD PTR [rsp + 28], edx
    mov eax, DWORD PTR [rsp + 28]
    mov DWORD PTR [rsp + 36], eax
    mov eax, DWORD PTR [rsp + 36]
    mov DWORD PTR [rsp + 24], eax
    ; mdup
    ; mcopy
    ; id
    ; idiv
    ; id
    ; id
    mov rdx, 000000000h
    mov eax, DWORD PTR [rbp + 16]
    div DWORD PTR [rsp + 32]
    mov DWORD PTR [rsp + 28], eax
    mov eax, DWORD PTR [rsp + 28]
    mov DWORD PTR [rbp + 16], eax
    mov eax, DWORD PTR [rbp + 16]
    mov DWORD PTR [rsp + 24], eax
    ; mdup
    ; call
    ; id
    ; id
    mov eax, DWORD PTR [rbp + 16]
    mov ecx, eax
    call puti
    mov eax, eax
    mov QWORD PTR [rsp + 16], rcx ; save spill
    mov eax, eax
    mov DWORD PTR [rsp + 28], eax
    ; mdup
    ; call
    ; id
    ; iadd
    ; id
    ; constant
    mov eax, DWORD PTR [rsp + 36]
    add eax, 00000000000000030h
    mov DWORD PTR [rsp + 28], eax
    mov eax, DWORD PTR [rsp + 28]
    mov ecx, eax
    call putchar
    mov eax, eax
    mov QWORD PTR [rsp + 16], rcx ; save spill
    mov eax, eax
    mov DWORD PTR [rsp + 24], eax
    @L0:
    ; constant
    ; copy return value
    mov eax, 00000000000000000h
    add rsp, 00000000000000028h
    pop rdi
    pop rsi
    pop rbp
    ret
puti ENDP

run PROC
    ; return address:        [rbp + 8]
    ; non-volatile register: [rsp + 80, rbp + 8)
    ; local zone:            [rsp + 56, rsp + 80)
    ; temp  zone:            [rsp + 20, rsp + 56)
    ; spill zone:            [rsp + 0, rsp + 16)
    ; call  zone:            [rsp + 0, rsp + 0)
    ; pp: [rsp + 56]
    ; p: [rsp + 64]
    ; i: [rsp + 76]
    push rbp
    mov rbp, rsp
    push rsi
    push rdi
    sub rsp, 00000000000000050h
    ; mdup
    ; mcopy
    ; id
    ; pnew
    ; id
    lea rax, QWORD PTR [rsp + 76]
    mov QWORD PTR [rsp + 48], rax
    mov rax, QWORD PTR [rsp + 48]
    mov QWORD PTR [rsp + 64], rax
    mov rax, QWORD PTR [rsp + 64]
    mov QWORD PTR [rsp + 40], rax
    ; mdup
    ; mcopy
    ; id
    ; pnew
    ; id
    lea rax, QWORD PTR [rsp + 64]
    mov QWORD PTR [rsp + 48], rax
    mov rax, QWORD PTR [rsp + 48]
    mov QWORD PTR [rsp + 56], rax
    mov rax, QWORD PTR [rsp + 56]
    mov QWORD PTR [rsp + 40], rax
    ; mdup
    ; mcopy
    ; id
    ; constant
    mov eax, 00000000000000001h
    mov DWORD PTR [rsp + 76], eax
    mov eax, DWORD PTR [rsp + 76]
    mov DWORD PTR [rsp + 52], eax
    ; mdup
    ; mcopy
    ; pind
    ; id
    mov rcx, QWORD PTR [rsp + 64]
    mov QWORD PTR [rsp + 8], rcx ; save spill
    ; constant
    mov eax, 00000000000000002h
    mov rcx, QWORD PTR [rsp + 8] ; load spill
    mov DWORD PTR [rcx], eax
    mov eax, DWORD PTR [rcx]
    mov DWORD PTR [rsp + 52], eax
    ; mdup
    ; mcopy
    ; pind
    ; pind
    ; id
    mov rcx, QWORD PTR [rsp + 56]
    mov QWORD PTR [rsp + 8], rcx ; save spill
    mov rcx, QWORD PTR [rcx]
    mov QWORD PTR [rsp + 8], rcx ; save spill
    ; constant
    mov eax, 00000000000000003h
    mov rcx, QWORD PTR [rsp + 8] ; load spill
    mov DWORD PTR [rcx], eax
    mov eax, DWORD PTR [rcx]
    mov DWORD PTR [rsp + 52], eax
    ; mdup
    ; mcopy
    ; id
    ; psubsi
    ; id
    ; cvt_si2si
    ; constant
    mov eax, 00000000000000001h
    movsxd rax, eax
    mov QWORD PTR [rsp + 48], rax
    mov rax, QWORD PTR [rsp + 48]
    mov rcx, 0fffffffffffffff8h
    imul rcx
    add rax, QWORD PTR [rsp + 56]
    mov QWORD PTR [rsp + 40], rax
    mov rax, QWORD PTR [rsp + 40]
    mov QWORD PTR [rsp + 56], rax
    mov rax, QWORD PTR [rsp + 56]
    mov QWORD PTR [rsp + 32], rax
    ; mdup
    ; mcopy
    ; id
    ; psubsi
    ; id
    ; cvt_si2si
    ; constant
    mov eax, 00000000000000001h
    movsxd rax, eax
    mov QWORD PTR [rsp + 48], rax
    mov rax, QWORD PTR [rsp + 48]
    mov rcx, 0fffffffffffffffch
    imul rcx
    add rax, QWORD PTR [rsp + 64]
    mov QWORD PTR [rsp + 40], rax
    mov rax, QWORD PTR [rsp + 40]
    mov QWORD PTR [rsp + 64], rax
    mov rax, QWORD PTR [rsp + 64]
    mov QWORD PTR [rsp + 32], rax
    ; mdup
    ; mcopy
    ; pind
    ; paddsi
    ; pind
    ; paddsi
    ; id
    ; cvt_si2si
    ; constant
    mov eax, 00000000000000001h
    movsxd rax, eax
    mov QWORD PTR [rsp + 48], rax
    mov rax, QWORD PTR [rsp + 48]
    mov rcx, 00000000000000008h
    imul rcx
    add rax, QWORD PTR [rsp + 56]
    mov QWORD PTR [rsp + 40], rax
    mov rcx, QWORD PTR [rsp + 40]
    mov QWORD PTR [rsp + 8], rcx ; save spill
    ; cvt_si2si
    ; constant
    mov eax, 00000000000000001h
    movsxd rax, eax
    mov QWORD PTR [rsp + 32], rax
    mov rax, QWORD PTR [rsp + 32]
    mov rcx, 00000000000000004h
    imul rcx
    mov rcx, QWORD PTR [rsp + 8] ; load spill
    add rax, QWORD PTR [rcx]
    mov QWORD PTR [rsp + 24], rax
    mov rcx, QWORD PTR [rsp + 24]
    mov QWORD PTR [rsp + 8], rcx ; save spill
    ; constant
    mov eax, 00000000000000004h
    mov rcx, QWORD PTR [rsp + 8] ; load spill
    mov DWORD PTR [rcx], eax
    mov eax, DWORD PTR [rcx]
    mov DWORD PTR [rsp + 20], eax
    ; id
    ; copy return value
    mov eax, DWORD PTR [rsp + 76]
    add rsp, 00000000000000050h
    pop rdi
    pop rsi
    pop rbp
    ret
run ENDP

_TEXT	ENDS

END
