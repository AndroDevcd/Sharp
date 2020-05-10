//
// Created by BNunnally on 4/4/2020.
//
#include "Opcode.h"

opcode_instr tmpInstr;
opcode_instr OpBuilder::instruction_Buffer[INSTRUCTION_BUFFER_SIZE];

opcode_instr OpBuilder::nop() {
    return SET_Ei(tmpInstr, NOP);
}

opcode_instr OpBuilder::_int(interruptFlag flag) {
    return SET_Di(tmpInstr, INT, flag, POSITIVE);
}

opcode_instr OpBuilder::ill() {
    return SET_Ei(tmpInstr, ILL);
}

opcode_instr* OpBuilder::movi(opcode_arg value, _register outRegister) {
    clearBuf(instruction_Buffer, INSTRUCTION_BUFFER_SIZE);

    instruction_Buffer[0] = SET_Di(tmpInstr, MOVI, outRegister, POSITIVE);
    instruction_Buffer[1] = value;
    return instruction_Buffer;
}

void OpBuilder::clearBuf(opcode_instr *instrs, int buffLen) {
    for(int i = 0; i < buffLen; i++) {
        instrs[i] = -1;
    }
}

opcode_instr OpBuilder::ret() {
    return SET_Ei(tmpInstr, RET);
}

opcode_instr OpBuilder::hlt() {
    return SET_Ei(tmpInstr, HLT);
}

opcode_instr OpBuilder::newVarArray(_register inRegister) {
    return SET_Di(tmpInstr, NEWARRAY, inRegister, POSITIVE);
}

opcode_instr OpBuilder::cast(_register outRegister) {
    return SET_Di(tmpInstr, CAST, outRegister, POSITIVE);
}

opcode_instr OpBuilder::mov8(_register outRegister, _register registerToCast) {
    return SET_Ci(tmpInstr, MOV8, outRegister, POSITIVE, registerToCast, POSITIVE);
}

bool OpBuilder::illegalParam(opcode_arg param, instr_class iclass, short argNum) {
    switch(iclass) {
        case E_CLASS:
            return true; // e class should not have parameters
        case D_CLASS:
            if(argNum == 1)
                return !(param >= -((int32_t)DA_MAX) && param <= (int32_t)DA_MAX);
            else return true;
        case C_CLASS: {
            if(argNum == 1)
                return !(param >= -((int32_t)CA1_MAX) && param <= (int32_t)CA1_MAX);
            else if(argNum == 2)
                return !(param >= -((int32_t)CA2_MAX) && param <= (int32_t)CA2_MAX);
            else return true;
        }
        case B_CLASS:
            if(argNum >= 1 && argNum <= 3)
                return !(param >= -((int32_t)BA_MAX) && param <= (int32_t)BA_MAX);
            else return true;
    }
    return false;
}

opcode_instr OpBuilder::mov16(_register outRegister, _register registerToCast) {
    return SET_Ci(tmpInstr, MOV16, outRegister, POSITIVE, registerToCast, POSITIVE);
}

opcode_instr OpBuilder::mov32(_register outRegister, _register registerToCast) {
    return SET_Ci(tmpInstr, MOV32, outRegister, POSITIVE, registerToCast, POSITIVE);
}

opcode_instr OpBuilder::mov64(_register outRegister, _register registerToCast) {
    return SET_Ci(tmpInstr, MOV64, outRegister, POSITIVE, registerToCast, POSITIVE);
}

opcode_instr OpBuilder::movu8(_register outRegister, _register registerToCast) {
    return SET_Ci(tmpInstr, MOVU8, outRegister, POSITIVE, registerToCast, POSITIVE);
}

opcode_instr OpBuilder::movu16(_register outRegister, _register registerToCast) {
    return SET_Ci(tmpInstr, MOVU16, outRegister, POSITIVE, registerToCast, POSITIVE);
}

opcode_instr OpBuilder::movu32(_register outRegister, _register registerToCast) {
    return SET_Ci(tmpInstr, MOVU32, outRegister, POSITIVE, registerToCast, POSITIVE);
}

opcode_instr OpBuilder::movu64(_register outRegister, _register registerToCast) {
    return SET_Ci(tmpInstr, MOVU64, outRegister, POSITIVE, registerToCast, POSITIVE);
}

opcode_instr OpBuilder::rstore(_register registerToStore) {
    return SET_Di(tmpInstr, RSTORE, registerToStore, POSITIVE);
}

opcode_instr OpBuilder::add(_register outRegister, _register leftRegister, _register rightRegister) {
    return SET_Bi(tmpInstr, ADD, leftRegister, POSITIVE, rightRegister, POSITIVE, outRegister, POSITIVE);
}

opcode_instr OpBuilder::sub(_register outRegister, _register leftRegister, _register rightRegister) {
    return SET_Bi(tmpInstr, SUB, leftRegister, POSITIVE, rightRegister, POSITIVE, outRegister, POSITIVE);
}

opcode_instr OpBuilder::mul(_register outRegister, _register leftRegister, _register rightRegister) {
    return SET_Bi(tmpInstr, MUL, leftRegister, POSITIVE, rightRegister, POSITIVE, outRegister, POSITIVE);
}

opcode_instr OpBuilder::exp(_register outRegister, _register leftRegister, _register rightRegister) {
    return SET_Bi(tmpInstr, EXP, leftRegister, POSITIVE, rightRegister, POSITIVE, outRegister, POSITIVE);
}

opcode_instr OpBuilder::div(_register outRegister, _register leftRegister, _register rightRegister) {
    return SET_Bi(tmpInstr, DIV, leftRegister, POSITIVE, rightRegister, POSITIVE, outRegister, POSITIVE);
}

opcode_instr OpBuilder::mod(_register outRegister, _register leftRegister, _register rightRegister) {
    return SET_Bi(tmpInstr, MOD, leftRegister, POSITIVE, rightRegister, POSITIVE, outRegister, POSITIVE);
}

opcode_instr* OpBuilder::iadd(_register outRegister, opcode_arg value) {
    clearBuf(instruction_Buffer, INSTRUCTION_BUFFER_SIZE);
    instruction_Buffer[0] = SET_Di(tmpInstr, IADD, outRegister, POSITIVE);
    instruction_Buffer[1] = value;
    return instruction_Buffer;
}

opcode_instr* OpBuilder::isub(_register outRegister, opcode_arg value) {

    clearBuf(instruction_Buffer, INSTRUCTION_BUFFER_SIZE);
    instruction_Buffer[0] = SET_Di(tmpInstr, ISUB, outRegister, POSITIVE);
    instruction_Buffer[1] = value;
    return instruction_Buffer;
}

opcode_instr* OpBuilder::imul(_register outRegister, opcode_arg value) {

    clearBuf(instruction_Buffer, INSTRUCTION_BUFFER_SIZE);
    instruction_Buffer[0] = SET_Di(tmpInstr, IMUL, outRegister, POSITIVE);
    instruction_Buffer[1] = value;
    return instruction_Buffer;
}

opcode_instr* OpBuilder::idiv(_register outRegister, opcode_arg value) {

    clearBuf(instruction_Buffer, INSTRUCTION_BUFFER_SIZE);
    instruction_Buffer[0] = SET_Di(tmpInstr, IDIV, outRegister, POSITIVE);
    instruction_Buffer[1] = value;
    return instruction_Buffer;
}

opcode_instr* OpBuilder::imod(_register outRegister, opcode_arg value) {

    clearBuf(instruction_Buffer, INSTRUCTION_BUFFER_SIZE);
    instruction_Buffer[0] = SET_Di(tmpInstr, IMOD, outRegister, POSITIVE);
    instruction_Buffer[1] = value;
    return instruction_Buffer;
}

opcode_instr OpBuilder::pop() {
    return SET_Ei(tmpInstr, POP);
}

opcode_instr OpBuilder::inc(_register outRegister) {
    return SET_Di(tmpInstr, INC, outRegister, POSITIVE);
}

opcode_instr OpBuilder::dec(_register outRegister) {
    return SET_Di(tmpInstr, DEC, outRegister, POSITIVE);
}

opcode_instr OpBuilder::movr(_register outRegister, _register inRegister) {
    return SET_Ci(tmpInstr, MOVR, outRegister, POSITIVE, inRegister, POSITIVE);
}

opcode_instr OpBuilder::iaload(_register outRegister, _register indexRegister) {
    return SET_Ci(tmpInstr, IALOAD, outRegister, POSITIVE, indexRegister, POSITIVE);
}

opcode_instr OpBuilder::brh() {
    return SET_Ei(tmpInstr, BRH);
}

opcode_instr OpBuilder::ife() {
    return SET_Ei(tmpInstr, IFE);
}

opcode_instr OpBuilder::ifne() {
    return SET_Ei(tmpInstr, IFNE);
}

opcode_instr OpBuilder::lt(_register leftRegister, _register rightRegister) {
    return SET_Ci(tmpInstr, LT, leftRegister, POSITIVE, rightRegister, POSITIVE);
}

opcode_instr OpBuilder::gt(_register leftRegister, _register rightRegister) {
    return SET_Ci(tmpInstr, GT, leftRegister, POSITIVE, rightRegister, POSITIVE);
}

opcode_instr OpBuilder::le(_register leftRegister, _register rightRegister) {
    return SET_Ci(tmpInstr, LTE, leftRegister, POSITIVE, rightRegister, POSITIVE);
}

opcode_instr OpBuilder::ge(_register leftRegister, _register rightRegister) {
    return SET_Ci(tmpInstr, GTE, leftRegister, POSITIVE, rightRegister, POSITIVE);
}

opcode_instr OpBuilder::movl(opcode_arg relFrameAddress) {
    if(illegalParam(relFrameAddress, D_CLASS))
        return ill();
    return SET_Di(tmpInstr, MOVL, abs(relFrameAddress), posNeg(relFrameAddress));
}

opcode_instr OpBuilder::movsl(opcode_arg relStackAddress) {
    if(illegalParam(relStackAddress, D_CLASS))
        return ill();
    return SET_Di(tmpInstr, MOVSL, abs(relStackAddress), posNeg(relStackAddress));
}

opcode_instr OpBuilder::_sizeof(_register outRegister) {
    return SET_Di(tmpInstr, SIZEOF, outRegister, POSITIVE);
}

opcode_instr OpBuilder::put(_register inRegister) {
    return SET_Di(tmpInstr, PUT, inRegister, POSITIVE);
}

opcode_instr OpBuilder::putc(_register inRegister) {
    return SET_Di(tmpInstr, PUTC, inRegister, POSITIVE);
}

opcode_instr OpBuilder::checklen(_register inRegister) {
    return SET_Di(tmpInstr, CHECKLEN, inRegister, POSITIVE);
}

opcode_instr OpBuilder::jmp(opcode_arg address) {
    if(illegalParam(address, D_CLASS))
        return ill();
    return SET_Di(tmpInstr, JMP, abs(address), posNeg(address));
}

opcode_instr OpBuilder::loadpc(_register outRegister) {
    return SET_Di(tmpInstr, LOADPC, outRegister, POSITIVE);
}

opcode_instr OpBuilder::pushObject() {
    return SET_Ei(tmpInstr, PUSHOBJ);
}

opcode_instr OpBuilder::del() {
    return SET_Ei(tmpInstr, DEL);
}

opcode_instr OpBuilder::call(opcode_arg address) {
    if(illegalParam(address, D_CLASS))
        return ill();
    return SET_Di(tmpInstr, CALL, abs(address), posNeg(address));
}

opcode_instr OpBuilder::newClass(opcode_arg address) {
    if(illegalParam(address, D_CLASS))
        return ill();
    return SET_Di(tmpInstr, NEWCLASS, abs(address), posNeg(address));
}

opcode_instr* OpBuilder::movn(opcode_arg address) {
    clearBuf(instruction_Buffer, INSTRUCTION_BUFFER_SIZE);

    instruction_Buffer[0] = SET_Ei(tmpInstr, MOVN);
    instruction_Buffer[1] = address;
    return instruction_Buffer;
}

opcode_instr OpBuilder::sleep(_register inRegister) {
    return SET_Di(tmpInstr, SLEEP, inRegister, POSITIVE);
}

opcode_instr OpBuilder::te(_register leftRegister, _register rightRegister) {
    return SET_Ci(tmpInstr, TEST, leftRegister, POSITIVE, rightRegister, POSITIVE);
}

opcode_instr OpBuilder::tne(_register leftRegister, _register rightRegister) {
    return SET_Ci(tmpInstr, TNE, leftRegister, POSITIVE, rightRegister, POSITIVE);
}

opcode_instr OpBuilder::lock() {
    return SET_Ei(tmpInstr, LOCK);
}

opcode_instr OpBuilder::unlock() {
    return SET_Ei(tmpInstr, ULOCK);
}

opcode_instr OpBuilder::movg(opcode_arg address) {
    if(illegalParam(address, D_CLASS))
        return ill();
    return SET_Di(tmpInstr, MOVG, abs(address), posNeg(address));
}

opcode_instr OpBuilder::movnd(_register inRegister) {
    return SET_Di(tmpInstr, MOVND, inRegister, POSITIVE);
}

opcode_instr OpBuilder::newObjectArray(_register inRegister) {
    return SET_Di(tmpInstr, NEWOBJARRAY, inRegister, POSITIVE);
}

opcode_instr OpBuilder::_not(_register outRegister, _register inRegister) {
    return SET_Ci(tmpInstr, NOT, outRegister, POSITIVE, inRegister, POSITIVE);
}

opcode_instr OpBuilder::skip(opcode_arg instructionsToSkip) {
    if(illegalParam(instructionsToSkip, D_CLASS))
        return ill();
    return SET_Di(tmpInstr, SKIP, abs(instructionsToSkip), posNeg(instructionsToSkip));
}

opcode_instr OpBuilder::loadValue(_register outRegister) {
    return SET_Di(tmpInstr, LOADVAL, outRegister, POSITIVE);
}

opcode_instr OpBuilder::shl(_register outRegister, _register leftRegister, _register rightRegister) {
    return SET_Bi(tmpInstr, SHL, leftRegister, POSITIVE, rightRegister, POSITIVE, outRegister, POSITIVE);
}

opcode_instr OpBuilder::shr(_register outRegister, _register leftRegister, _register rightRegister) {
    return SET_Bi(tmpInstr, SHR, leftRegister, POSITIVE, rightRegister, POSITIVE, outRegister, POSITIVE);
}

opcode_instr OpBuilder::skipife(_register inRegister, opcode_arg address) {
    if(illegalParam(address, C_CLASS, 2))
        return ill();
    return SET_Ci(tmpInstr, SKPE, inRegister, POSITIVE, abs(address), posNeg(address));
}

opcode_instr OpBuilder::skipifne(_register inRegister, opcode_arg address) {
    if(illegalParam(address, C_CLASS, 2))
        return ill();
    return SET_Ci(tmpInstr, SKNE, inRegister, POSITIVE, abs(address), posNeg(address));
}

opcode_instr OpBuilder::_and(_register leftRegister, _register rightRegister) {
    return SET_Ci(tmpInstr, AND, leftRegister, POSITIVE, rightRegister, POSITIVE);
}

opcode_instr OpBuilder::uand(_register leftRegister, _register rightRegister) {
    return SET_Ci(tmpInstr, UAND, leftRegister, POSITIVE, rightRegister, POSITIVE);
}

opcode_instr OpBuilder::_or(_register leftRegister, _register rightRegister) {
    return SET_Ci(tmpInstr, OR, leftRegister, POSITIVE, rightRegister, POSITIVE);
}

opcode_instr OpBuilder::_xor(_register leftRegister, _register rightRegister) {
    return SET_Ci(tmpInstr, XOR, leftRegister, POSITIVE, rightRegister, POSITIVE);
}

opcode_instr OpBuilder::_throw() {
    return SET_Ei(tmpInstr, THROW);
}

opcode_instr OpBuilder::checkNull(_register outRegister) {
    return SET_Di(tmpInstr, CHECKNULL, outRegister, POSITIVE);
}

opcode_instr OpBuilder::returnObject() {
    return SET_Ei(tmpInstr, RETURNOBJ);
}

opcode_instr OpBuilder::newClassArray(_register inRegister, opcode_arg classAddress) {
    return SET_Ci(tmpInstr, NEWCLASSARRAY, inRegister, POSITIVE, abs(classAddress), posNeg(classAddress));
}

opcode_instr OpBuilder::newString(opcode_arg address) {
    if(illegalParam(address, D_CLASS))
        return ill();
    return SET_Di(tmpInstr, NEWSTRING, abs(address), posNeg(address));
}

opcode_instr OpBuilder::addl(_register inRegister, opcode_arg relFrameAddress) {
    return SET_Ci(tmpInstr, ADDL, inRegister, POSITIVE, abs(relFrameAddress), posNeg(relFrameAddress));
}

opcode_instr OpBuilder::subl(_register inRegister, opcode_arg relFrameAddress) {
    return SET_Ci(tmpInstr, SUBL, inRegister, POSITIVE, abs(relFrameAddress), posNeg(relFrameAddress));
}

opcode_instr OpBuilder::mull(_register inRegister, opcode_arg relFrameAddress) {
    return SET_Ci(tmpInstr, MULL, inRegister, POSITIVE, abs(relFrameAddress), posNeg(relFrameAddress));
}

opcode_instr OpBuilder::divl(_register inRegister, opcode_arg relFrameAddress) {
    return SET_Ci(tmpInstr, DIVL, inRegister, POSITIVE, abs(relFrameAddress), posNeg(relFrameAddress));
}

opcode_instr OpBuilder::modl(_register inRegister, opcode_arg relFrameAddress) {
    return SET_Ci(tmpInstr, MODL, inRegister, POSITIVE, abs(relFrameAddress), posNeg(relFrameAddress));
}

opcode_instr* OpBuilder::iaddl(opcode_arg value, opcode_arg relFrameAddress) {
    clearBuf(instruction_Buffer, INSTRUCTION_BUFFER_SIZE);
    if(illegalParam(relFrameAddress, D_CLASS)) {
        instruction_Buffer[0] =  ill();
        return instruction_Buffer;
    }

    instruction_Buffer[0] = SET_Di(tmpInstr, IADDL, abs(relFrameAddress), posNeg(relFrameAddress));
    instruction_Buffer[1] = value;
    return instruction_Buffer;
}

opcode_instr* OpBuilder::isubl(opcode_arg value, opcode_arg relFrameAddress) {
    clearBuf(instruction_Buffer, INSTRUCTION_BUFFER_SIZE);
    if(illegalParam(relFrameAddress, D_CLASS)) {
        instruction_Buffer[0] =  ill();
        return instruction_Buffer;
    }

    instruction_Buffer[0] = SET_Di(tmpInstr, ISUBL, abs(relFrameAddress), posNeg(relFrameAddress));
    instruction_Buffer[1] = value;
    return instruction_Buffer;
}

opcode_instr* OpBuilder::imull(opcode_arg value, opcode_arg relFrameAddress) {
    clearBuf(instruction_Buffer, INSTRUCTION_BUFFER_SIZE);
    if(illegalParam(relFrameAddress, D_CLASS)) {
        instruction_Buffer[0] =  ill();
        return instruction_Buffer;
    }

    instruction_Buffer[0] = SET_Di(tmpInstr, IMULL, abs(relFrameAddress), posNeg(relFrameAddress));
    instruction_Buffer[1] = value;
    return instruction_Buffer;
}

opcode_instr* OpBuilder::idivl(opcode_arg value, opcode_arg relFrameAddress) {
    clearBuf(instruction_Buffer, INSTRUCTION_BUFFER_SIZE);
    if(illegalParam(relFrameAddress, D_CLASS)) {
        instruction_Buffer[0] =  ill();
        return instruction_Buffer;
    }

    instruction_Buffer[0] = SET_Di(tmpInstr, IDIVL, abs(relFrameAddress), posNeg(relFrameAddress));
    instruction_Buffer[1] = value;
    return instruction_Buffer;
}

opcode_instr* OpBuilder::imodl(opcode_arg value, opcode_arg relFrameAddress) {
    clearBuf(instruction_Buffer, INSTRUCTION_BUFFER_SIZE);
    if(illegalParam(relFrameAddress, D_CLASS)) {
        instruction_Buffer[0] =  ill();
        return instruction_Buffer;
    }

    instruction_Buffer[0] = SET_Di(tmpInstr, IMODL, abs(relFrameAddress), posNeg(relFrameAddress));
    instruction_Buffer[1] = value;
    return instruction_Buffer;
}

opcode_instr OpBuilder::loadl(_register inRegister, opcode_arg relFrameAddress) {
    return SET_Ci(tmpInstr, LOADL, inRegister, POSITIVE, abs(relFrameAddress), posNeg(relFrameAddress));
}

opcode_instr OpBuilder::popObject() {
    return SET_Ei(tmpInstr, POPOBJ);
}

opcode_instr OpBuilder::smovr(_register inRegister, opcode_arg relStackAddress) {
    return SET_Ci(tmpInstr, SMOVR, inRegister, POSITIVE, abs(relStackAddress), posNeg(relStackAddress));
}

opcode_instr OpBuilder::andl(_register inRegister, opcode_arg relStackAddress) {
    return SET_Ci(tmpInstr, ANDL, inRegister, POSITIVE, abs(relStackAddress), posNeg(relStackAddress));
}

opcode_instr OpBuilder::orl(_register inRegister, opcode_arg relStackAddress) {
    return SET_Ci(tmpInstr, ORL, inRegister, POSITIVE, abs(relStackAddress), posNeg(relStackAddress));
}

opcode_instr OpBuilder::xorl(_register inRegister, opcode_arg relStackAddress) {
    return SET_Ci(tmpInstr, XORL, inRegister, POSITIVE, abs(relStackAddress), posNeg(relStackAddress));
}

opcode_instr OpBuilder::rmov(_register indexRegister, _register inRegister) {
    return SET_Ci(tmpInstr, RMOV, indexRegister, POSITIVE, inRegister, POSITIVE);
}

opcode_instr OpBuilder::smov(_register outRegister, opcode_arg relStackAddress) {
    return SET_Ci(tmpInstr, SMOV, outRegister, POSITIVE, abs(relStackAddress), posNeg(relStackAddress));
}

opcode_instr OpBuilder::returnValue(_register inRegister) {
    return SET_Di(tmpInstr, RETURNVAL, inRegister, POSITIVE);
}

opcode_instr* OpBuilder::istore(opcode_arg value) {
    clearBuf(instruction_Buffer, INSTRUCTION_BUFFER_SIZE);

    instruction_Buffer[0] = SET_Ei(tmpInstr, ISTORE);
    instruction_Buffer[1] = value;
    return instruction_Buffer;
}

opcode_instr OpBuilder::smovr2(_register inRegister, opcode_arg relFrameAddress) {
    return SET_Ci(tmpInstr, SMOVR_2, inRegister, POSITIVE, abs(relFrameAddress), posNeg(relFrameAddress));
}

opcode_instr OpBuilder::smovr3(opcode_arg relFrameAddress) {
    return SET_Di(tmpInstr, SMOVR_3, abs(relFrameAddress), posNeg(relFrameAddress));
}

opcode_instr* OpBuilder::istorel(opcode_arg relFrameAddress, opcode_arg integerValue) {
    clearBuf(instruction_Buffer, INSTRUCTION_BUFFER_SIZE);
    if(illegalParam(relFrameAddress, D_CLASS)) {
        instruction_Buffer[0] = ill();
        return instruction_Buffer;
    }

    instruction_Buffer[0] = SET_Di(tmpInstr, ISTOREL, abs(relFrameAddress), posNeg(relFrameAddress));
    instruction_Buffer[1] = integerValue;
    return instruction_Buffer;
}

opcode_instr OpBuilder::popl(opcode_arg relFrameAddress) {
    if(illegalParam(relFrameAddress, D_CLASS))
        return ill();
    return SET_Di(tmpInstr, POPL, abs(relFrameAddress), posNeg(relFrameAddress));
}

opcode_instr OpBuilder::pushNull() {
    return SET_Ei(tmpInstr, PUSHNULL);
}

opcode_instr OpBuilder::ipushl(opcode_arg relFrameAddress) {
    if(illegalParam(relFrameAddress, D_CLASS))
        return ill();
    return SET_Di(tmpInstr, IPUSHL, abs(relFrameAddress), posNeg(relFrameAddress));
}

opcode_instr OpBuilder::pushl(opcode_arg relFrameAddress) {
    if(illegalParam(relFrameAddress, D_CLASS))
        return ill();
    return SET_Di(tmpInstr, PUSHL, abs(relFrameAddress), posNeg(relFrameAddress));
}

opcode_instr OpBuilder::itest(_register outRegister) {
    return SET_Di(tmpInstr, ITEST, outRegister, POSITIVE);
}

opcode_instr* OpBuilder::invokeDelegate(opcode_arg address, opcode_arg argCount, bool isFunctionStatic) {
    clearBuf(instruction_Buffer, INSTRUCTION_BUFFER_SIZE);
    if(illegalParam(address, D_CLASS) ||
        illegalParam(argCount, C_CLASS, 2)) {
        instruction_Buffer[0] = ill();
        return instruction_Buffer;
    }

    instruction_Buffer[0] = SET_Di(tmpInstr, INVOKE_DELEGATE, abs(address), posNeg(address));
    instruction_Buffer[1] = SET_Ci(tmpInstr, ILL, (isFunctionStatic ? 1 : 0), POSITIVE, abs(argCount), posNeg(argCount));
    return instruction_Buffer;
}

CXX11_INLINE
uint8_t OpBuilder::posNeg(opcode_arg data) {
    return data >= 0 ? POSITIVE : NEGATIVE;
}

opcode_instr OpBuilder::get(_register outRegister) {
    return SET_Di(tmpInstr, GET, outRegister, POSITIVE);
}

opcode_instr* OpBuilder::isadd(opcode_arg relStackAddress, opcode_arg value) {
    clearBuf(instruction_Buffer, INSTRUCTION_BUFFER_SIZE);
    if(illegalParam(relStackAddress, D_CLASS)) {
        instruction_Buffer[0] = ill();
        return instruction_Buffer;
    }

    instruction_Buffer[0] = SET_Di(tmpInstr, ISADD, abs(relStackAddress), posNeg(relStackAddress));
    instruction_Buffer[1] = value;
    return instruction_Buffer;
}

opcode_instr OpBuilder::je(opcode_arg address) {
    if(illegalParam(address, D_CLASS))
        return ill();
    return SET_Di(tmpInstr, JE, abs(address), posNeg(address));
}

opcode_instr OpBuilder::jne(opcode_arg address) {
    if(illegalParam(address, D_CLASS))
        return ill();
    return SET_Di(tmpInstr, JNE, abs(address), posNeg(address));
}

opcode_instr OpBuilder::ipopl(opcode_arg relFrameAddress) {
    if(illegalParam(relFrameAddress, D_CLASS))
        return ill();
    return SET_Di(tmpInstr, IPOPL, abs(relFrameAddress), posNeg(relFrameAddress));
}

opcode_instr* OpBuilder::_switch(opcode_arg value) {
    clearBuf(instruction_Buffer, INSTRUCTION_BUFFER_SIZE);

    instruction_Buffer[0] = SET_Ei(tmpInstr, SWITCH);
    instruction_Buffer[1] = value;
    return instruction_Buffer;
}


opcode_instr *OpBuilder::cmp(_register inRegister, opcode_arg value) {
    clearBuf(instruction_Buffer, INSTRUCTION_BUFFER_SIZE);

    instruction_Buffer[0] = SET_Di(tmpInstr, CMP, inRegister, POSITIVE);
    instruction_Buffer[1] = value;

    return instruction_Buffer;
}

opcode_instr OpBuilder::calld(_register inRegister) {
    return SET_Di(tmpInstr, CALLD, inRegister, POSITIVE);
}

opcode_instr OpBuilder::varCast(opcode_arg varType, bool isArray) {
    if(illegalParam(varType, C_CLASS))
        return ill();
    return SET_Ci(tmpInstr, VARCAST, varType, posNeg(varType), (isArray ? 1 : 0), POSITIVE);
}

opcode_instr OpBuilder::tlsMovl(opcode_arg address) {
    if(illegalParam(address, D_CLASS))
        return ill();
    return SET_Di(tmpInstr, TLS_MOVL, abs(address), posNeg(address));
}

opcode_instr OpBuilder::dup() {
    return SET_Ei(tmpInstr, DUP);
}

opcode_instr OpBuilder::popObject2() {
    return SET_Ei(tmpInstr, POPOBJ_2);
}

opcode_instr OpBuilder::swap() {
    return SET_Ei(tmpInstr, SWAP);
}

opcode_instr OpBuilder::ldc(_register outRegister, opcode_arg address) {
    if(illegalParam(address, D_CLASS))
        return ill();
    return SET_Ci(tmpInstr, LDC, outRegister, POSITIVE, abs(address), posNeg(address));
}

opcode_instr OpBuilder::neg(_register outRegister, _register inRegister) {
    return SET_Ci(tmpInstr, NEG, outRegister, POSITIVE, inRegister, POSITIVE);
}
