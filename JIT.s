; method std.kernel#Runtime.__srt_init_

; starting save state
push rbp
mov rbp, rsp
push r12
push r13
push r14
push r15
push r8
push r9
sub rsp, 160
mov qword [rbp-56], rcx
xor r12, r12
xor r13, r13
mov qword [rbp-72], 0
mov qword [rbp-64], 0
mov qword [rbp-80], 0
mov r14, qword [rcx+8]
mov r15, qword [rcx]
mov rcx, qword [rcx+16]
mov rcx, qword [rcx]
mov qword [rbp-64], rcx
mov rcx, qword [rcx]
test rcx, rcx
jne code_start
jmp init_addr_tbl
code_start:
; instr 0
L0:
L1:
movsd xmm0, [L20]
mov rcx, r14
movsd qword [rcx], xmm0
nop
nop
nop
nop
nop
; instr 2
L2:
movsd xmm0, [L21]
mov rcx, r15
mov rdx, qword [rcx+16]
lea rdx, [rdx+16]
mov qword [rcx+16], rdx
mov rcx, rdx
movsd qword [rcx], xmm0
nop
nop
nop
nop
nop
; instr 3
L3:
mov rcx, r15
mov rcx, qword [rcx+16]
movsd xmm1, qword [rcx]
movsd xmm0, [L22]
addsd xmm0, xmm1
movsd qword [rcx], xmm0
nop
nop
nop
nop
nop
; instr 4
L4:
mov rcx, r15
mov rcx, qword [rcx+16]
movsd xmm0, qword [rcx]
mov rcx, r14
add rcx, 24
movsd qword [rcx], xmm0
nop
nop
nop
nop
nop
; instr 5
L5:
mov rcx, 3
call 4664200
nop
nop
nop
nop
nop
; instr 6
L6:
mov ecx, 159
call 4665642
lea r12, [L23]
mov r13, 6
mov rcx, r15
mov ecx, qword [rcx+80]
cmp ecx, 0
jne .thread_check
L23:
movzx ecx, [6242096]
cmp ecx, 0
je L24
mov r13, 6
jmp func_end
L24:
nop
nop
nop
nop
nop
; instr 7
L7:
nop
nop
nop
nop
nop
nop
; instr 8
L8:
nop
nop
nop
nop
nop
nop
; instr 9
L9:
nop
nop
nop
nop
nop
nop
; instr 10
L10:
nop
nop
nop
nop
nop
nop
; instr 11
L11:
L12:
pxor xmm0, xmm0
mov rcx, r14
add rcx, 24
movsd qword [rcx], xmm0
nop
nop
nop
nop
nop
; instr 13
L13:
mov rcx, r15
mov rcx, qword [rcx+24]
mov rax, rcx
mov rcx, r14
add rcx, 24
movsd xmm0, qword [rcx]
mov rcx, rax
movsd qword [rcx], xmm0
nop
nop
nop
nop
nop
; instr 14
L14:
mov r13, 14
cmp r13, -1
je L25
mov rcx, r15
mov rax, qword [rcx+64]
imul r13, 8
add rax, r13
mov qword [rcx+72], rax
L25:
mov r13, 0
jmp func_end
nop
nop
nop
nop
nop
mov r13, 14
cmp r13, -1
je L26
mov rcx, r15
mov rax, qword [rcx+64]
imul r13, 8
add rax, r13
mov qword [rcx+72], rax
L26:
mov r13, 0
jmp func_end
init_addr_tbl:
nop
; setting label values
mov rax, qword [rbp-64]
lea rcx, [L0]
mov qword [rax], rcx
add rax, 8
lea rcx, [L1]
mov qword [rax], rcx
add rax, 8
lea rcx, [L2]
mov qword [rax], rcx
add rax, 8
lea rcx, [L3]
mov qword [rax], rcx
add rax, 8
lea rcx, [L4]
mov qword [rax], rcx
add rax, 8
lea rcx, [L5]
mov qword [rax], rcx
add rax, 8
lea rcx, [L6]
mov qword [rax], rcx
add rax, 8
lea rcx, [L7]
mov qword [rax], rcx
add rax, 8
lea rcx, [L8]
mov qword [rax], rcx
add rax, 8
lea rcx, [L9]
mov qword [rax], rcx
add rax, 8
lea rcx, [L10]
mov qword [rax], rcx
add rax, 8
lea rcx, [L11]
mov qword [rax], rcx
add rax, 8
lea rcx, [L12]
mov qword [rax], rcx
add rax, 8
lea rcx, [L13]
mov qword [rax], rcx
add rax, 8
lea rcx, [L14]
mov qword [rax], rcx
nop
jmp code_start
func_end:
mov rcx, r15
call 4319859
mov rcx, r15
mov rdx, qword [rcx+72]
lea rdx, [rdx+8]
mov qword [rcx+72], rdx
add rsp, 160
pop r9
pop r8
pop r15
pop r14
pop r13
pop r12
pop rbp
ret
.thread_check:
mov rcx, r15
mov eax, qword [rcx+80]
sar eax, 2
and eax, 1
test eax, eax
je L27
call 4231688
L27:
mov rcx, r15
mov eax, qword [rcx+136]
cmp eax, 3
jne L28
short jmp func_end
L28:
mov rcx, r15
mov eax, qword [rcx+80]
sar eax, 1
and eax, 1
test eax, eax
je L29
cmp r13, -1
je L31
mov rcx, r15
mov rax, qword [rcx+64]
imul r13, 8
add rax, r13
mov qword [rcx+72], rax
L31:
mov r13, 0
mov rcx, qword [rcx+32]
call 4667810
cmp rax, 1
je L30
jmp func_end
L30:
mov rcx, r15
call 4667774
mov rdx, qword [rbp-64]
imul rax, 8
add rdx, rax
mov r12, [rdx]
jmp r12
L29:
jmp r12
nop
nop
nop
align 64
.data:
; data section start
L20:
db 000000000000F03F
L21:
db 0000000000000840
L22:
db 0000000000000040
