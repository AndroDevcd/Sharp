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

#define DISPATCH() goto *opcode_table[GET_OP(cache[pc])];

#define SAFTEY_CHECK \
    if (suspendPending) \
        suspendSelf(); \
    if (state == THREAD_KILLED) \
        return; \

#define STACK_CHECK  if((sp+1) >= stack_lmt) throw Exception(Environment::StackOverflowErr, "");

#define _brh_NOINCREMENT SAFTEY_CHECK count++; if(!startAddress) DISPATCH() else goto *opcodeStart;
#define _brh  pc++; _brh_NOINCREMENT

#define CHECK_NULL(x) if(o2==NULL) { throw Exception(Environment::NullptrException, ""); } else { x }
#define CHECK_NULL2(x) if(o2==NULL|o2->object == NULL) { throw Exception(Environment::NullptrException, ""); } else { x }
#define CHECK_NULLOBJ(x) if(o2==NULL || o2->object == NULL || o2->object->node==NULL) { throw Exception(Environment::NullptrException, ""); } else { x }
#define CHECK_INULLOBJ(x) if(o2==NULL || o2->object == NULL || o2->object->HEAD==NULL) { throw Exception(Environment::NullptrException, ""); } else { x }

#define _initOpcodeTable \
    static void* opcode_table[] = { \
        &&_NOP,            \
        &&_INT,              \
        &&_MOVI,              \
        &&RET,              \
        &&HLT,              \
        &&NEWARRAY,         \
        &&CAST,              \
        &&MOV8,              \
        &&MOV16,              \
        &&MOV32,              \
        &&MOV64,              \
        &&MOVU8,              \
        &&MOVU16,              \
        &&MOVU32,              \
        &&MOVU64,              \
        &&RSTORE,               \
        &&ADD,                  \
        &&SUB,                   \
        &&MUL,                   \
        &&DIV,                   \
        &&MOD,                   \
        &&IADD,                   \
        &&ISUB,                   \
        &&IMUL,                   \
        &&IDIV,                   \
        &&IMOD,                   \
        &&POP,                   \
        &&INC,                   \
        &&DEC,                   \
        &&MOVR,                   \
        &&IALOAD,                   \
        &&BRH,                   \
        &&IFE,                   \
        &&IFNE,                   \
        &&LT,                   \
        &&GT,                   \
        &&LTE,                   \
        &&GTE,                   \
        &&MOVL,                   \
        &&MOVSL,                   \
        &&MOVBI,                   \
        &&SIZEOF,                   \
        &&PUT,                   \
        &&PUTC,                   \
        &&CHECKLEN,                   \
        &&GOTO,                   \
        &&LOADPC,                   \
        &&PUSHOBJ,                   \
        &&DEL,                   \
        &&CALL,                   \
        &&NEWCLASS,                   \
        &&MOVN,                   \
        &&SLEEP,                   \
        &&TEST,                   \
        &&TNE,                   \
        &&LOCK,                   \
        &&ULOCK,                   \
        &&EXP,                   \
        &&MOVG,                   \
        &&MOVND,                   \
        &&NEWOBJARRAY,                      \
        &&NOT,                      \
        &&SKIP,                      \
        &&LOADVAL,                      \
        &&SHL,                      \
        &&SHR,                      \
        &&SKPE,                      \
        &&SKNE,                      \
        &&AND,                      \
        &&UAND,                      \
        &&OR,                      \
        &&UNOT,                      \
        &&THROW,                      \
        &&CHECKNULL,                      \
        &&RETURNOBJ,                      \
        &&NEWCLASSARRAY,                      \
        &&NEWSTRING,                      \
        &&ADDL,                      \
        &&SUBL,                      \
        &&MULL,                      \
        &&DIVL,                      \
        &&MODL,                      \
        &&IADDL,                      \
        &&ISUBL,                      \
        &&IMULL,                      \
        &&IDIVL,                      \
        &&IMODL,                      \
        &&LOADL,                      \
        &&IALOAD_2,                     \
        &&POPOBJ,                       \
        &&SMOVR,                        \
        &&ANDL,                        \
        &&ORL,                        \
        &&NOTL,                        \
        &&RMOV,                          \
        &&SMOV,                          \
        &&LOADPC_2,                      \
        &&RETURNVAL,                     \
        &&ISTORE,                        \
        &&SMOVR_2,                        \
        &&ISTOREL,                        \
        &&POPL,                           \
        &&PUSHNIL,                           \
        &&IPUSHL,                           \
        &&PUSHL,                           \
        &&ITEST,                           \
        &&INVOKE_DELEGATE,                  \
        &&INVOKE_DELEGATE_STATIC            \
    };

enum Opcode {
    op_NOP                      =0x0,
    op_INT                      =0x1,
    op_MOVI                     =0x2,
    op_RET                      =0x3,
    op_HLT                      =0x4,
    op_NEWARRAY                 =0x5,
    op_CAST                     =0x6,
    op_MOV8                     =0x7,
    op_MOV16                    =0x8,
    op_MOV32                    =0x9,
    op_MOV64                    =0xa,
    op_MOVU8                    =0xb,
    op_MOVU16                   =0xc,
    op_MOVU32                   =0xd,
    op_MOVU64                   =0xe,
    op_RSTORE                   =0xf,
    op_ADD                      =0x10,
    op_SUB                      =0x11,
    op_MUL                      =0x12,
    op_DIV                      =0x13,
    op_MOD                      =0x14,
    op_IADD                     =0x15,
    op_ISUB                     =0x16,
    op_IMUL                     =0x17,
    op_IDIV                     =0x18,
    op_IMOD                     =0x19,
    op_POP                      =0x1a,
    op_INC                      =0x1b,
    op_DEC                      =0x1c,
    op_MOVR                     =0x1d,
    op_IALOAD                   =0x1e,
    op_BRH                      =0x1f,
    op_IFE                      =0x20,
    op_IFNE                     =0x21,
    op_LT                       =0x22,
    op_GT                       =0x23,
    op_LTE                      =0x24,
    op_GTE                      =0x25,
    op_MOVL                     =0x26,
    op_MOVSL                    =0x27,
    op_MOVBI                    =0x28,
    op_SIZEOF                   =0x29,
    op_PUT                      =0x2a,
    op_PUTC                     =0x2b,
    op_CHECKLEN                 =0x2c,
    op_GOTO                     =0x2d,
    op_LOADPC                   =0x2e,
    op_PUSHOBJ                  =0x2f,
    op_DEL                      =0x30,
    op_CALL                     =0x31,
    op_NEWCLASS                 =0x32,
    op_MOVN                     =0x33,
    op_SLEEP                    =0x34,
    op_TEST                     =0x35,
    op_TNE                      =0x36,
    op_LOCK                     =0x37,
    op_ULOCK                    =0x38,
    op_EXP                      =0x39,
    op_MOVG                     =0x3a,
    op_MOVND                    =0x3b,
    op_NEWOBJARRAY              =0x3c,
    op_NOT                      =0x3d,
    op_SKIP                     =0x3e,
    op_LOADVAL                  =0x3f,
    op_SHL                      =0x40,
    op_SHR                      =0x41,
    op_SKPE                     =0x42,
    op_SKNE                     =0x43,
    op_AND                      =0x44,
    op_UAND                     =0x45,
    op_OR                       =0x46,
    op_UNOT                     =0x47,
    op_THROW                    =0x48,
    op_CHECKNULL                =0x49,
    op_RETURNOBJ                =0x4a,
    op_NEWCLASSARRAY            =0x4b,
    op_NEWSTRING                =0x4c,
    op_ADDL                     =0x4d,
    op_SUBL                     =0x4e,
    op_MULL                     =0x4f,
    op_DIVL                     =0x50,
    op_MODL                     =0x51,
    op_IADDL                    =0x52,
    op_ISUBL                    =0x53,
    op_IMULL                    =0x54,
    op_IDIVL                    =0x55,
    op_IMODL                    =0x56,
    op_LOADL                    =0x57,
    op_IALOAD_2                 =0x58,
    op_POPOBJ                   =0x59,
    op_SMOVR                    =0x5a,
    op_ANDL                     =0x5b,
    op_ORL                      =0x5c,
    op_NOTL                     =0x5d,
    op_RMOV                     =0x5e,
    op_SMOV                     =0x5f,
    op_LOADPC_2                 =0x60,
    op_RETURNVAL                =0x61,
    op_ISTORE                   =0x62,
    op_SMOVR_2                  =0x63,
    op_ISTOREL                  =0x64,
    op_POPL                     =0x65,
    op_PUSHNIL                  =0x66,
    op_IPUSHL                   =0x67,
    op_PUSHL                    =0x68,
    op_ITEST                    =0x69,
    op_INVOKE_DELEGATE          =0x6a,
    op_INVOKE_DELEGATE_STATIC   =0x6b
};

#endif //SHARP_OPCODE_H
