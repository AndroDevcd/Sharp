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
mov rcx, r15
mov rcx, qword [rcx+24]
lea rcx, qword [rcx+8]
mov qword [rbp-56], rcx
nop
nop
nop
nop
nop
; instr 1
L1:
mov rcx, qword [rbp-56]
call 4648092
nop
nop
nop
nop
nop
; instr 2
L2:
L3:
pxor xmm0, xmm0
mov rcx, r14
add rcx, 24
movsd qword [rcx], xmm0
nop
nop
nop
nop
nop
; instr 4
L4:
nop
nop
nop
nop
nop
nop
; instr 5
L5:
mov r13, 5
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
mov r13, 5
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
nop
jmp code_start
func_end:
mov rcx, r15
call 4320211
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
je L11
call 4231800
L11:
mov rcx, r15
mov eax, qword [rcx+136]
cmp eax, 3
jne L12
short jmp func_end
L12:
mov rcx, r15
mov eax, qword [rcx+80]
sar eax, 1
and eax, 1
test eax, eax
je L13
mov rcx, r15
mov rax, qword [rcx+64]
imul r13, 8
add rax, r13
mov qword [rcx+72], rax
mov rcx, qword [rcx+32]
call 4648726
cmp rax, 1
je L14
jmp func_end
L14:
mov rcx, r15
call 4648690
mov rdx, qword [rbp-48]
imul rax, 8
add rdx, rax
mov r12, [rdx]
jmp r12
L13:
jmp r12
nop
nop
nop
align 64
.data:
; data section start
