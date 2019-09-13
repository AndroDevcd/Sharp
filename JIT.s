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
movsd xmm0, [L22]
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
movsd xmm0, [L23]
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
L5:
movsd xmm0, [L24]
mov rcx, r14
movsd qword [rcx], xmm0
nop
nop
nop
nop
nop
; instr 6
L6:
mov rcx, r14
add rcx, 24
movsd xmm0, qword [rcx]
cvttsd2si rcx, xmm0
call 4661164
mov qword [rbp-64], rax
lea r12, [L6]
mov r13, 6
mov rcx, r15
mov ecx, qword [rcx+80]
cmp ecx, 0
jne .thread_check
mov rcx, r15
mov rdx, qword [rcx+16]
lea rdx, [rdx+16]
mov qword [rcx+16], rdx
mov rcx, qword [rbp-64]
call 4662264
nop
nop
nop
nop
nop
; instr 7
L7:
mov rcx, r15
mov rcx, qword [rcx+16]
lea rcx, qword [rcx+8]
mov qword [rbp-56], rcx
nop
nop
nop
nop
nop
; instr 8
L8:
mov rcx, qword [rbp-56]
cmp rcx, 0
je L26
mov rcx, qword [rcx]
cmp rcx, 0
je L26
mov rcx, qword [rcx]
cmp rcx, 0
jne L25
L26:
mov r13, 8
cmp r13, -1
je L27
mov rcx, r15
mov rax, qword [rcx+64]
imul r13, 8
add rax, r13
mov qword [rcx+72], rax
L27:
mov r13, 0
call 4661910
mov r13, -1
jmp .thread_check
L25:
mov rdx, rcx
mov rcx, r14
movsd xmm0, qword [rcx]
cvttsd2si rax, xmm0
imul rax, 8
add rdx, rax
mov rcx, r14
add rcx, 24
movsd xmm0, qword [rcx]
movsd qword [rdx], xmm0
mov rcx, qword [rbp-56]
call 4659992
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
nop
nop
nop
nop
nop
nop
; instr 12
L12:
nop
nop
nop
nop
nop
nop
; instr 13
L13:
L14:
pxor xmm0, xmm0
mov rcx, r14
add rcx, 24
movsd qword [rcx], xmm0
nop
nop
nop
nop
nop
; instr 15
L15:
nop
nop
nop
nop
nop
nop
; instr 16
L16:
mov r13, 16
cmp r13, -1
je L28
mov rcx, r15
mov rax, qword [rcx+64]
imul r13, 8
add rax, r13
mov qword [rcx+72], rax
L28:
mov r13, 0
jmp func_end
nop
nop
nop
nop
nop
mov r13, 16
cmp r13, -1
je L29
mov rcx, r15
mov rax, qword [rcx+64]
imul r13, 8
add rax, r13
mov qword [rcx+72], rax
L29:
mov r13, 0
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
add rax, 8
lea rcx, [L12]
mov qword [rax], rcx
add rax, 8
lea rcx, [L13]
mov qword [rax], rcx
add rax, 8
lea rcx, [L14]
mov qword [rax], rcx
add rax, 8
lea rcx, [L15]
mov qword [rax], rcx
add rax, 8
lea rcx, [L16]
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
je L30
call 4231688
L30:
mov rcx, r15
mov eax, qword [rcx+136]
cmp eax, 3
jne L31
short jmp func_end
L31:
mov rcx, r15
mov eax, qword [rcx+80]
sar eax, 1
and eax, 1
test eax, eax
je L32
cmp r13, -1
je L34
mov rcx, r15
mov rax, qword [rcx+64]
imul r13, 8
add rax, r13
mov qword [rcx+72], rax
L34:
mov r13, 0
mov rcx, qword [rcx+32]
call 4663160
cmp rax, 1
je L33
jmp func_end
L33:
mov rcx, r15
call 4663124
mov rdx, qword [rbp-48]
imul rax, 8
add rdx, rax
mov r12, [rdx]
jmp r12
L32:
jmp r12
nop
nop
nop
align 64
.data:
; data section start
L22:
db 0000000000001440
L23:
db 0000000000E8C940
L24:
db 000000000000F03F
