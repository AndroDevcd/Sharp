//
// Created by BraxtonN on 2/24/2017.
//

#ifndef SHARP_OPCODE_H
#define SHARP_OPCODE_H

#include "../../../stdimports.h"

/**
 * Bit layout
 *
 * Class D Instruction
    | 123456789012345678901234567890123456789012345678901234 | 12345678 |  max: 7FFFFFFFFFFFFF
    |						argument						  |  opcode  |

    max 36028797018963967 min -36028797018963968

    Class C Instruction
    | 190123456789012345678901234 | 0123456789012345678901234567 | 8 | 12345678 | max: FFFFFFF
    |			 argument		 |			argument		      | +-|  opcode  |

    max 134217727 min -134217727
 */

#define OPCODE_MASK 0xff

#define SET_Ei(i, op) (i=op)

#define SET_Di(i, op, a1) (i=((op) | ((int64_t)a1 << 8)))

#define SET_Ci(i, op, a1, n, a2) (i=((op | ((n & 1) << 8) | ((int64_t)a1 << 9)) | ((int64_t)a2 << 36)))

#define GET_OP(i) (i & OPCODE_MASK)
#define GET_Da(i) (i >> 8)
#define GET_Ca(i) (((i >> 8) & 1) ? (-1*(i >> 9 & 0x7FFFFFF)) : (i >> 9 & 0x7FFFFFF))
#define GET_Cb(i) (i >> 36)

#define _brh pc++; goto _interp;
#define _brh_NOINCREMENT goto _interp;

#define NOP _brh

#define _int(x) vm->interrupt(x); _brh

#define movi(x) regs[*(pc+1)]=x; pc++; _brh

#define ret { pop(); return; } _brh

#define hlt self->state=thread_killed; _brh

#define _new(t,x) \
{ \
    CHECK_PTR(ptr->createnative(t,regs[x]);)\
}; _brh

#define check_cast \
{ \
    \
}; _brh

#define mov8(r,x) regs[r]=(int8_t)regs[x]; _brh

#define mov16(r,x) regs[r]=(int16_t)regs[x]; _brh

#define mov32(r,x) regs[r]=(int32_t)regs[x]; _brh

#define mov64(r,x) regs[r]=(int64_t)regs[x]; _brh

#define pushr(r) thread_stack->push(regs[r]); _brh

#define add(r,x) regs[0x0008]=regs[r]+regs[x]; _brh

#define sub(r,x) regs[0x0008]=regs[r]-regs[x]; _brh _brh

#define mul(r,x) regs[0x0008]=regs[r]*regs[x]; _brh _brh

#define div(r,x) regs[0x0008]=regs[r]/regs[x]; _brh _brh

#define mod(r,x) regs[0x0008]=(int64_t)regs[r]%(int64_t)regs[x]; _brh

#define _pop _brh

#define inc(r) regs[r]++; _brh

#define dec(r) regs[r]--; _brh

#define movr(r,x) regs[r]=regs[x]; _brh

#define movx(r,x) _nativeread(r,x) _brh

#define lt(r,x) regs[0x0002]=regs[r]<regs[x]; _brh

#define brh pc=&env->bytecode[(int64_t)regs[0x0000]]; _brh_NOINCREMENT

#define bre if(regs[0x0002])pc=&env->bytecode[(int64_t)regs[0x0000]]; else _brh

#define ife if((regs[0x0002]) == false)pc=&env->bytecode[(int64_t)regs[0x0000]]; else  _brh

#define ifne if((!regs[0x0002]) == false)pc=&env->bytecode[(int64_t)regs[0x0000]]; else _brh

#define gt(r,x) regs[0x0002]=regs[r]>regs[x]; _brh

#define gte(r,x) regs[0x0002]=regs[r]>=regs[x]; _brh

#define lte(r,x) regs[0x0002]=regs[r]<regs[x]; _brh

#define movl(x) CHECK_PTR(ptr=x;) _brh

#define object_nxt CHECK_PTR(ptr=ptr->nxt;) _brh

#define object_prev CHECK_PTR(ptr=ptr->prev;) _brh

#define movbi(x) regs[0x0008]=x; pc++; _brh

#define _sizeof(r) CHECK_PTR(regs[r]=ptr->size;) _brh

#define _put(r) cout << regs[r]; _brh

#define putc(r) cout << (char)regs[r]; _brh

enum OPCODE {
    _NOP=0x0,
    _INT=0x1,
    MOVI=0x2,
    RET=0x3,
    HLT=0x4,
    NEW=0x5,
    CHECK_CAST=0x6,
    MOV8=0x7,
    MOV16=0x8,
    MOV32=0x9,
    MOV64=0xa,
    PUSHR=0xb,
    ADD=0xc,
    SUB=0xd,
    MUL=0xe,
    DIV=0xf,
    MOD=0x10,
    POP=0x11,
    INC=0x12,
    DEC=0x13,
    MOVR=0x14,
    MOVX=0x15,
    LT=0x16,
    BRH=0x18,
    BRE=0x19,
    IFE=0x1c,
    IFNE=0x1d,
    GT=0x1e,
    GTE=0x1f,
    LTE=0x20,
    MOVL=0x21,
    OBJECT_NXT=0x22,
    OBJECT_PREV=0x23,
    RMOV=0x24,
    MOV=0x25,
    MOVD=0x26,
    MOVBI=0x27,
    _SIZEOF=0x28,
    PUT=0x29,
    PUTC=0x2a
};

#endif //SHARP_OPCODE_H
