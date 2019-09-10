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
pxor xmm0, xmm0
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
mov rcx, r15
mov rcx, qword [rcx+24]
add rcx, 16
lea rcx, qword [rcx+8]
mov qword [rbp-56], rcx
nop
nop
nop
nop
nop
; instr 3
L3:
mov rcx, qword [rbp-56]
cmp rcx, 0
je L14
mov rcx, qword [rcx]
cmp rcx, 0
je L14
mov rcx, qword [rcx+8]
cmp rcx, 0
jne L13
L14:
mov r13, 3
cmp r13, -1
je L15
mov rcx, r15
mov rax, qword [rcx+64]
imul r13, 8
add rax, r13
mov qword [rcx+72], rax
L15:
call 4650808
mov r13, -1
jmp .thread_check
L13:
mov rdx, rcx
mov rcx, r14
add rcx, 24
movsd xmm0, qword [rcx]
cvttsd2si rax, xmm0
imul rax, 8
add rdx, rax
mov qword [rbp-56], rdx
nop
nop
nop
nop
nop
; instr 4
L4:
L5:
pxor xmm0, xmm0
mov rcx, r14
add rcx, 24
movsd qword [rcx], xmm0
nop
nop
nop
nop
nop
; instr 6
L6:
nop
nop
nop
nop
nop
nop
; instr 7
L7:
mov r13, 7
cmp r13, -1
je L16
mov rcx, r15
mov rax, qword [rcx+64]
imul r13, 8
add rax, r13
mov qword [rcx+72], rax
L16:
jmp func_end
nop
nop
nop
nop
nop
mov r13, 7
cmp r13, -1
je L17
mov rcx, r15
mov rax, qword [rcx+64]
imul r13, 8
add rax, r13
mov qword [rcx+72], rax
L17:
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
nop
jmp code_start
func_end:
mov rcx, r15
call 4320051
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
je L18
call 4231688
L18:
mov rcx, r15
mov eax, qword [rcx+136]
cmp eax, 3
jne L19
short jmp func_end
L19:
mov rcx, r15
mov eax, qword [rcx+80]
sar eax, 1
and eax, 1
test eax, eax
je L20
cmp r13, -1
je L22
mov rcx, r15
mov rax, qword [rcx+64]
imul r13, 8
add rax, r13
mov qword [rcx+72], rax
L22:
mov rcx, qword [rcx+32]
call 4651886
cmp rax, 1
je L21
jmp func_end
L21:
mov rcx, r15
call 4651850
mov rdx, qword [rbp-48]
imul rax, 8
add rdx, rax
mov r12, [rdx]
jmp r12
L20:
jmp r12
nop
nop
nop
align 64
.data:
; data section start
