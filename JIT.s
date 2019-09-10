; method std.kernel#Runtime.__srt_init_

; starting save state
push rbp
mov rbp, rsp
push r12
push r13
push r14
push r15
sub rsp, 160
mov qword [rbp-40], rcx
xor r12, r12
xor r13, r13
mov qword [rbp-56], 0
mov qword [rbp-48], 0
mov qword [rbp-64], 0
mov r14, qword [rcx+8]
mov r15, qword [rcx]
mov rcx, qword [rcx+16]
mov rcx, qword [rcx]
mov qword [rbp-48], rcx
mov rcx, qword [rcx]
test rcx, rcx
jne code_start
jmp init_addr_tbl
code_start:
; instr 0
L0:
L1:
movsd xmm0, [L17]
mov rcx, r14
add rcx, 24
movsd qword [rcx], xmm0
nop
nop
nop
nop
nop
; instr 2
L2:
L3:
movsd xmm0, [L17]
mov rcx, r14
add rcx, 32
movsd qword [rcx], xmm0
nop
nop
nop
nop
nop
; instr 4
L4:
mov rcx, r14
add rcx, 24
movsd xmm1, qword [rcx]
mov rcx, r14
add rcx, 32
movsd xmm0, qword [rcx]
ucomisd xmm0, xmm1
jp L18
jne L18
movsd xmm0, [L20]
jmp L19
L18:
pxor xmm0, xmm0
L19:
mov rcx, r14
add rcx, 16
movsd qword [rcx], xmm0
nop
nop
nop
nop
nop
; instr 5
L5:
mov rcx, 2
call 4647566
nop
nop
nop
nop
nop
; instr 6
L6:
mov rcx, r14
add rcx, 24
movsd xmm1, qword [rcx]
mov rcx, r14
add rcx, 32
movsd xmm0, qword [rcx]
ucomisd xmm0, xmm1
jp L21
je L21
movsd xmm0, [L20]
jmp L22
L21:
pxor xmm0, xmm0
L22:
mov rcx, r14
add rcx, 16
movsd qword [rcx], xmm0
nop
nop
nop
nop
nop
; instr 7
L7:
mov rcx, 2
call 4647566
nop
nop
nop
nop
nop
; instr 8
L8:
L9:
pxor xmm0, xmm0
mov rcx, r14
add rcx, 24
movsd qword [rcx], xmm0
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
mov r13, 11
mov rcx, r15
mov rax, qword [rcx+64]
imul r13, 8
add rax, r13
mov qword [rcx+72], rax
jmp func_end
nop
nop
nop
nop
nop
mov r13, 11
mov rcx, r15
mov rax, qword [rcx+64]
imul r13, 8
add rax, r13
mov qword [rcx+72], rax
jmp func_end
init_addr_tbl:
nop
; setting label values
mov rax, qword [rbp-48]
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
nop
jmp code_start
func_end:
mov rcx, r15
call 4320099
mov rcx, r15
mov rdx, qword [rcx+72]
lea rdx, [rdx+8]
mov qword [rcx+72], rdx
add rsp, 160
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
je L23
call 4231688
L23:
mov rcx, r15
mov eax, qword [rcx+136]
cmp eax, 3
jne L24
short jmp func_end
L24:
mov rcx, r15
mov eax, qword [rcx+80]
sar eax, 1
and eax, 1
test eax, eax
je L25
mov rcx, r15
mov rax, qword [rcx+64]
imul r13, 8
add rax, r13
mov qword [rcx+72], rax
mov rcx, qword [rcx+32]
call 4650440
cmp rax, 1
je L26
jmp func_end
L26:
mov rcx, r15
call 4650404
mov rdx, qword [rbp-48]
imul rax, 8
add rdx, rax
mov r12, [rdx]
jmp r12
L25:
jmp r12
nop
nop
nop
align 64
.data:
; data section start
L17:
db 0000000000001440
L20:
db 000000000000F03F
