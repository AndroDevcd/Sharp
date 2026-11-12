//
// Created by bknun on 9/16/2022.
//

#ifndef SHARP_OPCODE_H
#define SHARP_OPCODE_H

#include "../vm_interrupt_flag.h"
#include "../vm_register.h"
#include "../../../stdimports.h"

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
    // SLOTS OF A CLASS INSTANCE" in lib/runtime/memory/sharp_object.h). These two
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
        static opcode_instr calld(_register inRegister);
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
