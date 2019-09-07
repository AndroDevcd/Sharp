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
mov rcx, r15
mov rcx, qword [rcx+24]
add rcx, 16
L1:
pxor xmm0, xmm0
movsd qword [rcx], xmm0
nop
nop
nop
nop
nop
instr 2
L2:
L3:
movsd xmm0, [L27]
mov rcx, r14
add rcx, 72
movsd qword [rcx], xmm0
nop
nop
nop
nop
nop
instr 4
L4:
mov rcx, r15
mov rcx, qword [rcx+24]
add rcx, 16
movsd xmm0, qword [rcx]
mov rcx, r14
add rcx, 24
movsd qword [rcx], xmm0
nop
nop
nop
nop
nop
instr 5
L5:
mov rcx, r14
add rcx, 24
movsd xmm0, qword [rcx]
mov rcx, r14
add rcx, 72
movsd xmm0, qword [rcx]
comisd xmm0, xmm1
jbe L28
movsd xmm0, [L30]
jmp L29
L28:
pxor xmm0, xmm0
L29:
mov rcx, r14
add rcx, 16
movsd qword [rcx], xmm0
nop
nop
nop
nop
nop
instr 6
L6:
L7:
movsd xmm0, [L31]
mov rcx, r14
movsd qword [rcx], xmm0
nop
nop
nop
nop
nop
lea r12, [L8]
mov r13, 8
mov rcx, r15
mov ecx, qword [rcx+80]
cmp ecx, 0
jne .thread_check
instr 8
L8:
mov rcx, r14
add rcx, 16
movsd xmm0, qword [rcx]
cvttsd2si rax, xmm0
cmp rax, 0
jne L32
mov rcx, r14
movsd xmm0, qword [rcx]
cvttsd2si rdx, xmm0
mov rax, qword [rbp-48]
cmp rdx, 0
je L33
imul rdx, 8
add rax, rdx
L33:
mov rax, [rax]
jmp rax
L32:
nop
nop
nop
nop
nop
instr 9
L9:
mov rcx, r15
mov rcx, qword [rcx+24]
add rcx, 16
movsd xmm1, qword [rcx]
movsd xmm0, [L30]
addsd xmm0, xmm1
movsd qword [rcx], xmm0
nop
nop
nop
nop
nop
instr 10
L10:
L11:
movsd xmm0, [L34]
mov rcx, r14
movsd qword [rcx], xmm0
nop
nop
nop
nop
nop
lea r12, [L12]
mov r13, 12
mov rcx, r15
mov ecx, qword [rcx+80]
cmp ecx, 0
jne .thread_check
instr 12
L12:
mov rcx, r14
movsd xmm0, qword [rcx]
cvttsd2si rax, xmm0
mov rdx, qword [rbp-48]
cmp rax, 0
je L35
imul rax, 8
add rdx, rax
L35:
mov rdx, [rdx]
jmp rdx
nop
nop
nop
nop
nop
instr 13
L13:
nop
nop
nop
nop
nop
nop
instr 14
L14:
mov rcx, r15
mov rcx, qword [rcx+24]
add rcx, 48
L15:
movsd xmm0, [L36]
movsd qword [rcx], xmm0
nop
nop
nop
nop
nop
instr 16
L16:
L17:
movsd xmm0, [L37]
mov rcx, r14
add rcx, 24
movsd qword [rcx], xmm0
nop
nop
nop
nop
nop
instr 18
L18:
L19:
pxor xmm0, xmm0
mov rcx, r14
add rcx, 24
movsd qword [rcx], xmm0
nop
nop
nop
nop
nop
instr 20
L20:
nop
nop
nop
nop
nop
nop
instr 21
L21:
mov r13, 21
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
mov r13, 21
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
add rax, 8
lea rcx, [L17]
mov qword [rax], rcx
add rax, 8
lea rcx, [L18]
mov qword [rax], rcx
add rax, 8
lea rcx, [L19]
mov qword [rax], rcx
add rax, 8
lea rcx, [L20]
mov qword [rax], rcx
add rax, 8
lea rcx, [L21]
mov qword [rax], rcx
nop
jmp code_start
func_end:
mov rcx, r15
call 4328032
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
je L38
call 4234608
L38:
mov rcx, r15
mov eax, qword [rcx+136]
cmp eax, 3
jne L39
short jmp func_end
L39:
mov rcx, r15
mov eax, qword [rcx+80]
sar eax, 1
and eax, 1
test eax, eax
je L40
mov rcx, r15
mov rax, qword [rcx+64]
imul r13, 8
add rax, r13
mov qword [rcx+72], rax
mov rcx, qword [rcx+32]
call 4561920
cmp rax, 1
je L41
jmp func_end
L41:
mov rcx, r15
call 4561888
mov rdx, qword [rbp-48]
imul rax, 8
add rdx, rax
mov r12, [rdx]
jmp r12
L40:
jmp r12
nop
nop
nop
align 64
.data:
; data section start
L27:
db 0000000065CDBD41
L30:
db 000000000000F03F
L31:
db 0000000000002A40
L34:
db 0000000000001040
L36:
db 0000000000D07B40
L37:
db 0000000000001440
