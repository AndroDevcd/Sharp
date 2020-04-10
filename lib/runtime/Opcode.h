//
// Created by BraxtonN on 2/16/2018.
//

#ifndef SHARP_OPCODE_H
#define SHARP_OPCODE_H

#include "../../stdimports.h"
#include "InterruptFlag.h"
#include "register.h"

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

#define DA_MAX 0xFFFFFFu
#define CA1_MAX 0xFu
#define CA2_MAX 0x3FFFFu
#define BA_MAX 0x7Fu

#define SET_Ei(i, op) (i=op)
#define SET_Di(i, op, a1, n) (i=((op) | ((n & POSITIVE) << 8u) | ((uint32_t)a1 << 9u)))
#define SET_Ci(i, op, a1, n, a2, n2) (i=((op) | ((n & POSITIVE) << 8u) | ((n2 & POSITIVE) << 9u) | ((uint8_t)a1 << 10u) | ((uint32_t)a2 << 14u)))
#define SET_Bi(i, op, a1, n, a2, n2, a3, n3) (i=((op) | ((n & POSITIVE) << 8u) | ((n2 & POSITIVE) << 9u) | ((n3 & POSITIVE) << 10u) | ((uint8_t)a1 << 11u) | ((uint8_t)a2 << 18u) | ((uint8_t)a3 << 25u)))

#define GET_OP(i) (i & OPCODE_MASK)
#define GET_Da(i) (((i >> 8u) & POSITIVE) ? (i >> 9u) : (-1 * (i >> 9u)))
#define GET_Ca(i) (((i >> 8u) & POSITIVE) ? (i >> 10u & CA1_MAX) : (-1 * (i >> 10u & CA1_MAX)))
#define GET_Cb(i) (((i >> 9u) & POSITIVE) ? (i >> 14u) : (-1 * (i >> 14u)))
#define GET_Ba(i) (((i >> 8u) & POSITIVE) ? (i >> 11u & BA_MAX) : (-1 * (i >> 11u & BA_MAX)))
#define GET_Bb(i) (((i >> 9u) & POSITIVE) ? (i >> 18u & BA_MAX) : (-1 * (i >> 18u & BA_MAX)))
#define GET_Bc(i) (((i >> 10u) & POSITIVE) ? (i >> 25u & BA_MAX) : (-1 * (i >> 25u & BA_MAX)))

#define DISPATCH() /*if(GET_OP(cache[pc])> op_GET) throw Exception("op"); else*/ goto *opcode_table[GET_OP(*pc)];

#define HAS_SIGNAL \
    if(signal) { \
        if (hasSignal(signal, tsig_suspend)) \
            suspendSelf(); \
        if (state == THREAD_KILLED) \
            return; \
    }

#define LONG_CALL() \
    if(current->longCalls < JIT_IR_LIMIT) \
        current->longCalls++;

#define THREAD_EXECEPT() \
    if(hasSignal(signal, tsig_except)) \
        goto exception_catch;

#define STACK_CHECK  if(((sp-dataStack)+1) >= stack_lmt) throw Exception(Environment::StackOverflowErr, "");
#define CALLSTACK_CHECK  if((calls+1) >= stack_lmt) throw Exception(Environment::StackOverflowErr, "");
#define THREAD_STACK_CHECK(self)  if(((self->sp-self->dataStack)+2) >= self->stack_lmt) throw Exception(Environment::StackOverflowErr, "");
#define THREAD_STACK_CHECK2(self, x)  if(((self->sp-self->dataStack)+2) >= self->stack_lmt || (((int64_t)(&x) - self->stfloor) <= STACK_OVERFLOW_BUF)) throw Exception(Environment::StackOverflowErr, "");

#ifndef SHARP_PROF_
#define _brh_NOINCREMENT HAS_SIGNAL if(!startAddress) DISPATCH() else goto *opcodeStart;
#else
#define _brh_NOINCREMENT SAFTEY_CHECK irCount++; if(irCount == 0) overflow++; goto *opcodeStart;
#endif
#define _brh  pc++; _brh_NOINCREMENT
#define _brh_inc(x)  pc+=x; _brh_NOINCREMENT

#define CHECK_NULL(x) if(o2==NULL) { throw Exception(Environment::NullptrException, ""); } else { x }
#define CHECK_NULL2(x) if(o2==NULL|o2->object == NULL) { throw Exception(Environment::NullptrException, ""); } else { x }
#define CHECK_NULLOBJ(x) if(o2==NULL || o2->object == NULL || TYPE(o2->object->info) != _stype_struct) { throw Exception(Environment::NullptrException, ""); } else { x }
#define CHECK_INULLOBJ(x) if(o2==NULL || o2->object == NULL || TYPE(o2->object->info) != _stype_var) { throw Exception(Environment::NullptrException, ""); } else { x }

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
            &&BRH,                   \
            &&IFE,                   \
            &&IFNE,                   \
            &&LT,                   \
            &&GT,                   \
            &&LTE,                   \
            &&GTE,                   \
            &&MOVL,                   \
            &&MOVSL,                   \
            &&MOVF,                   \
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
            &&SWITCH,                                 \
            &&CMP,                                 \
            &&CALLD,                                 \
            &&VARCAST,                                 \
            &&TLS_MOVL,                                 \
            &&DUP,                                      \
            &&POPOBJ_2,                                  \
            &&SWAP,                                     \
            &&LDC                                     \
        };

typedef unsigned int opcode_instr;
typedef int opcode_arg;
#define INSTRUCTION_BUFFER_SIZE 3

struct Opcode { // TODO: update VM and JIT with new formatting for instructions

public:
        static const uint8_t ILL         = 0xFF;
        static const uint8_t NOP         = 0x0;
        static const uint8_t INT         = 0x1;
        static const uint8_t MOVI        = 0x2;
        static const uint8_t RET         = 0x3;
        static const uint8_t HLT         = 0x4;
        static const uint8_t NEWARRAY    = 0x5;
        static const uint8_t CAST        = 0x6;
        static const uint8_t MOV8        = 0x7;
        static const uint8_t MOV16       = 0x8;
        static const uint8_t MOV32       = 0x9;
        static const uint8_t MOV64       = 0xa;
        static const uint8_t MOVU8       = 0xb;
        static const uint8_t MOVU16      = 0xc;
        static const uint8_t MOVU32      = 0xd;
        static const uint8_t MOVU64      = 0xe;
        static const uint8_t RSTORE      = 0xf;
        static const uint8_t ADD         = 0x10;
        static const uint8_t SUB         = 0x11;
        static const uint8_t MUL         = 0x12;
        static const uint8_t DIV         = 0x13;
        static const uint8_t MOD         = 0x14;
        static const uint8_t IADD        = 0x15;
        static const uint8_t ISUB        = 0x16;
        static const uint8_t IMUL        = 0x17;
        static const uint8_t IDIV        = 0x18;
        static const uint8_t IMOD        = 0x19;
        static const uint8_t POP         = 0x1a;
        static const uint8_t INC         = 0x1b;
        static const uint8_t DEC         = 0x1c;
        static const uint8_t MOVR        = 0x1d;
        static const uint8_t BRH         = 0x1e;
        static const uint8_t IFE         = 0x1f;
        static const uint8_t IFNE        = 0x20;
        static const uint8_t LT          = 0x21;
        static const uint8_t GT          = 0x22;
        static const uint8_t LTE         = 0x23;
        static const uint8_t GTE         = 0x24;
        static const uint8_t MOVL        = 0x25;
        static const uint8_t MOVSL       = 0x26;
        static const uint8_t MOVF        = 0x27;
        static const uint8_t SIZEOF      = 0x28;
        static const uint8_t PUT         = 0x29;
        static const uint8_t PUTC        = 0x2a;
        static const uint8_t CHECKLEN    = 0x2b;
        static const uint8_t GOTO        = 0x2c;
        static const uint8_t LOADPC      = 0x2d;
        static const uint8_t PUSHOBJ     = 0x2e;
        static const uint8_t DEL         = 0x2f;
        static const uint8_t CALL        = 0x30;
        static const uint8_t NEWCLASS    = 0x31;
        static const uint8_t MOVN        = 0x32;
        static const uint8_t SLEEP       = 0x33;
        static const uint8_t TEST        = 0x34;
        static const uint8_t TNE         = 0x35;
        static const uint8_t LOCK        = 0x36;
        static const uint8_t ULOCK       = 0x37;
        static const uint8_t MOVG        = 0x38;
        static const uint8_t MOVND       = 0x39;
        static const uint8_t NEWOBJARRAY = 0x3a;
        static const uint8_t NOT           = 0x3b;
        static const uint8_t SKIP          = 0x3c;
        static const uint8_t LOADVAL       = 0x3d;
        static const uint8_t SHL           = 0x3e;
        static const uint8_t SHR           = 0x3f;
        static const uint8_t SKPE          = 0x40;
        static const uint8_t SKNE          = 0x41;
        static const uint8_t AND           = 0x42;
        static const uint8_t UAND          = 0x43;
        static const uint8_t OR            = 0x44;
        static const uint8_t XOR           = 0x45;
        static const uint8_t THROW         = 0x46;
        static const uint8_t CHECKNULL     = 0x47;
        static const uint8_t RETURNOBJ     = 0x48;
        static const uint8_t NEWCLASSARRAY = 0x49;
        static const uint8_t NEWSTRING     = 0x4a;
        static const uint8_t ADDL          = 0x4b;
        static const uint8_t SUBL          = 0x4c;
        static const uint8_t MULL          = 0x4d;
        static const uint8_t DIVL          = 0x4e;
        static const uint8_t MODL          = 0x4f;
        static const uint8_t IADDL         = 0x50;
        static const uint8_t ISUBL         = 0x51;
        static const uint8_t IMULL         = 0x52;
        static const uint8_t IDIVL         = 0x53;
        static const uint8_t IMODL         = 0x54;
        static const uint8_t LOADL         = 0x55;
        static const uint8_t IALOAD        = 0x56;
        static const uint8_t POPOBJ        = 0x57;
        static const uint8_t SMOVR         = 0x58;
        static const uint8_t ANDL          = 0x59;
        static const uint8_t ORL           = 0x5a;
        static const uint8_t XORL          = 0x5b;
        static const uint8_t RMOV          = 0x5c;
        static const uint8_t SMOV          = 0x5d;
        static const uint8_t RETURNVAL     = 0x5e;
        static const uint8_t ISTORE        = 0x5f;
        static const uint8_t SMOVR_2       = 0x60;
        static const uint8_t ISTOREL       = 0x61;
        static const uint8_t POPL          = 0x62;
        static const uint8_t PUSHNULL      = 0x63;
        static const uint8_t IPUSHL        = 0x64;
        static const uint8_t PUSHL         = 0x65;
        static const uint8_t ITEST         = 0x66;
        static const uint8_t INVOKE_DELEGATE = 0x67;
        static const uint8_t GET      = 0x68;
        static const uint8_t ISADD    = 0x69;
        static const uint8_t JE       = 0x6a;
        static const uint8_t JNE      = 0x6b;
        static const uint8_t IPOPL    = 0x6c;
        static const uint8_t SWITCH   = 0x6d;
        static const uint8_t CMP      = 0x6e;
        static const uint8_t CALLD    = 0x6f;
        static const uint8_t VARCAST  = 0x70;
        static const uint8_t TLS_MOVL = 0x71;
        static const uint8_t DUP      = 0x72;
        static const uint8_t POPOBJ_2 = 0x73;
        static const uint8_t SWAP     = 0x74;
        static const uint8_t LDC      = 0x75;

        enum instr_class {
            E_CLASS,
            D_CLASS,
            C_CLASS,
            B_CLASS
        };

    struct Builder {
    public:
        static opcode_instr ill();
        static opcode_instr nop();
        static opcode_instr _int(interruptFlag flag);
        static opcode_instr* movi(opcode_arg value, _register outRegister);
        static opcode_instr ret();
        static opcode_instr hlt();
        static opcode_instr newVarArray(_register inRegister);
        static opcode_instr cast(_register outRegister);
        static opcode_instr mov8(_register outRegister, _register registerToCast);
        static opcode_instr mov16(_register outRegister, _register registerToCast);
        static opcode_instr mov32(_register outRegister, _register registerToCast);
        static opcode_instr mov64(_register outRegister, _register registerToCast);
        static opcode_instr movu8(_register outRegister, _register registerToCast);
        static opcode_instr movu16(_register outRegister, _register registerToCast);
        static opcode_instr movu32(_register outRegister, _register registerToCast);
        static opcode_instr movu64(_register outRegister, _register registerToCast);
        static opcode_instr rstore(_register registerToCast);
        static opcode_instr add(_register outRegister, _register leftRegister, _register rightRegister);
        static opcode_instr sub(_register outRegister, _register leftRegister, _register rightRegister);
        static opcode_instr mul(_register outRegister, _register leftRegister, _register rightRegister);
        static opcode_instr div(_register outRegister, _register leftRegister, _register rightRegister);
        static opcode_instr mod(_register outRegister, _register leftRegister, _register rightRegister);
        static opcode_instr* iadd(_register outRegister, opcode_arg value);
        static opcode_instr* isub(_register outRegister, opcode_arg value);
        static opcode_instr* imul(_register outRegister, opcode_arg value);
        static opcode_instr* idiv(_register outRegister, opcode_arg value);
        static opcode_instr* imod(_register outRegister, opcode_arg value);
        static opcode_instr pop();
        static opcode_instr inc(_register outRegister);
        static opcode_instr dec(_register outRegister);
        static opcode_instr movr(_register outRegister, _register inRegister);
        static opcode_instr iaload(_register outRegister, _register inRegister);
        static opcode_instr brh();
        static opcode_instr ife();
        static opcode_instr ifne();
        static opcode_instr lt(_register leftRegister, _register rightRegister);
        static opcode_instr gt(_register leftRegister, _register rightRegister);
        static opcode_instr le(_register leftRegister, _register rightRegister);
        static opcode_instr ge(_register leftRegister, _register rightRegister);
        static opcode_instr movl(opcode_arg relFrameAddress);
        static opcode_instr movsl(opcode_arg relStackAddress);
        static opcode_instr movf(_register outRegister, opcode_arg floatingPtrAddress);
        static opcode_instr _sizeof(_register outRegister);
        static opcode_instr put(_register inRegister);
        static opcode_instr putc(_register inRegister);
        static opcode_instr checklen(_register inRegister);
        static opcode_instr _goto(opcode_arg address);
        static opcode_instr loadpc(_register outRegister);
        static opcode_instr pushObject();
        static opcode_instr del();
        static opcode_instr call(opcode_arg address);
        static opcode_instr newClass(opcode_arg address);
        static opcode_instr* movn(opcode_arg address);
        static opcode_instr sleep(_register inRegister);
        static opcode_instr test(_register leftRegister, _register rightRegister);
        static opcode_instr tne(_register leftRegister, _register rightRegister);
        static opcode_instr lock();
        static opcode_instr unlock();
        static opcode_instr movg(opcode_arg address);
        static opcode_instr movnd(_register inRegister);
        static opcode_instr newObjectArray(_register inRegister);
        static opcode_instr _not(_register outRegister, _register inRegister);
        static opcode_instr skip(opcode_arg instructionsToSkip);
        static opcode_instr loadValue(_register outRegister);
        static opcode_instr shl(_register outRegister, _register leftRegister, _register rightRegister);
        static opcode_instr shr(_register outRegister, _register leftRegister, _register rightRegister);
        static opcode_instr skipife(opcode_arg address);
        static opcode_instr skipifne(opcode_arg address);
        static opcode_instr _and(_register leftRegister, _register rightRegister);
        static opcode_instr uand(_register leftRegister, _register rightRegister);
        static opcode_instr _or(_register leftRegister, _register rightRegister);
        static opcode_instr _xor(_register leftRegister, _register rightRegister);
        static opcode_instr _throw();
        static opcode_instr checkNull();
        static opcode_instr returnObject();
        static opcode_instr newClassArray(_register inRegister, opcode_arg classAddress);
        static opcode_instr newString(opcode_arg address);
        static opcode_instr addl(_register inRegister, opcode_arg relFrameAddress);
        static opcode_instr subl(_register inRegister, opcode_arg relFrameAddress);
        static opcode_instr mull(_register inRegister, opcode_arg relFrameAddress);
        static opcode_instr divl(_register inRegister, opcode_arg relFrameAddress);
        static opcode_instr modl(_register inRegister, opcode_arg relFrameAddress);
        static opcode_instr* iaddl(opcode_arg value, opcode_arg relFrameAddress);
        static opcode_instr* isubl(opcode_arg value, opcode_arg relFrameAddress);
        static opcode_instr* imull(opcode_arg value, opcode_arg relFrameAddress);
        static opcode_instr* idivl(opcode_arg value, opcode_arg relFrameAddress);
        static opcode_instr* imodl(opcode_arg value, opcode_arg relFrameAddress);
        static opcode_instr loadl(_register inRegister, opcode_arg relFrameAddress);
        static opcode_instr popObject();
        static opcode_instr smovr(_register inRegister, opcode_arg relStackAddress);
        static opcode_instr andl(_register inRegister, opcode_arg relStackAddress);
        static opcode_instr orl(_register inRegister, opcode_arg relStackAddress);
        static opcode_instr xorl(_register inRegister, opcode_arg relStackAddress);
        static opcode_instr rmov(_register indexRegister, _register inRegister);
        static opcode_instr smov(_register outRegister, opcode_arg relStackAddress);
        static opcode_instr returnValue(_register inRegister);
        static opcode_instr* istore(opcode_arg value);
        static opcode_instr smovr2(_register inRegister, opcode_arg relFrameAddress);
        static opcode_instr istorel(opcode_arg relFrameAddress);
        static opcode_instr popl(opcode_arg relFrameAddress);
        static opcode_instr pushNull();
        static opcode_instr ipushl(opcode_arg relFrameAddress);
        static opcode_instr pushl(opcode_arg relFrameAddress);
        static opcode_instr itest(_register outRegister);
        static opcode_instr* invokeDelegate(opcode_arg address, opcode_arg argCount, bool isFunctionStatic);
        static opcode_instr get(_register outRegister);
        static opcode_instr* isadd(opcode_arg relStackAddress, opcode_arg value);
        static opcode_instr je(opcode_arg address);
        static opcode_instr jne(opcode_arg address);
        static opcode_instr ipopl(opcode_arg relFrameAddress);
        static opcode_instr* _switch(opcode_arg value);
        static opcode_instr *cmp(_register inRegister, opcode_arg value);
        static opcode_instr calld(_register inRegister);
        static opcode_instr varCast(opcode_arg varType, bool isArray);
        static opcode_instr tlsMovl(opcode_arg address);
        static opcode_instr dup();
        static opcode_instr popObject2();
        static opcode_instr swap();
        static opcode_instr ldc(_register outRegister, opcode_arg address);

        static bool illegalParam(opcode_arg param, instr_class iclass, short argNum = 1);

    private:
        static opcode_instr instruction_Buffer[INSTRUCTION_BUFFER_SIZE];
        static void clearBuf(opcode_instr instrs[], int bufLen);
        CXX11_INLINE static uint8_t posNeg(opcode_arg data);
    };
};

typedef Opcode::Builder OpBuilder;

/*
 * Quick way to generate the new address values for instructions
	static String hexChars = "0123456789abcdef";
	static void printHex() {
	    int decNum = 0;
	    int indexCount = 0;

	    for(long i = 0; i < 500; i++) {
	        if(indexCount >= hexChars.length()) {
	            indexCount = 0;
	            decNum++;
	        }

	        if(decNum > 0)
	            System.out.print("" + decNum);

		    System.out.println("" + hexChars.charAt(indexCount));
	        indexCount++;
	    }
	}
 *
 * A quick way to test opcode encoding
 * int declOp = Opcode::POPL;
    opcode_arg declArg1 = -127;
    opcode_arg declArg2 = -127;
    opcode_arg declArg3 = -127;

    int tmpInstr= 0;
    int instr = 0, opcode=0, arg1=0, arg2=0,arg3=0;
    instr = SET_Bi(tmpInstr, declOp, abs(declArg1), NEGATIVE, abs(declArg2), NEGATIVE, abs(declArg3), NEGATIVE);
    opcode = GET_OP(instr);
    arg1 = GET_Ba(instr);
    arg2 = GET_Bb(instr);
    arg3 = GET_Bc(instr);

    if(opcode == declOp) cout << "opcode is correct" << endl;
    if(arg1 == declArg1) cout << "arg1 is correct" << endl;
    if(arg2 == declArg2) cout << "arg2 is correct" << endl;
    if(arg3 == declArg3) cout << "arg3 is correct" << endl;

    instr = 0;
 */

#endif //SHARP_OPCODE_H
