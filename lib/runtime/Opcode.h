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

#define DISPATCH() if(pc<cacheSize) { goto *opcode_table[GET_OP(cache[pc])]; } else { throw Exception("invalid branch/dispatch; check your assembly code?"); }

#define _brh pc++; for(int i = 0; i < 9895; i++){ i++; i++; i-=2; } goto *opcodeStart;
#define _brh_NOINCREMENT goto *opcodeStart;

#define CHECK_NULL(x) if(o2==NULL) { throw Exception(Environment::NullptrException, ""); } else { x }
#define CHECK_NULLOBJ(x) if(o2==NULL || o2->object == NULL || o2->object->size==0) { throw Exception(Environment::NullptrException, ""); } else { x }

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
        &&RETURNVAL                     \
    };

enum Opcode {
    op_NOP                 =0x0,
    op_INT                 =0x1,
    op_MOVI                =0x2,
    op_RET                 =0x3,
    op_HLT                 =0x4,
    op_NEWARRAY            =0x5,
    op_CAST                =0x6,
    op_MOV8                =0x7,
    op_MOV16               =0x8,
    op_MOV32               =0x9,
    op_MOV64               =0xa,
    op_MOVU8               =0xb,
    op_MOVU16              =0xc,
    op_MOVU32              =0xd,
    op_MOVU64              =0xe,
    op_RSTORE              =0xf,
    op_ADD                 =0x11,
    op_SUB                 =0x12,
    op_MUL                 =0x13,
    op_DIV                 =0x14,
    op_MOD                 =0x15,
    op_IADD                =0x16,
    op_ISUB                =0x17,
    op_IMUL                =0x18,
    op_IDIV                =0x19,
    op_IMOD                =0x1a,
    op_POP                 =0x1b,
    op_INC                 =0x1c,
    op_DEC                 =0x1d,
    op_MOVR                =0x1e,
    op_IALOAD              =0x1f,
    op_BRH                 =0x20,
    op_IFE                 =0x21,
    op_IFNE                =0x22,
    op_LT                  =0x23,
    op_GT                  =0x24,
    op_LTE                 =0x25,
    op_GTE                 =0x26,
    op_MOVL                =0x27,
    op_MOVSL               =0x28,
    op_MOVBI               =0x29,
    op_SIZEOF              =0x2a,
    op_PUT                 =0x2b,
    op_PUTC                =0x2c,
    op_CHECKLEN            =0x2d,
    op_GOTO                =0x2e,
    op_LOADPC              =0x2f,
    op_PUSHOBJ             =0x30,
    op_DEL                 =0x31,
    op_CALL                =0x32,
    op_NEWCLASS            =0x33,
    op_MOVN                =0x34,
    op_SLEEP               =0x35,
    op_TEST                =0x36,
    op_TNE                 =0x37,
    op_LOCK                =0x38,
    op_ULOCK               =0x39,
    op_EXP                 =0x3a,
    op_MOVG                =0x3b,
    op_MOVND               =0x3c,
    op_NEWOBJARRAY         =0x3d,
    op_NOT                 =0x3e,
    op_SKIP                =0x3f,
    op_LOADVAL             =0x40,
    op_SHL                 =0x41,
    op_SHR                 =0x42,
    op_SKPE                =0x43,
    op_SKNE                =0x44,
    op_AND                 =0x45,
    op_UAND                =0x46,
    op_OR                  =0x47,
    op_UNOT                =0x48,
    op_THROW               =0x49,
    op_CHECKNULL           =0x4a,
    op_RETURNOBJ           =0x4b,
    op_NEWCLASSARRAY       =0x4c,
    op_NEWSTRING           =0x4d,
    op_ADDL                =0x4e,
    op_SUBL                =0x4f,
    op_MULL                =0x50,
    op_DIVL                =0x51,
    op_MODL                =0x52,
    op_IADDL               =0x53,
    op_ISUBL               =0x54,
    op_IMULL               =0x55,
    op_IDIVL               =0x56,
    op_IMODL               =0x57,
    op_LOADL               =0x58,
    op_IALOAD_2            =0x59,
    op_POPOBJ              =0x5a,
    op_SMOVR               =0x5b,
    op_ANDL                =0x5c,
    op_ORL                 =0x5d,
    op_NOTL                =0x5e,
    op_RMOV                =0x5f,
    op_SMOV                =0x60,
    op_LOADPC_2            =0x61,
    op_RETURNVAL           =0x62
};

#endif //SHARP_OPCODE_H
