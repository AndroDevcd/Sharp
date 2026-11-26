//
// Created by bknun on 9/16/2022.
//

#ifndef SHARP_OPCODE_MACROS_H
#define SHARP_OPCODE_MACROS_H

#include "../../debugger/hooks.h"     // what the debugger build (sdb) adds; empty in the plain VM

/**
 * 32-Bit layout
 *
 *
 * Class E Instruction
    | 00000000 00000000 0000000  | 00000000 |  max: FF
    |	       ignored			|  opcode  |
    max 255 min 0

   Class D Instruction
    | 00000000 00000000 0000000  | 0 | 00000000 |  max: 7FFFFF
    |	       argument			| +-|  opcode  |
    max 8388607 min -8388607

    Class C Instruction
    | 00000000 00000000 00 |   0000   | 0 | | 0 | 00000000 | max: 3FFFF & F
    |       argument	  | argument | +-| | +-|  opcode  |
    arg 1 max 15 min -15
    arg 2 max 262143 min -262143

    Class B Instruction
    |  0000000  |  0000000  |  0000000  | 0 | | 0 | | 0 | 00000000 | max: 7F
    | argument |  argument |  argument  | +-| | +-| | +-|  opcode  |
    max 127 min -127
 */

#define OPCODE_MASK 0xffu

#define POSITIVE 1u
#define NEGATIVE 0u
#define ERR_STATE 1
#define NO_ERR 0

#define DA_MAX 0xFFFFFFu
#define CA1_MAX 0xFu
#define CA2_MAX 0x3FFFFu
#define BA_MAX 0x7Fu

#define SET_Ei(i, op) (i=op)
#define SET_Di(i, op, a1, n) (i=((op) | ((n & POSITIVE) << 8u) | ((uint32_t)a1 << 9u)))
#define SET_Ci(i, op, a1, n, a2, n2) (i=((op) | ((n & POSITIVE) << 8u) | ((n2 & POSITIVE) << 9u) | ((uint8_t)a1 << 10u) | ((uint32_t)a2 << 14u)))
#define SET_Bi(i, op, a1, n, a2, n2, a3, n3) (i=((op) | ((n & POSITIVE) << 8u) | ((n2 & POSITIVE) << 9u) | ((n3 & POSITIVE) << 10u) | ((uint8_t)a1 << 11u) | ((uint8_t)a2 << 18u) | ((uint8_t)a3 << 25u)))

#define GET_OP(i) (i & OPCODE_MASK)
#define GET_Da(i) (int32_t)(((i >> 8u) & POSITIVE) ? (i >> 9u) : (-1 * (i >> 9u)))
#define GET_Ca(i) (int32_t)(((i >> 8u) & POSITIVE) ? (i >> 10u & CA1_MAX) : (-1 * (i >> 10u & CA1_MAX)))
#define GET_Cb(i) (int32_t)(((i >> 9u) & POSITIVE) ? (i >> 14u) : (-1 * (i >> 14u)))
#define GET_Ba(i) (int32_t)(((i >> 8u) & POSITIVE) ? (i >> 11u & BA_MAX) : (-1 * (i >> 11u & BA_MAX)))
#define GET_Bb(i) (int32_t)(((i >> 9u) & POSITIVE) ? (i >> 18u & BA_MAX) : (-1 * (i >> 18u & BA_MAX)))
#define GET_Bc(i) (int32_t)(((i >> 10u) & POSITIVE) ? (i >> 25u & BA_MAX) : (-1 * (i >> 25u & BA_MAX)))

// TELEMETRY_OPCODE counts the instruction about to run when SHARP_TELEMETRY is
// defined and expands to nothing otherwise (see runtime/telemetry/telemetry.h)
// SDB_BEFORE_INSTRUCTION is the debugger hook: nothing in the plain VM (debugger/hooks.h)
#define DISPATCH() TELEMETRY_OPCODE(GET_OP(*task->pc)) SDB_BEFORE_INSTRUCTION(thread, task) goto *opcode_table[GET_OP(*task->pc)];
#ifdef VM_DEBUG
#define branch task->pc++; goto run;
#define branch_for(x) task->pc+=x; goto run;
#else
#define branch  task->pc++; DISPATCH()
#define branch_for(x)  task->pc+=x; DISPATCH()
#endif

#define single_arg GET_Da(*task->pc)
#define dual_arg1 GET_Ca(*task->pc)
#define dual_arg2 GET_Cb(*task->pc)
#define triple_arg1 GET_Ba(*task->pc)
#define triple_arg2 GET_Bb(*task->pc)
#define triple_arg3 GET_Bc(*task->pc)
#define raw_arg2 (int32_t)*(task->pc+1)
#define dual_raw_arg1 GET_Ca(*(task->pc+1))
#define dual_raw_arg2 GET_Cb(*(task->pc+1))

#define pop_stack_object (task->sp--)->obj
#define push_stack_object (++task->sp)->obj
#define push_stack_number (++task->sp)->var
#define pop_stack_number (task->sp--)->var
#define grow_stack_for(n) \
     if(((task->sp-task->stack)+(n)) >= task->stackSize) task->growStack((n));
#define grow_stack \
     grow_stack_for(1)
#define stack_overflow_check_for(n)  \
    if(((task->sp-task->stack)+(n)) >= task->stackLimit) throw vm_exception(vm.stack_overflow_except, "");
#define stack_overflow_check  \
    stack_overflow_check_for(1)

#define check_state(x) \
    task->pc += (x); \
    goto state_check;

#define require_object(code) \
    if(task->ptr==NULL) { \
        vm_exception err(vm.nullptr_except, ""); \
        enable_exception_flag(thread, true); \
        goto catch_exception;   \
    } else { code }

#define require_object_with_value(code) \
    if(task->ptr==NULL || task->ptr->o == NULL) { \
        vm_exception err(vm.nullptr_except, ""); \
        enable_exception_flag(thread, true); \
        goto catch_exception;   \
    } else { code }

#define require_numeric_object_with_value(code) \
    if(task->ptr==NULL || task->ptr->o == NULL || task->ptr->o->type > type_var) { \
        vm_exception err(vm.nullptr_except, ""); \
        enable_exception_flag(thread, true); \
        goto catch_exception;   \
    } else { code }

// ---- the slot forms (opcode.h, a4 .. f0): operands are frame slots ----
// SLOT(n)                      the number in frame slot n
// slot_jump(condition)         the two-word jumps: go to the second word's pc when the condition is
//                              what the instruction's third operand asks for (1: holds, 0: does not)
// slot_element(array, index, require, code)
//                              the array in slot `array`, the position in slot `index`: the object
//                              pointer is put on the array (as `movl` would), null and the position
//                              are checked, then `code` runs with the position in `data`
#define SLOT(n) ((task->fp + (n))->var)
#define slot_jump(condition) \
if((condition) == (triple_arg3 != 0)) { \
    loop_back_edge(raw_arg2) \
    task->pc = task->rom + raw_arg2; \
    check_state(0) \
} else { branch_for(2) }
// the array in slot `array`, the index in slot `index`: `code` runs with the index in `data`
#define slot_element(array, index, require, code) \
task->ptr = &(task->fp + (array))->obj; \
data = (task->fp + (index))->var.i; \
require( \
    if(data < task->ptr->o->size && data >= 0) { code } \
    else { \
        stringstream ss; \
        ss << "Access to Object at: " << data << " size is " << task->ptr->o->size; \
        throw vm_exception(vm.bounds_except, ss.str()); \
    } \
)

#define current_pc \
    (task->pc-task->rom)

#define setupOpcodeTable \
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
            &&DDIV,                   \
            &&MOD,                   \
            &&IADD,                   \
            &&ISUB,                   \
            &&IMUL,                   \
            &&DIDIV,                   \
            &&IMOD,                   \
            &&POP,                   \
            &&INC,                   \
            &&DEC,                   \
            &&MOVR,                   \
            &&BRH,                   \
            &&IFE,                   \
            &&IFNE,                   \
            &&LT,                   \
            &&GT,                   \
            &&LTE,                   \
            &&GTE,                   \
            &&MOVL,                   \
            &&MOVSL,                   \
            &&SIZEOF,                   \
            &&PUT,                   \
            &&PUTC,                   \
            &&CHECKLEN,                   \
            &&JMP,                     \
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
            &&XOR,                      \
            &&THROW,                      \
            &&CHECKNULL,                      \
            &&RETURNOBJ,                      \
            &&NEWCLASSARRAY,                      \
            &&NEWSTRING,                      \
            &&ADDL,                      \
            &&SUBL,                      \
            &&MULL,                      \
            &&DDIVL,                      \
            &&MODL,                      \
            &&IADDL,                      \
            &&ISUBL,                      \
            &&IMULL,                      \
            &&DIDIVL,                      \
            &&IMODL,                      \
            &&LOADL,                      \
            &&POPOBJ,                       \
            &&SMOVR,                        \
            &&ANDL,                        \
            &&ORL,                        \
            &&XORL,                        \
            &&SMOV,                          \
            &&RETURNVAL,                     \
            &&ISTORE,                        \
            &&SMOVR_2,                        \
            &&ISTOREL,                        \
            &&POPL,                           \
            &&PUSHNULL,                           \
            &&IPUSHL,                           \
            &&PUSHL,                           \
            &&ITEST,                           \
            &&INVOKE_DELEGATE,                  \
            &&GET,                                 \
            &&ISADD,                                 \
            &&JE,                                 \
            &&JNE,                                 \
            &&IPOPL,                                 \
            &&CMP,                                 \
            &&CALLD,                                 \
            &&VARCAST,                                 \
            &&TLS_MOVL,                                 \
            &&DUP,                                      \
            &&POPOBJ_2,                                  \
            &&SWAP,                                     \
            &&LDC,                                     \
            &&SMOVR_3,                        \
            &&NEG,                             \
            &&DEXP,                               \
            &&IS,                                   \
            &&MOV_ABS,                                   \
            &&LOAD_ABS,                                   \
            &&SMOVR_4,                                   \
            &&DADD, \
            &&DSUB, \
            &&DMUL, \
            &&DNEG, \
            &&DINC, \
            &&DDEC, \
            &&DLT, \
            &&DGT, \
            &&DLTE, \
            &&DGTE, \
            &&DTEST, \
            &&DTNE, \
            &&DNOT, \
            &&DCMP, \
            &&DIADD, \
            &&DISUB, \
            &&DIMUL, \
            &&DADDL, \
            &&DSUBL, \
            &&DMULL, \
            &&DIADDL, \
            &&DISUBL, \
            &&DIMULL, \
            &&DISADD, \
            &&DISTORE, \
            &&DISTOREL, \
            &&DMOVI, \
            &&DPUT, \
            &&I2D, \
            &&D2I, \
            &&GETF, \
            &&PUTF, \
            &&ALOAD8, \
            &&ALOADU8, \
            &&ALOAD16, \
            &&ALOADU16, \
            &&ALOAD32, \
            &&ALOADU32, \
            &&ALOAD64, \
            &&ASTORE8, \
            &&ASTORE16, \
            &&ASTORE32, \
            &&ASTORE64, \
            &&ADD_S, \
            &&SUB_S, \
            &&MUL_S, \
            &&AND_S, \
            &&OR_S, \
            &&XOR_S, \
            &&SHL_S, \
            &&SHR_S, \
            &&DADD_S, \
            &&DSUB_S, \
            &&DMUL_S, \
            &&DDIV_S, \
            &&I2D_S, \
            &&D2I_S, \
            &&JLT_S, \
            &&JGT_S, \
            &&JLE_S, \
            &&JGE_S, \
            &&JEQ_S, \
            &&JNE_S, \
            &&DJLT_S, \
            &&DJGT_S, \
            &&DJLE_S, \
            &&DJGE_S, \
            &&DJEQ_S, \
            &&DJNE_S, \
            &&ALOAD8_S, \
            &&ALOADU8_S, \
            &&ALOAD16_S, \
            &&ALOADU16_S, \
            &&ALOAD32_S, \
            &&ALOADU32_S, \
            &&ALOAD64_S, \
            &&ASTORE8_S, \
            &&ASTORE16_S, \
            &&ASTORE32_S, \
            &&ASTORE64_S, \
            &&GETF_S, \
            &&PUTF_S, \
            &&SIZEOF_S, \
            &&AGETOBJ_S, \
            &&GETOBJ_S, \
            &&JNZ_S, \
            &&JZ_S, \
            &&JNULL_S, \
            &&ISNULL_S, \
            &&NOT_S, \
            &&MOV8_S, \
            &&MOV16_S, \
            &&MOV32_S, \
            &&MOVU8_S, \
            &&MOVU16_S, \
            &&MOVU32_S, \
            &&RETURNVAL_S, \
            &&RETURNOBJ_S, \
            &&IADD_S, \
            &&ISUB_S, \
            &&IMUL_S, \
            &&PUTOBJ_S, \
            &&MOVOBJ_S, \
            &&IAND_S, \
            &&IOR_S, \
            &&IXOR_S, \
            &&ISHL_S, \
            &&ISHR_S, \
            &&JLT_K, \
            &&JGT_K, \
            &&JLE_K, \
            &&JGE_K, \
            &&JEQ_K, \
            &&JNE_K, \
            &&DJLT_K, \
            &&DJGT_K, \
            &&DJLE_K, \
            &&DJGE_K, \
            &&DJEQ_K, \
            &&DJNE_K, \
            &&DSQRT, \
            &&DFLOOR, \
            &&DCEIL, \
            &&DABS, \
            &&DMATH, \
            &&DMATH_S, \
            &&WDIV \
        };

#endif //SHARP_OPCODE_MACROS_H
