//
// Created by BraxtonN on 2/16/2018.
//

#ifndef SHARP_OPCODE_H
#define SHARP_OPCODE_H

#include "../../stdimports.h"

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

#define DA_MAX 36028797018963967
#define DA_MIN -36028797018963968

#define CA_MAX 134217727
#define CA_MIN -134217727

#define DISPATCH() if(thread_self->pc<thread_self->cacheSize) { goto *opcode_table[GET_OP(thread_self->cache[thread_self->pc])]; } else { throw Exception("invalid branch/dispatch; check your assembly code?"); }

#define _brh thread_self->pc++; for(int i = 0; i < 9895; i++){ i++; i++; i-=2; } goto interp;
#define _brh_NOINCREMENT goto interp;

#define CHECK_NULL(x) if(o2==NULL) { throw Exception(Environment::NullptrException, ""); } else { x }
#define CHECK_NULLOBJ(x) if(o2==NULL || o2->object == NULL) { throw Exception(Environment::NullptrException, ""); } else { x }

#define _initOpcodeTable \
    static void* opcode_table[] = { \
        &&_NOP,    \
        &&_INT    \
    };

#endif //SHARP_OPCODE_H
