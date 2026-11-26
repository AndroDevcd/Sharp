//
// Created by bknun on 9/16/2022.
//

#ifndef SHARP_OPCODE_H
#define SHARP_OPCODE_H

#include "../interrupts/vm_interrupt_flag.h"
#include "../types/vm_register.h"
#include "../stdimports.h"

typedef unsigned int opcode_instr;
typedef int opcode_arg;
#define INSTRUCTION_BUFFER_SIZE 2


struct Opcode {

public:
    static const uint8_t ILL         = 0xFF;
    static const uint8_t NOP         = 0x00;
    static const uint8_t INT         = 0x01;
    static const uint8_t MOVI        = 0x02;
    static const uint8_t RET         = 0x03;
    static const uint8_t HLT         = 0x04;
    static const uint8_t NEWARRAY    = 0x05;
    static const uint8_t CAST        = 0x06;
    static const uint8_t MOV8        = 0x07;
    static const uint8_t MOV16       = 0x08;
    static const uint8_t MOV32       = 0x09;
    static const uint8_t MOV64       = 0x0a;
    static const uint8_t MOVU8       = 0x0b;
    static const uint8_t MOVU16      = 0x0c;
    static const uint8_t MOVU32      = 0x0d;
    static const uint8_t MOVU64      = 0x0e;
    static const uint8_t RSTORE      = 0x0f;
    static const uint8_t ADD         = 0x10;
    static const uint8_t SUB         = 0x11;
    static const uint8_t MUL         = 0x12;
    static const uint8_t DDIV         = 0x13;
    static const uint8_t MOD         = 0x14;
    static const uint8_t IADD        = 0x15;
    static const uint8_t ISUB        = 0x16;
    static const uint8_t IMUL        = 0x17;
    static const uint8_t DIDIV        = 0x18;
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
    static const uint8_t SIZEOF      = 0x27;
    static const uint8_t PUT         = 0x28;
    static const uint8_t PUTC        = 0x29;
    static const uint8_t CHECKLEN    = 0x2a;
    static const uint8_t JMP         = 0x2b;
    static const uint8_t LOADPC      = 0x2c;
    static const uint8_t PUSHOBJ     = 0x2d;
    static const uint8_t DEL         = 0x2e;
    static const uint8_t CALL        = 0x2f;
    static const uint8_t NEWCLASS    = 0x30;
    static const uint8_t MOVN        = 0x31;
    static const uint8_t SLEEP       = 0x32;
    static const uint8_t TEST        = 0x33;
    static const uint8_t TNE         = 0x34;
    static const uint8_t LOCK        = 0x35;
    static const uint8_t ULOCK       = 0x36;
    static const uint8_t MOVG        = 0x37;
    static const uint8_t MOVND       = 0x38;
    static const uint8_t NEWOBJARRAY = 0x39;
    static const uint8_t NOT           = 0x3a;
    static const uint8_t SKIP          = 0x3b;
    static const uint8_t LOADVAL       = 0x3c;
    static const uint8_t SHL           = 0x3d;
    static const uint8_t SHR           = 0x3e;
    static const uint8_t SKPE          = 0x3f;
    static const uint8_t SKNE          = 0x40;
    static const uint8_t AND           = 0x41;
    static const uint8_t UAND          = 0x42;
    static const uint8_t OR            = 0x43;
    static const uint8_t XOR           = 0x44;
    static const uint8_t THROW         = 0x45;
    static const uint8_t CHECKNULL     = 0x46;
    static const uint8_t RETURNOBJ     = 0x47;
    static const uint8_t NEWCLASSARRAY = 0x48;
    static const uint8_t NEWSTRING     = 0x49;
    static const uint8_t ADDL          = 0x4a;
    static const uint8_t SUBL          = 0x4b;
    static const uint8_t MULL          = 0x4c;
    static const uint8_t DDIVL          = 0x4d;
    static const uint8_t MODL          = 0x4e;
    static const uint8_t IADDL         = 0x4f;
    static const uint8_t ISUBL         = 0x50;
    static const uint8_t IMULL         = 0x51;
    static const uint8_t DIDIVL         = 0x52;
    static const uint8_t IMODL         = 0x53;
    static const uint8_t LOADL         = 0x54;
    static const uint8_t POPOBJ        = 0x55;
    static const uint8_t SMOVR         = 0x56;
    static const uint8_t ANDL          = 0x57;
    static const uint8_t ORL           = 0x58;
    static const uint8_t XORL          = 0x59;
    static const uint8_t SMOV          = 0x5a;
    static const uint8_t RETURNVAL     = 0x5b;
    static const uint8_t ISTORE        = 0x5c;
    static const uint8_t SMOVR_2       = 0x5d;
    static const uint8_t ISTOREL       = 0x5e;
    static const uint8_t POPL          = 0x5f;
    static const uint8_t PUSHNULL      = 0x60;
    static const uint8_t IPUSHL        = 0x61;
    static const uint8_t PUSHL         = 0x62;
    static const uint8_t ITEST         = 0x63;
    static const uint8_t INVOKE_DELEGATE = 0x64;
    static const uint8_t GET          = 0x65;
    static const uint8_t ISADD        = 0x66;
    static const uint8_t JE           = 0x67;
    static const uint8_t JNE          = 0x68;
    static const uint8_t IPOPL        = 0x69;
    static const uint8_t CMP          = 0x6a;
    static const uint8_t CALLD        = 0x6b;
    static const uint8_t VARCAST      = 0x6c;
    static const uint8_t TLS_MOVL     = 0x6d;
    static const uint8_t DUP          = 0x6e;
    static const uint8_t POPOBJ_2     = 0x6f;
    static const uint8_t SWAP         = 0x70;
    static const uint8_t LDC          = 0x71;
    static const uint8_t SMOVR_3      = 0x72;
    static const uint8_t NEG          = 0x73;
    static const uint8_t DEXP          = 0x74;
    static const uint8_t IS           = 0x75;
    static const uint8_t MOV_ABS      = 0x76;
    static const uint8_t LOAD_ABS     = 0x77;
    static const uint8_t SMOVR_4      = 0x78;

    // ---- typed numbers ---------------------------------------------------------
    // A number is either an INTEGER (64-bit, wraps on overflow) or a DECIMAL
    // (64-bit double). Moving a number (register, local, stack, field) is the
    // same for both, so those instructions have one form. Arithmetic and
    // comparison are not: the instructions above this block (ADD, LT, ...) work
    // on integers, the D... instructions below are their decimal twins, and
    // I2D / D2I convert a register from one to the other. The compiler knows the
    // type of every value and picks the instruction; the VM never has to ask.
    //
    // Division and ** exist for decimals only (DDIV, DIDIV, DDIVL, DIDIVL, DEXP):
    // in Sharp `7 / 2` is 3.5.
    static const uint8_t DADD         = 0x79;
    static const uint8_t DSUB         = 0x7a;
    static const uint8_t DMUL         = 0x7b;
    static const uint8_t DNEG         = 0x7c;
    static const uint8_t DINC         = 0x7d;
    static const uint8_t DDEC         = 0x7e;
    static const uint8_t DLT          = 0x7f;
    static const uint8_t DGT          = 0x80;
    static const uint8_t DLTE         = 0x81;
    static const uint8_t DGTE         = 0x82;
    static const uint8_t DTEST        = 0x83;
    static const uint8_t DTNE         = 0x84;
    static const uint8_t DNOT         = 0x85;
    static const uint8_t DCMP         = 0x86;
    static const uint8_t DIADD        = 0x87;
    static const uint8_t DISUB        = 0x88;
    static const uint8_t DIMUL        = 0x89;
    static const uint8_t DADDL        = 0x8a;
    static const uint8_t DSUBL        = 0x8b;
    static const uint8_t DMULL        = 0x8c;
    static const uint8_t DIADDL       = 0x8d;
    static const uint8_t DISUBL       = 0x8e;
    static const uint8_t DIMULL       = 0x8f;
    static const uint8_t DISADD       = 0x90;
    static const uint8_t DISTORE      = 0x91;
    static const uint8_t DISTOREL     = 0x92;
    static const uint8_t DMOVI        = 0x93;
    static const uint8_t DPUT         = 0x94;
    static const uint8_t I2D          = 0x95;
    static const uint8_t D2I          = 0x96;
    // ---- number fields ---------------------------------------------------------
    // A number field is stored in the slot of the object that has it (see "THE
    // SLOTS OF A CLASS INSTANCE" in runtime/memory/sharp_object.h). These two
    // copy a register from / to slot <b> of the object in the object pointer.
    // The copy is the same for an integer and a decimal field.
    static const uint8_t GETF         = 0x97;
    static const uint8_t PUTF         = 0x98;
    static const uint8_t ALOAD8       = 0x99;
    static const uint8_t ALOADU8      = 0x9a;
    static const uint8_t ALOAD16      = 0x9b;
    static const uint8_t ALOADU16     = 0x9c;
    static const uint8_t ALOAD32      = 0x9d;
    static const uint8_t ALOADU32     = 0x9e;
    static const uint8_t ALOAD64      = 0x9f;
    static const uint8_t ASTORE8      = 0xa0;
    static const uint8_t ASTORE16     = 0xa1;
    static const uint8_t ASTORE32     = 0xa2;
    static const uint8_t ASTORE64     = 0xa3;

    // ---- THE SLOT FORMS (0xa4 .. 0xf0, in three groups): one instruction that reads its operands from the function's
    // frame slots and writes its result into one, where the forms above need a load per operand,
    // the operation, and a store (the compiler's IR is exactly this shape: compiler, bytecode step).
    // Operands are frame slots (fp + n), three of 7 bits each (class B), so n <= 127; a function
    // with more slots than that uses the older forms for the slots out of reach.
    //
    //      ADD_S d, a, b               fp[d] = fp[a] + fp[b]            (SUB MUL AND OR XOR SHL SHR; D...: decimals)
    //      I2D_S d, a   D2I_S d, a     fp[d] = (double) fp[a] / decimal_to_integer(fp[a])
    //      JLT_S a, b, sense; target   if((fp[a] < fp[b]) == sense) goto target     (two words; DJ...: decimals)
    //      ALOAD8_S d, array, index    fp[d] = array[index], null and bounds checked  (the array is fp[array].obj)
    //      ASTORE8_S array, index, v   array[index] = fp[v]
    //      GETF_S d, object, field     fp[d] = the number field of the object         PUTF_S object, field, v
    //      SIZEOF_S d, array           fp[d] = its length, 0 for null
    //      AGETOBJ_S d, array, index   fp[d].obj = array[index] (an array of references)
    //      GETOBJ_S d, object, field   fp[d].obj = the reference field of the object
    static const uint8_t ADD_S        = 0xa4;
    static const uint8_t SUB_S        = 0xa5;
    static const uint8_t MUL_S        = 0xa6;
    static const uint8_t AND_S        = 0xa7;
    static const uint8_t OR_S         = 0xa8;
    static const uint8_t XOR_S        = 0xa9;
    static const uint8_t SHL_S        = 0xaa;
    static const uint8_t SHR_S        = 0xab;
    static const uint8_t DADD_S       = 0xac;
    static const uint8_t DSUB_S       = 0xad;
    static const uint8_t DMUL_S       = 0xae;
    static const uint8_t DDIV_S       = 0xaf;
    static const uint8_t I2D_S        = 0xb0;
    static const uint8_t D2I_S        = 0xb1;
    static const uint8_t JLT_S        = 0xb2;
    static const uint8_t JGT_S        = 0xb3;
    static const uint8_t JLE_S        = 0xb4;
    static const uint8_t JGE_S        = 0xb5;
    static const uint8_t JEQ_S        = 0xb6;
    static const uint8_t JNE_S        = 0xb7;
    static const uint8_t DJLT_S       = 0xb8;
    static const uint8_t DJGT_S       = 0xb9;
    static const uint8_t DJLE_S       = 0xba;
    static const uint8_t DJGE_S       = 0xbb;
    static const uint8_t DJEQ_S       = 0xbc;
    static const uint8_t DJNE_S       = 0xbd;
    static const uint8_t ALOAD8_S     = 0xbe;
    static const uint8_t ALOADU8_S    = 0xbf;
    static const uint8_t ALOAD16_S    = 0xc0;
    static const uint8_t ALOADU16_S   = 0xc1;
    static const uint8_t ALOAD32_S    = 0xc2;
    static const uint8_t ALOADU32_S   = 0xc3;
    static const uint8_t ALOAD64_S    = 0xc4;
    static const uint8_t ASTORE8_S    = 0xc5;
    static const uint8_t ASTORE16_S   = 0xc6;
    static const uint8_t ASTORE32_S   = 0xc7;
    static const uint8_t ASTORE64_S   = 0xc8;
    static const uint8_t GETF_S       = 0xc9;
    static const uint8_t PUTF_S       = 0xca;
    static const uint8_t SIZEOF_S     = 0xcb;
    static const uint8_t AGETOBJ_S    = 0xcc;
    static const uint8_t GETOBJ_S     = 0xcd;
    // the second group: what still went through registers in ordinary code
    //      JNZ_S a; target   JZ_S a; target        if(fp[a] != 0) / if(fp[a] == 0) goto target
    //      JNULL_S a, 0, sense; target             if((fp[a].obj is null) == sense) goto target
    //      ISNULL_S d, a     NOT_S d, a            fp[d] = fp[a].obj is null / !fp[a]
    //      MOV8_S d, a  ...  MOVU32_S d, a         fp[d] = fp[a] cut to that width
    //      RETURNVAL_S a     RETURNOBJ_S a         the function's result = fp[a] / fp[a].obj
    //      IADD_S d, a; k    ISUB_S   IMUL_S       fp[d] = fp[a] + k   (k: the second word)
    //      PUTOBJ_S object, field, v               (fp[object].obj).<reference field> = fp[v].obj, with the write barrier
    //      MOVOBJ_S d, a                           fp[d].obj = fp[a].obj
    static const uint8_t JNZ_S        = 0xce;
    static const uint8_t JZ_S         = 0xcf;
    static const uint8_t JNULL_S      = 0xd0;
    static const uint8_t ISNULL_S     = 0xd1;
    static const uint8_t NOT_S        = 0xd2;
    static const uint8_t MOV8_S       = 0xd3;
    static const uint8_t MOV16_S      = 0xd4;
    static const uint8_t MOV32_S      = 0xd5;
    static const uint8_t MOVU8_S      = 0xd6;
    static const uint8_t MOVU16_S     = 0xd7;
    static const uint8_t MOVU32_S     = 0xd8;
    static const uint8_t RETURNVAL_S  = 0xd9;
    static const uint8_t RETURNOBJ_S  = 0xda;
    static const uint8_t IADD_S       = 0xdb;
    static const uint8_t ISUB_S       = 0xdc;
    static const uint8_t IMUL_S       = 0xdd;
    static const uint8_t PUTOBJ_S     = 0xde;
    static const uint8_t MOVOBJ_S     = 0xdf;
    // the third group: a constant in the instruction, where the second operand would be
    //      IAND_S d, a; k    IOR_S  IXOR_S  ISHL_S  ISHR_S     fp[d] = fp[a] & k        (k: the second word)
    //      JLT_K a, k, sense; target   ... JNE_K               if((fp[a] < k) == sense) goto target   (k: -127 .. 127)
    //      DJLT_K a, k, sense; target  ... DJNE_K              the same for a decimal fp[a]
    static const uint8_t IAND_S       = 0xe0;
    static const uint8_t IOR_S        = 0xe1;
    static const uint8_t IXOR_S       = 0xe2;
    static const uint8_t ISHL_S       = 0xe3;
    static const uint8_t ISHR_S       = 0xe4;
    static const uint8_t JLT_K        = 0xe5;
    static const uint8_t JGT_K        = 0xe6;
    static const uint8_t JLE_K        = 0xe7;
    static const uint8_t JGE_K        = 0xe8;
    static const uint8_t JEQ_K        = 0xe9;
    static const uint8_t JNE_K        = 0xea;
    static const uint8_t DJLT_K       = 0xeb;
    static const uint8_t DJGT_K       = 0xec;
    static const uint8_t DJLE_K       = 0xed;
    static const uint8_t DJGE_K       = 0xee;
    static const uint8_t DJEQ_K       = 0xef;
    static const uint8_t DJNE_K       = 0xf0;
    // ---- the math instructions ---------------------------------------------------
    // What std.math's functions are made of. The first four the processor does itself, and
    // the JITs use its instruction where there is one; DMATH calls the C library's function.
    //      DSQRT a, b          regs[a] = sqrt(regs[b]), both decimals (not a number for a negative one)
    //      DFLOOR a, b         regs[a] = the largest whole number that is <= regs[b]
    //      DCEIL a, b          regs[a] = the smallest whole number that is >= regs[b]
    //      DABS a, b           regs[a] = regs[b] without its sign
    //      DMATH a, b, c       regs[a] = function number c of regs[b] - and of regs[EGX], for the
    //                          functions of two numbers. c is one of the numbers of __cmath
    //                          (runtime/interrupts/native/math_functions.cpp): 0 sin, 1 cos ... 0xd pow ... 0x13 fmod.
    //                          A number that is no function gives "not a number".
    // (`int 0x18` does the same as DMATH with the function number in a register: for code that
    //  only knows which function at run time.)
    static const uint8_t DSQRT        = 0xf1;
    static const uint8_t DFLOOR       = 0xf2;
    static const uint8_t DCEIL        = 0xf3;
    static const uint8_t DABS         = 0xf4;
    static const uint8_t DMATH        = 0xf5;
    //      DMATH_S a, b, c     the slot form: fp[a] = function number c of fp[b], for the functions of ONE
    //                          number (sqrt, floor, ceil and abs are numbers too: 0xe, 0x10, 0xf, 0x11).
    //                          The compiler writes this one; the four above are for `asm` blocks.
    static const uint8_t DMATH_S      = 0xf6;
    //      WDIV a, b, c        regs[c] = regs[a] / regs[b] on whole numbers: the quotient, toward zero
    //                          (-7 / 2 is -3). A divisor of 0 raises invalid_operation_exception.
    static const uint8_t WDIV         = 0xf7;

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
        static opcode_instr _int(interrupt_flag flag);
        static opcode_instr* movi(opcode_arg value, _register outRegister);
        static opcode_instr* dmovi(opcode_arg value, _register outRegister);
        static opcode_instr ret(opcode_arg errState);
        static opcode_instr hlt();
        static opcode_instr newVarArray(_register inRegister, unsigned short ntype);
        static opcode_instr cast(opcode_arg classAddress);
        static opcode_instr mov8(_register outRegister, _register registerToCast);
        static opcode_instr mov16(_register outRegister, _register registerToCast);
        static opcode_instr mov32(_register outRegister, _register registerToCast);
        static opcode_instr mov64(_register outRegister, _register registerToCast);
        static opcode_instr d2i(_register outRegister, _register registerToCast);
        static opcode_instr i2d(_register outRegister, _register registerToCast);
        static opcode_instr movu8(_register outRegister, _register registerToCast);
        static opcode_instr movu16(_register outRegister, _register registerToCast);
        static opcode_instr movu32(_register outRegister, _register registerToCast);
        static opcode_instr movu64(_register outRegister, _register registerToCast);
        static opcode_instr rstore(_register registerToCast);
        static opcode_instr add(_register outRegister, _register leftRegister, _register rightRegister);
        static opcode_instr dadd(_register outRegister, _register leftRegister, _register rightRegister);
        static opcode_instr sub(_register outRegister, _register leftRegister, _register rightRegister);
        static opcode_instr dsub(_register outRegister, _register leftRegister, _register rightRegister);
        static opcode_instr mul(_register outRegister, _register leftRegister, _register rightRegister);
        static opcode_instr dmul(_register outRegister, _register leftRegister, _register rightRegister);
        static opcode_instr dexp(_register outRegister, _register leftRegister, _register rightRegister);
        static opcode_instr ddiv(_register outRegister, _register leftRegister, _register rightRegister);
        static opcode_instr mod(_register outRegister, _register leftRegister, _register rightRegister);
        static opcode_instr* iadd(_register outRegister, opcode_arg value);
        static opcode_instr* diadd(_register outRegister, opcode_arg value);
        static opcode_instr* isub(_register outRegister, opcode_arg value);
        static opcode_instr* disub(_register outRegister, opcode_arg value);
        static opcode_instr* imul(_register outRegister, opcode_arg value);
        static opcode_instr* dimul(_register outRegister, opcode_arg value);
        static opcode_instr* didiv(_register outRegister, opcode_arg value);
        static opcode_instr* imod(_register outRegister, opcode_arg value);
        static opcode_instr pop();
        static opcode_instr inc(_register outRegister);
        static opcode_instr dinc(_register outRegister);
        static opcode_instr dec(_register outRegister);
        static opcode_instr ddec(_register outRegister);
        static opcode_instr movr(_register outRegister, _register inRegister);
        static opcode_instr aload64(_register outRegister, _register indexRegister);
        static opcode_instr aloadu32(_register outRegister, _register indexRegister);
        static opcode_instr aload32(_register outRegister, _register indexRegister);
        static opcode_instr aloadu16(_register outRegister, _register indexRegister);
        static opcode_instr aload16(_register outRegister, _register indexRegister);
        static opcode_instr aloadu8(_register outRegister, _register indexRegister);
        static opcode_instr aload8(_register outRegister, _register indexRegister);
        static opcode_instr brh();
        static opcode_instr ife();
        static opcode_instr ifne();
        static opcode_instr lt(_register leftRegister, _register rightRegister);
        static opcode_instr dlt(_register leftRegister, _register rightRegister);
        static opcode_instr gt(_register leftRegister, _register rightRegister);
        static opcode_instr dgt(_register leftRegister, _register rightRegister);
        static opcode_instr le(_register leftRegister, _register rightRegister);
        static opcode_instr dlte(_register leftRegister, _register rightRegister);
        static opcode_instr ge(_register leftRegister, _register rightRegister);
        static opcode_instr dgte(_register leftRegister, _register rightRegister);
        static opcode_instr movl(opcode_arg relFrameAddress);
        static opcode_instr movsl(opcode_arg relStackAddress);
        static opcode_instr _sizeof(_register outRegister);
        static opcode_instr put(_register inRegister);
        static opcode_instr dput(_register inRegister);
        static opcode_instr putc(_register inRegister);
        static opcode_instr checklen(_register inRegister);
        static opcode_instr jmp(opcode_arg address);
        static opcode_instr movabs(opcode_arg address);
        static opcode_instr loadabs(opcode_arg address);
        static opcode_instr loadpc(_register outRegister);
        static opcode_instr pushObject();
        static opcode_instr del();
        static opcode_instr call(opcode_arg address);
        static opcode_instr* newClass(opcode_arg address);
        static opcode_instr* movn(opcode_arg address);
        static opcode_instr sleep(_register inRegister);
        static opcode_instr te(_register leftRegister, _register rightRegister);
        static opcode_instr dtest(_register leftRegister, _register rightRegister);
        static opcode_instr tne(_register leftRegister, _register rightRegister);
        static opcode_instr dtne(_register leftRegister, _register rightRegister);
        static opcode_instr lock();
        static opcode_instr unlock();
        static opcode_instr movg(opcode_arg address);
        static opcode_instr movnd(_register inRegister);
        static opcode_instr newObjectArray(_register inRegister);
        static opcode_instr _not(_register outRegister, _register inRegister);
        static opcode_instr dnot(_register outRegister, _register inRegister);
        static opcode_instr skip(opcode_arg instructionsToSkip);
        static opcode_instr loadValue(_register outRegister);
        static opcode_instr shl(_register outRegister, _register leftRegister, _register rightRegister);
        static opcode_instr shr(_register outRegister, _register leftRegister, _register rightRegister);
        static opcode_instr skipife(_register inRegister, opcode_arg address);
        static opcode_instr skipifne(_register inRegister, opcode_arg address);
        static opcode_instr _and(_register leftRegister, _register rightRegister);
        static opcode_instr uand(_register leftRegister, _register rightRegister);
        static opcode_instr _or(_register leftRegister, _register rightRegister);
        static opcode_instr _xor(_register leftRegister, _register rightRegister);
        static opcode_instr _throw();
        static opcode_instr checkNull(_register outRegister);
        static opcode_instr returnObject();
        static opcode_instr* newClassArray(_register inRegister, opcode_arg classAddress);
        static opcode_instr newString(opcode_arg address);
        static opcode_instr addl(_register inRegister, opcode_arg relFrameAddress);
        static opcode_instr daddl(_register inRegister, opcode_arg relFrameAddress);
        static opcode_instr subl(_register inRegister, opcode_arg relFrameAddress);
        static opcode_instr dsubl(_register inRegister, opcode_arg relFrameAddress);
        static opcode_instr mull(_register inRegister, opcode_arg relFrameAddress);
        static opcode_instr dmull(_register inRegister, opcode_arg relFrameAddress);
        static opcode_instr ddivl(_register inRegister, opcode_arg relFrameAddress);
        static opcode_instr modl(_register inRegister, opcode_arg relFrameAddress);
        static opcode_instr* iaddl(opcode_arg value, opcode_arg relFrameAddress);
        static opcode_instr* diaddl(opcode_arg value, opcode_arg relFrameAddress);
        static opcode_instr* isubl(opcode_arg value, opcode_arg relFrameAddress);
        static opcode_instr* disubl(opcode_arg value, opcode_arg relFrameAddress);
        static opcode_instr* imull(opcode_arg value, opcode_arg relFrameAddress);
        static opcode_instr* dimull(opcode_arg value, opcode_arg relFrameAddress);
        static opcode_instr* didivl(opcode_arg value, opcode_arg relFrameAddress);
        static opcode_instr* imodl(opcode_arg value, opcode_arg relFrameAddress);
        static opcode_instr loadl(_register inRegister, opcode_arg relFrameAddress);
        static opcode_instr getf(_register outRegister, opcode_arg fieldSlot);
        static opcode_instr popObject();
        static opcode_instr smovr(_register inRegister, opcode_arg relStackAddress);
        static opcode_instr andl(_register inRegister, opcode_arg relStackAddress);
        static opcode_instr orl(_register inRegister, opcode_arg relStackAddress);
        static opcode_instr xorl(_register inRegister, opcode_arg relStackAddress);
        static opcode_instr astore64(_register indexRegister, _register inRegister);
        static opcode_instr astore32(_register indexRegister, _register inRegister);
        static opcode_instr astore16(_register indexRegister, _register inRegister);
        static opcode_instr astore8(_register indexRegister, _register inRegister);
        static opcode_instr smov(_register outRegister, opcode_arg relStackAddress);
        static opcode_instr returnValue(_register inRegister);
        static opcode_instr* istore(opcode_arg value);
        static opcode_instr* distore(opcode_arg value);
        static opcode_instr smovr2(_register inRegister, opcode_arg relFrameAddress);
        static opcode_instr putf(_register inRegister, opcode_arg fieldSlot);
        static opcode_instr smovr3(opcode_arg relFrameAddress);
        static opcode_instr* smovr4(opcode_arg relFrameAddress1, opcode_arg relFrameAddress2);
        static opcode_instr* istorel(opcode_arg relFrameAddress, opcode_arg integerValue);
        static opcode_instr* distorel(opcode_arg relFrameAddress, opcode_arg integerValue);
        static opcode_instr popl(opcode_arg relFrameAddress);
        static opcode_instr pushNull();
        static opcode_instr ipushl(opcode_arg relFrameAddress);
        static opcode_instr pushl(opcode_arg relFrameAddress);
        static opcode_instr itest(_register outRegister);
        static opcode_instr* invokeDelegate(opcode_arg address, opcode_arg argCount, bool isFunctionStatic);
        static opcode_instr get(_register outRegister);
        static opcode_instr* isadd(opcode_arg relStackAddress, opcode_arg value);
        static opcode_instr* disadd(opcode_arg relStackAddress, opcode_arg value);
        static opcode_instr je(opcode_arg address);
        static opcode_instr jne(opcode_arg address);
        static opcode_instr ipopl(opcode_arg relFrameAddress);
        static opcode_instr* cmp(_register inRegister, opcode_arg value);
        static opcode_instr* dcmp(_register inRegister, opcode_arg value);
        static opcode_instr calld(opcode_arg argumentCount);    // call the function value under that many arguments
        static opcode_instr varCast(opcode_arg varType, bool isArray);
        static opcode_instr tlsMovl(opcode_arg address);
        static opcode_instr dup();
        static opcode_instr popObject2();
        static opcode_instr swap();
        static opcode_instr ldc(_register outRegister, opcode_arg address);
        static opcode_instr neg(_register outRegister, _register inRegister);
        static opcode_instr dneg(_register outRegister, _register inRegister);
        static opcode_instr* is(_register outRegister, opcode_arg type);

        static bool illegalParam(opcode_arg param, instr_class iclass, short argNum = 1);
        static uint8_t posNeg(opcode_arg data);

    private:
        static opcode_instr instruction_Buffer[INSTRUCTION_BUFFER_SIZE];
        static void clearBuf(opcode_instr instrs[], int bufLen);
    };
};

typedef Opcode::Builder OpBuilder;

#endif //SHARP_OPCODE_H
