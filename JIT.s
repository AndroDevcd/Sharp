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
instr 0
L0:
L1:
movsd xmm0, [L12]
mov rcx, r14
add rcx, 24
movsd qword [rcx], xmm0
nop
nop
nop
nop
instr 2
L2:
mov rcx, r14
add rcx, 24
movsd xmm0, qword [rcx]
movsd xmm1, [L13]
addsd xmm1, xmm0
mov rcx, r14
add rcx, 24
movsd qword [rcx], xmm1
call 4638066
nop
nop
nop
nop
instr 3
L3:
L4:
pxor xmm0, xmm0
mov rcx, r14
add rcx, 24
movsd qword [rcx], xmm0
nop
nop
nop
nop
instr 5
L5:
nop
nop
nop
nop
nop
instr 6
L6:
mov r13, 6
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
mov r13, 6
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
je L14
call 4231688
L14:
mov rcx, r15
mov eax, qword [rcx+136]
cmp eax, 3
jne L15
short jmp func_end
L15:
mov rcx, r15
mov eax, qword [rcx+80]
sar eax, 1
and eax, 1
test eax, eax
je L16
mov rcx, r15
mov rax, qword [rcx+64]
imul r13, 8
add rax, r13
mov qword [rcx+72], rax
mov rcx, qword [rcx+32]
call 4639730
cmp rax, 1
je L17
jmp func_end
L17:
mov rcx, r15
call 4639694
mov rdx, qword [rbp-48]
imul rax, 8
add rdx, rax
mov r12, [rdx]
jmp r12
L16:
jmp r12
nop
nop
nop
align 64
.data:
; data section start
L12:
db 0000000000001440
L13:
db 000000000000F03F
