//
// Created by bknun on 9/16/2022.
//

#ifndef SHARP_OPCODE_MACROS_H
#define SHARP_OPCODE_MACROS_H

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

#define DISPATCH() goto *opcode_table[GET_OP(*task->pc)];
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
            &&DIVL,                      \
            &&MODL,                      \
            &&IADDL,                      \
            &&ISUBL,                      \
            &&IMULL,                      \
            &&IDIVL,                      \
            &&IMODL,                      \
            &&LOADL,                      \
            &&IALOAD,                     \
            &&POPOBJ,                       \
            &&SMOVR,                        \
            &&ANDL,                        \
            &&ORL,                        \
            &&XORL,                        \
            &&RMOV,                          \
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
            &&EXP,                               \
            &&IS,                                   \
            &&MOV_ABS,                                   \
            &&LOAD_ABS,                                   \
            &&ILOAD,                                   \
            &&SMOVR_4,                                   \
            &&IMOV                                   \
        };

#endif //SHARP_OPCODE_MACROS_H
