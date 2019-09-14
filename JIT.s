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
mov rcx, 47
call 4667526
mov qword [rbp-80], rax
lea r12, [L0]
mov r13, 0
mov rcx, r15
mov ecx, qword [rcx+80]
cmp ecx, 0
jne .thread_check
mov rcx, r15
mov rdx, qword [rcx+16]
lea rdx, [rdx+16]
mov qword [rcx+16], rdx
mov rcx, qword [rbp-80]
call 4668444
nop
nop
nop
nop
nop
; instr 1
L1:
mov rcx, r15
mov rdx, 34
call 4667842
lea r12, [L1]
mov r13, 1
mov rcx, r15
mov ecx, qword [rcx+80]
cmp ecx, 0
jne .thread_check
nop
nop
nop
nop
nop
; instr 2
L2:
mov rcx, r15
mov rdx, 553
call 4664720
cmp rax, 0
je L165
mov rcx, qword [rbp-56]
call rax
L165:
lea r12, [L2]
mov r13, 2
mov rcx, r15
mov ecx, qword [rcx+80]
cmp ecx, 0
jne .thread_check
nop
nop
nop
nop
nop
; instr 3
L3:
mov rcx, r15
mov rcx, qword [rcx+24]
lea rcx, qword [rcx+8]
mov qword [rbp-72], rcx
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
mov rcx, qword [rbp-72]
cmp rcx, 0
je L167
mov rcx, qword [rcx]
cmp rcx, 0
je L167
mov rcx, qword [rcx+8]
cmp rcx, 0
jne L166
L167:
mov r13, 6
cmp r13, -1
je L168
mov rcx, r15
mov rax, qword [rcx+64]
imul r13, 8
add rax, r13
mov qword [rcx+72], rax
L168:
mov r13, 0
call 4668090
mov r13, -1
jmp .thread_check
L166:
mov rdx, rcx
mov rcx, r14
add rcx, 24
movsd xmm0, qword [rcx]
cvttsd2si rax, xmm0
imul rax, 8
add rdx, rax
mov qword [rbp-72], rdx
nop
nop
nop
nop
nop
; instr 7
L7:
mov rcx, qword [rbp-72]
mov rdx, 1
call 4664134
lea r12, [L169]
mov r13, 7
mov rcx, r15
mov ecx, qword [rcx+80]
cmp ecx, 0
jne .thread_check
L169:
movzx ecx, [6242096]
cmp ecx, 0
je L170
mov r13, 7
jmp func_end
L170:
nop
nop
nop
nop
nop
; instr 8
L8:
mov rcx, r15
mov rax, qword [rcx+16]
lea rax, [rax+16]
mov qword [rcx+16], rax
mov rcx, rax
lea rax, qword [rcx+8]
mov rcx, rax
mov rdx, qword [rbp-72]
call 4668536
nop
nop
nop
nop
nop
; instr 9
L9:
mov rcx, r15
mov rdx, 566
call 4664720
cmp rax, 0
je L171
mov rcx, qword [rbp-56]
call rax
L171:
lea r12, [L9]
mov r13, 9
mov rcx, r15
mov ecx, qword [rcx+80]
cmp ecx, 0
jne .thread_check
nop
nop
nop
nop
nop
; instr 10
L10:
mov qword [rbp-72], 90319760
nop
nop
nop
nop
nop
; instr 11
L11:
mov rcx, qword [rbp-72]
cmp rcx, 0
je L173
mov rcx, qword [rcx]
cmp rcx, 0
je L173
mov rcx, qword [rcx+8]
cmp rcx, 0
jne L172
L173:
mov r13, 11
cmp r13, -1
je L174
mov rcx, r15
mov rax, qword [rcx+64]
imul r13, 8
add rax, r13
mov qword [rcx+72], rax
L174:
mov r13, 0
call 4668090
mov r13, -1
jmp .thread_check
L172:
add rcx, 8
mov qword [rbp-72], rcx
nop
nop
nop
nop
nop
; instr 12
L12:
mov rcx, qword [rbp-72]
cmp rcx, 0
je L176
jmp L175
L176:
mov r13, 12
cmp r13, -1
je L177
mov rcx, r15
mov rax, qword [rcx+64]
imul r13, 8
add rax, r13
mov qword [rcx+72], rax
L177:
mov r13, 0
call 4668090
mov r13, -1
jmp .thread_check
L175:
mov rcx, r15
mov rax, qword [rcx+16]
lea rdx, [rax-16]
mov qword [rcx+16], rdx
mov rcx, rax
lea rdx, qword [rcx+8]
mov rcx, qword [rbp-72]
call 4668536
nop
nop
nop
nop
nop
; instr 13
L13:
mov rcx, 47
call 4667526
mov qword [rbp-80], rax
lea r12, [L13]
mov r13, 13
mov rcx, r15
mov ecx, qword [rcx+80]
cmp ecx, 0
jne .thread_check
mov rcx, r15
mov rdx, qword [rcx+16]
lea rdx, [rdx+16]
mov qword [rcx+16], rdx
mov rcx, qword [rbp-80]
call 4668444
nop
nop
nop
nop
nop
; instr 14
L14:
mov rcx, r15
mov rdx, 34
call 4667842
lea r12, [L14]
mov r13, 14
mov rcx, r15
mov ecx, qword [rcx+80]
cmp ecx, 0
jne .thread_check
nop
nop
nop
nop
nop
; instr 15
L15:
mov rcx, r15
mov rdx, 553
call 4664720
cmp rax, 0
je L178
mov rcx, qword [rbp-56]
call rax
L178:
lea r12, [L15]
mov r13, 15
mov rcx, r15
mov ecx, qword [rcx+80]
cmp ecx, 0
jne .thread_check
nop
nop
nop
nop
nop
; instr 16
L16:
mov rcx, r15
mov rcx, qword [rcx+24]
lea rcx, qword [rcx+8]
mov qword [rbp-72], rcx
nop
nop
nop
nop
nop
; instr 17
L17:
L18:
movsd xmm0, [L179]
mov rcx, r14
add rcx, 24
movsd qword [rcx], xmm0
nop
nop
nop
nop
nop
; instr 19
L19:
mov rcx, qword [rbp-72]
cmp rcx, 0
je L181
mov rcx, qword [rcx]
cmp rcx, 0
je L181
mov rcx, qword [rcx+8]
cmp rcx, 0
jne L180
L181:
mov r13, 19
cmp r13, -1
je L182
mov rcx, r15
mov rax, qword [rcx+64]
imul r13, 8
add rax, r13
mov qword [rcx+72], rax
L182:
mov r13, 0
call 4668090
mov r13, -1
jmp .thread_check
L180:
mov rdx, rcx
mov rcx, r14
add rcx, 24
movsd xmm0, qword [rcx]
cvttsd2si rax, xmm0
imul rax, 8
add rdx, rax
mov qword [rbp-72], rdx
nop
nop
nop
nop
nop
; instr 20
L20:
mov rcx, qword [rbp-72]
mov rdx, 1
call 4664134
lea r12, [L183]
mov r13, 20
mov rcx, r15
mov ecx, qword [rcx+80]
cmp ecx, 0
jne .thread_check
L183:
movzx ecx, [6242096]
cmp ecx, 0
je L184
mov r13, 20
jmp func_end
L184:
nop
nop
nop
nop
nop
; instr 21
L21:
mov rcx, r15
mov rax, qword [rcx+16]
lea rax, [rax+16]
mov qword [rcx+16], rax
mov rcx, rax
lea rax, qword [rcx+8]
mov rcx, rax
mov rdx, qword [rbp-72]
call 4668536
nop
nop
nop
nop
nop
; instr 22
L22:
mov rcx, r15
mov rdx, 566
call 4664720
cmp rax, 0
je L185
mov rcx, qword [rbp-56]
call rax
L185:
lea r12, [L22]
mov r13, 22
mov rcx, r15
mov ecx, qword [rcx+80]
cmp ecx, 0
jne .thread_check
nop
nop
nop
nop
nop
; instr 23
L23:
mov qword [rbp-72], 90319760
nop
nop
nop
nop
nop
; instr 24
L24:
mov rcx, qword [rbp-72]
cmp rcx, 0
je L187
mov rcx, qword [rcx]
cmp rcx, 0
je L187
mov rcx, qword [rcx+8]
cmp rcx, 0
jne L186
L187:
mov r13, 24
cmp r13, -1
je L188
mov rcx, r15
mov rax, qword [rcx+64]
imul r13, 8
add rax, r13
mov qword [rcx+72], rax
L188:
mov r13, 0
call 4668090
mov r13, -1
jmp .thread_check
L186:
add rcx, 24
mov qword [rbp-72], rcx
nop
nop
nop
nop
nop
; instr 25
L25:
mov rcx, qword [rbp-72]
cmp rcx, 0
je L190
jmp L189
L190:
mov r13, 25
cmp r13, -1
je L191
mov rcx, r15
mov rax, qword [rcx+64]
imul r13, 8
add rax, r13
mov qword [rcx+72], rax
L191:
mov r13, 0
call 4668090
mov r13, -1
jmp .thread_check
L189:
mov rcx, r15
mov rax, qword [rcx+16]
lea rdx, [rax-16]
mov qword [rcx+16], rdx
mov rcx, rax
lea rdx, qword [rcx+8]
mov rcx, qword [rbp-72]
call 4668536
nop
nop
nop
nop
nop
; instr 26
L26:
mov rcx, 47
call 4667526
mov qword [rbp-80], rax
lea r12, [L26]
mov r13, 26
mov rcx, r15
mov ecx, qword [rcx+80]
cmp ecx, 0
jne .thread_check
mov rcx, r15
mov rdx, qword [rcx+16]
lea rdx, [rdx+16]
mov qword [rcx+16], rdx
mov rcx, qword [rbp-80]
call 4668444
nop
nop
nop
nop
nop
; instr 27
L27:
mov rcx, r15
mov rdx, 34
call 4667842
lea r12, [L27]
mov r13, 27
mov rcx, r15
mov ecx, qword [rcx+80]
cmp ecx, 0
jne .thread_check
nop
nop
nop
nop
nop
; instr 28
L28:
mov rcx, r15
mov rdx, 553
call 4664720
cmp rax, 0
je L192
mov rcx, qword [rbp-56]
call rax
L192:
lea r12, [L28]
mov r13, 28
mov rcx, r15
mov ecx, qword [rcx+80]
cmp ecx, 0
jne .thread_check
nop
nop
nop
nop
nop
; instr 29
L29:
mov rcx, r15
mov rcx, qword [rcx+24]
lea rcx, qword [rcx+8]
mov qword [rbp-72], rcx
nop
nop
nop
nop
nop
; instr 30
L30:
L31:
movsd xmm0, [L193]
mov rcx, r14
add rcx, 24
movsd qword [rcx], xmm0
nop
nop
nop
nop
nop
; instr 32
L32:
mov rcx, qword [rbp-72]
cmp rcx, 0
je L195
mov rcx, qword [rcx]
cmp rcx, 0
je L195
mov rcx, qword [rcx+8]
cmp rcx, 0
jne L194
L195:
mov r13, 32
cmp r13, -1
je L196
mov rcx, r15
mov rax, qword [rcx+64]
imul r13, 8
add rax, r13
mov qword [rcx+72], rax
L196:
mov r13, 0
call 4668090
mov r13, -1
jmp .thread_check
L194:
mov rdx, rcx
mov rcx, r14
add rcx, 24
movsd xmm0, qword [rcx]
cvttsd2si rax, xmm0
imul rax, 8
add rdx, rax
mov qword [rbp-72], rdx
nop
nop
nop
nop
nop
; instr 33
L33:
mov rcx, qword [rbp-72]
mov rdx, 1
call 4664134
lea r12, [L197]
mov r13, 33
mov rcx, r15
mov ecx, qword [rcx+80]
cmp ecx, 0
jne .thread_check
L197:
movzx ecx, [6242096]
cmp ecx, 0
je L198
mov r13, 33
jmp func_end
L198:
nop
nop
nop
nop
nop
; instr 34
L34:
mov rcx, r15
mov rax, qword [rcx+16]
lea rax, [rax+16]
mov qword [rcx+16], rax
mov rcx, rax
lea rax, qword [rcx+8]
mov rcx, rax
mov rdx, qword [rbp-72]
call 4668536
nop
nop
nop
nop
nop
; instr 35
L35:
mov rcx, r15
mov rdx, 566
call 4664720
cmp rax, 0
je L199
mov rcx, qword [rbp-56]
call rax
L199:
lea r12, [L35]
mov r13, 35
mov rcx, r15
mov ecx, qword [rcx+80]
cmp ecx, 0
jne .thread_check
nop
nop
nop
nop
nop
; instr 36
L36:
mov qword [rbp-72], 90319760
nop
nop
nop
nop
nop
; instr 37
L37:
mov rcx, qword [rbp-72]
cmp rcx, 0
je L201
mov rcx, qword [rcx]
cmp rcx, 0
je L201
mov rcx, qword [rcx+8]
cmp rcx, 0
jne L200
L201:
mov r13, 37
cmp r13, -1
je L202
mov rcx, r15
mov rax, qword [rcx+64]
imul r13, 8
add rax, r13
mov qword [rcx+72], rax
L202:
mov r13, 0
call 4668090
mov r13, -1
jmp .thread_check
L200:
add rcx, 16
mov qword [rbp-72], rcx
nop
nop
nop
nop
nop
; instr 38
L38:
mov rcx, qword [rbp-72]
cmp rcx, 0
je L204
jmp L203
L204:
mov r13, 38
cmp r13, -1
je L205
mov rcx, r15
mov rax, qword [rcx+64]
imul r13, 8
add rax, r13
mov qword [rcx+72], rax
L205:
mov r13, 0
call 4668090
mov r13, -1
jmp .thread_check
L203:
mov rcx, r15
mov rax, qword [rcx+16]
lea rdx, [rax-16]
mov qword [rcx+16], rdx
mov rcx, rax
lea rdx, qword [rcx+8]
mov rcx, qword [rbp-72]
call 4668536
nop
nop
nop
nop
nop
; instr 39
L39:
mov rcx, 47
call 4667526
mov qword [rbp-80], rax
lea r12, [L39]
mov r13, 39
mov rcx, r15
mov ecx, qword [rcx+80]
cmp ecx, 0
jne .thread_check
mov rcx, r15
mov rdx, qword [rcx+16]
lea rdx, [rdx+16]
mov qword [rcx+16], rdx
mov rcx, qword [rbp-80]
call 4668444
nop
nop
nop
nop
nop
; instr 40
L40:
mov rcx, r15
mov rdx, 34
call 4667842
lea r12, [L40]
mov r13, 40
mov rcx, r15
mov ecx, qword [rcx+80]
cmp ecx, 0
jne .thread_check
nop
nop
nop
nop
nop
; instr 41
L41:
mov rcx, r15
mov rdx, 553
call 4664720
cmp rax, 0
je L206
mov rcx, qword [rbp-56]
call rax
L206:
lea r12, [L41]
mov r13, 41
mov rcx, r15
mov ecx, qword [rcx+80]
cmp ecx, 0
jne .thread_check
nop
nop
nop
nop
nop
; instr 42
L42:
mov rcx, r15
mov rcx, qword [rcx+24]
lea rcx, qword [rcx+8]
mov qword [rbp-72], rcx
nop
nop
nop
nop
nop
; instr 43
L43:
L44:
movsd xmm0, [L207]
mov rcx, r14
add rcx, 24
movsd qword [rcx], xmm0
nop
nop
nop
nop
nop
; instr 45
L45:
mov rcx, qword [rbp-72]
cmp rcx, 0
je L209
mov rcx, qword [rcx]
cmp rcx, 0
je L209
mov rcx, qword [rcx+8]
cmp rcx, 0
jne L208
L209:
mov r13, 45
cmp r13, -1
je L210
mov rcx, r15
mov rax, qword [rcx+64]
imul r13, 8
add rax, r13
mov qword [rcx+72], rax
L210:
mov r13, 0
call 4668090
mov r13, -1
jmp .thread_check
L208:
mov rdx, rcx
mov rcx, r14
add rcx, 24
movsd xmm0, qword [rcx]
cvttsd2si rax, xmm0
imul rax, 8
add rdx, rax
mov qword [rbp-72], rdx
nop
nop
nop
nop
nop
; instr 46
L46:
mov rcx, qword [rbp-72]
mov rdx, 1
call 4664134
lea r12, [L211]
mov r13, 46
mov rcx, r15
mov ecx, qword [rcx+80]
cmp ecx, 0
jne .thread_check
L211:
movzx ecx, [6242096]
cmp ecx, 0
je L212
mov r13, 46
jmp func_end
L212:
nop
nop
nop
nop
nop
; instr 47
L47:
mov rcx, r15
mov rax, qword [rcx+16]
lea rax, [rax+16]
mov qword [rcx+16], rax
mov rcx, rax
lea rax, qword [rcx+8]
mov rcx, rax
mov rdx, qword [rbp-72]
call 4668536
nop
nop
nop
nop
nop
; instr 48
L48:
mov rcx, r15
mov rdx, 566
call 4664720
cmp rax, 0
je L213
mov rcx, qword [rbp-56]
call rax
L213:
lea r12, [L48]
mov r13, 48
mov rcx, r15
mov ecx, qword [rcx+80]
cmp ecx, 0
jne .thread_check
nop
nop
nop
nop
nop
; instr 49
L49:
mov qword [rbp-72], 90319760
nop
nop
nop
nop
nop
; instr 50
L50:
mov rcx, qword [rbp-72]
cmp rcx, 0
je L215
mov rcx, qword [rcx]
cmp rcx, 0
je L215
mov rcx, qword [rcx+8]
cmp rcx, 0
jne L214
L215:
mov r13, 50
cmp r13, -1
je L216
mov rcx, r15
mov rax, qword [rcx+64]
imul r13, 8
add rax, r13
mov qword [rcx+72], rax
L216:
mov r13, 0
call 4668090
mov r13, -1
jmp .thread_check
L214:
mov qword [rbp-72], rcx
nop
nop
nop
nop
nop
; instr 51
L51:
mov rcx, qword [rbp-72]
cmp rcx, 0
je L218
jmp L217
L218:
mov r13, 51
cmp r13, -1
je L219
mov rcx, r15
mov rax, qword [rcx+64]
imul r13, 8
add rax, r13
mov qword [rcx+72], rax
L219:
mov r13, 0
call 4668090
mov r13, -1
jmp .thread_check
L217:
mov rcx, r15
mov rax, qword [rcx+16]
lea rdx, [rax-16]
mov qword [rcx+16], rdx
mov rcx, rax
lea rdx, qword [rcx+8]
mov rcx, qword [rbp-72]
call 4668536
nop
nop
nop
nop
nop
; instr 52
L52:
mov rcx, r15
mov rdx, 301
call 4664720
cmp rax, 0
je L220
mov rcx, qword [rbp-56]
call rax
L220:
lea r12, [L52]
mov r13, 52
mov rcx, r15
mov ecx, qword [rcx+80]
cmp ecx, 0
jne .thread_check
nop
nop
nop
nop
nop
; instr 53
L53:
mov rcx, r15
mov rdx, 302
call 4664720
cmp rax, 0
je L221
mov rcx, qword [rbp-56]
call rax
L221:
lea r12, [L53]
mov r13, 53
mov rcx, r15
mov ecx, qword [rcx+80]
cmp ecx, 0
jne .thread_check
nop
nop
nop
nop
nop
; instr 54
L54:
mov rcx, r15
mov rdx, 240
call 4664720
cmp rax, 0
je L222
mov rcx, qword [rbp-56]
call rax
L222:
lea r12, [L54]
mov r13, 54
mov rcx, r15
mov ecx, qword [rcx+80]
cmp ecx, 0
jne .thread_check
nop
nop
nop
nop
nop
; instr 55
L55:
mov qword [rbp-72], 90319768
nop
nop
nop
nop
nop
; instr 56
L56:
mov rcx, qword [rbp-72]
cmp rcx, 0
je L224
mov rcx, qword [rcx]
cmp rcx, 0
je L224
mov rcx, qword [rcx+8]
cmp rcx, 0
jne L223
L224:
mov r13, 56
cmp r13, -1
je L225
mov rcx, r15
mov rax, qword [rcx+64]
imul r13, 8
add rax, r13
mov qword [rcx+72], rax
L225:
mov r13, 0
call 4668090
mov r13, -1
jmp .thread_check
L223:
mov qword [rbp-72], rcx
nop
nop
nop
nop
nop
; instr 57
L57:
L58:
pxor xmm0, xmm0
mov rcx, r14
movsd qword [rcx], xmm0
nop
nop
nop
nop
nop
; instr 59
L59:
mov rcx, qword [rbp-72]
cmp rcx, 0
je L227
mov rcx, qword [rcx]
cmp rcx, 0
je L227
mov rcx, qword [rcx]
cmp rcx, 0
jne L226
L227:
mov r13, 59
cmp r13, -1
je L228
mov rcx, r15
mov rax, qword [rcx+64]
imul r13, 8
add rax, r13
mov qword [rcx+72], rax
L228:
mov r13, 0
call 4668090
mov r13, -1
jmp .thread_check
L226:
mov rdx, rcx
mov rcx, r14
movsd xmm0, qword [rcx]
cvttsd2si rax, xmm0
imul rax, 8
mov rcx, rdx
add rdx, rax
movsd xmm0, qword [rdx]
mov rcx, r14
add rcx, 24
movsd qword [rcx], xmm0
nop
nop
nop
nop
nop
; instr 60
L60:
mov rcx, r14
add rcx, 24
movsd xmm0, qword [rcx]
mov rcx, r15
add qword [rcx+16], 16
mov rax, qword [rcx+16]
movsd qword [rax], xmm0
nop
nop
nop
nop
nop
; instr 61
L61:
mov rcx, r15
mov rcx, qword [rcx+24]
lea rcx, qword [rcx+8]
mov qword [rbp-72], rcx
nop
nop
nop
nop
nop
; instr 62
L62:
mov rcx, qword [rbp-72]
cmp rcx, 0
jne L229
L231:
pxor xmm0, xmm0
mov rcx, r14
add rcx, 24
movsd qword [rcx], xmm0
jmp L230
L229:
mov rcx, qword [rcx]
cmp rcx, 0
short je L231
mov rcx, qword [rcx+24]
cvtsi2sd xmm0, rcx
mov rcx, r14
add rcx, 24
movsd qword [rcx], xmm0
L230:
nop
nop
nop
nop
nop
; instr 63
L63:
mov rcx, r15
mov rax, qword [rcx+16]
lea rdx, [rax-16]
mov qword [rcx+16], rdx
mov rcx, rax
movsd xmm0, qword [rcx]
mov rcx, r14
add rcx, 72
movsd qword [rcx], xmm0
nop
nop
nop
nop
nop
; instr 64
L64:
mov rcx, r14
add rcx, 24
movsd xmm0, qword [rcx]
mov rcx, r14
add rcx, 72
movsd xmm1, qword [rcx]
subsd xmm0, xmm1
L65:
mov rcx, r14
add rcx, 24
movsd qword [rcx], xmm0
nop
nop
nop
nop
nop
; instr 66
L66:
mov rcx, r14
add rcx, 24
movsd xmm0, qword [rcx]
cvttsd2si rcx, xmm0
mov rdx, 47
call 4667682
mov qword [rbp-80], rax
lea r12, [L66]
mov r13, 66
mov rcx, r15
mov ecx, qword [rcx+80]
cmp ecx, 0
jne .thread_check
mov rcx, r15
mov rdx, qword [rcx+16]
lea rdx, [rdx+16]
mov qword [rcx+16], rdx
mov rcx, qword [rbp-80]
call 4668444
nop
nop
nop
nop
nop
; instr 67
L67:
mov qword [rbp-72], 90319768
nop
nop
nop
nop
nop
; instr 68
L68:
mov rcx, qword [rbp-72]
cmp rcx, 0
je L233
mov rcx, qword [rcx]
cmp rcx, 0
je L233
mov rcx, qword [rcx+8]
cmp rcx, 0
jne L232
L233:
mov r13, 68
cmp r13, -1
je L234
mov rcx, r15
mov rax, qword [rcx+64]
imul r13, 8
add rax, r13
mov qword [rcx+72], rax
L234:
mov r13, 0
call 4668090
mov r13, -1
jmp .thread_check
L232:
add rcx, 8
mov qword [rbp-72], rcx
nop
nop
nop
nop
nop
; instr 69
L69:
mov rcx, qword [rbp-72]
cmp rcx, 0
je L236
jmp L235
L236:
mov r13, 69
cmp r13, -1
je L237
mov rcx, r15
mov rax, qword [rcx+64]
imul r13, 8
add rax, r13
mov qword [rcx+72], rax
L237:
mov r13, 0
call 4668090
mov r13, -1
jmp .thread_check
L235:
mov rcx, r15
mov rax, qword [rcx+16]
lea rdx, [rax-16]
mov qword [rcx+16], rdx
mov rcx, rax
lea rdx, qword [rcx+8]
mov rcx, qword [rbp-72]
call 4668536
nop
nop
nop
nop
nop
; instr 70
L70:
mov rcx, r15
mov rcx, qword [rcx+24]
add rcx, 16
L71:
pxor xmm0, xmm0
movsd qword [rcx], xmm0
nop
nop
nop