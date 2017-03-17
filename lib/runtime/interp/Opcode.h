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
    | 190123456789012345678901234 | 0123456789012345678901234567 | 12345678 | max: FFFFFFF
    |			 argument		 |			argument		   |  opcode  |

    max 134217727 min -134217727
 */

#define OPCODE_MASK 0xff

#define SET_Ei(i, op) i=op;

#define SET_Di(i, op, a1) i=((op) | (a1 << 8));

#define SET_Ci(i, op, a1, n, a2) i=((op | ((n & 1) << 8) | (a1 << 9)) | ((int64_t)a2 << 36));

#define GET_OP(i) (i & OPCODE_MASK)
#define GET_Da(i) ((i >> 8)) // TODO: fix this
#define GET_Ca(i) (((i >> 8) & 1) ? -(i >> 9 & 0x7FFFFFF) : (i >> 9 & 0x7FFFFFF))
#define GET_Cb(i) (i >> 36)

#define _brh goto interp;

#define NOP _brh

#define _int(x) vm->interrupt(x); _brh

#define movi(r,x) regs[r]=x; _brh

#define ret { pop(); return; } _brh

#define hlt self->state=thread_killed; _brh

#define _new(t,x) \
{ \
    ptr->createnative(t,x);\
    (*pc)+=2; \
}; _brh

#define check_cast \
{ \
    \
}; _brh

#define pushd(x) thread_stack->push(x); _brh

#define mov8(r,x) regs[r]=(int8_t)regs[x]; _brh

#define mov16(r,x) regs[r]=(int16_t)regs[x]; _brh

#define mov32(r,x) regs[r]=(int32_t)regs[x]; _brh

#define mov64(r,x) regs[r]=(int64_t)regs[x]; _brh

#define pushr(r) thread_stack->push(regs[r]); _brh

#define add(r,x) regs[0x0008]=regs[r]+regs[x]; _brh

#define sub(r,x) regs[0x0008]=regs[r]-regs[x]; _brh _brh

#define mult(r,x) regs[0x0008]=regs[r]*regs[x]; _brh _brh

#define div(r,x) regs[0x0008]=regs[r]/regs[x]; _brh _brh

#define mod(r,x) regs[0x0008]=(int64_t)regs[r]%(int64_t)regs[x]; _brh

#define _pop _brh

#define inc(r) regs[r]++; _brh

#define dec(r) regs[r]--; _brh

#define swapr(r,x) regs[r]=regs[x]; _brh

#define movx(r,x) _nativeread(r,x) _brh

#define lt(r,x) regs[0x0002]=regs[r]<regs[x]; _brh

#define movpc regs[0x0000]=*pc; _brh

#define brh *pc=regs[0x0000]; _brh

#define bre if(regs[0x0002])*pc=regs[0x0000]; _brh

#define je(x) if(regs[0x0002])*pc=x; else x; _brh

#define _join(h,l) thread_stack->push(strtod (string(h + "." + l).c_str(),NULL)); _brh

#define ife if((regs[0x0002]) == false)*pc=regs[0x0000]; _brh

#define ifne if((!regs[0x0002]) == false)*pc=regs[0x0000]; _brh

#define gt(r,x) regs[0x0002]=regs[r]>regs[x]; _brh

#define gte(r,x) regs[0x0002]=regs[r]>=regs[x]; _brh

#define lte(r,x) regs[0x0002]=regs[r]<regs[x]; _brh

#define movl(x) ptr=x; _brh

#define object_nxt ptr=ptr->next; _brh

#define object_prev ptr=ptr->prev; _brh

#define movbi(x) regs[0x0008]=x; _brh

#define _sizeof(r) regs[r]=ptr->size; _brh

#endif //SHARP_OPCODE_H
