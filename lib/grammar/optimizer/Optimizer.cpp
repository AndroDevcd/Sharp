//
// Created by BNunnally on 5/25/2020.
//

#include "Optimizer.h"

/**
 * [0xb] 11:	movr ebx, cmt
 * [0xc] 12:	movr cmt, ebx
 *
 * to -> nothing
 */
void Optimizer::optimizeRedundantMovr() {
    CodeHolder &code = currentMethod->data.code;
    for(Int i = 0; i < code.size(); i++) {
        switch (GET_OP(code.ir32.get(i))) {
            case Opcode::MOVI:
            case Opcode::IADD:
            case Opcode::ISUB:
            case Opcode::IMUL:
            case Opcode::IDIV:
            case Opcode::IMOD:
            case Opcode::IADDL:
            case Opcode::ISUBL:
            case Opcode::IMULL:
            case Opcode::IDIVL:
            case Opcode::IMODL:
            case Opcode::ISTORE:
            case Opcode::ISTOREL:
            case Opcode::ISADD:
            case Opcode::CMP:
            case Opcode::MOVN:
            case Opcode::INVOKE_DELEGATE:
                i++;
                break;
            case Opcode::MOVR: {

                if(GET_OP(code.ir32.get(i + 1)) == Opcode::MOVR) {
                    Int inReg, outReg;
                    outReg = GET_Ca(code.ir32.get(i));
                    inReg = GET_Cb(code.ir32.get(i));

                    if(GET_Ca(code.ir32.get(i + 1)) == inReg
                        && GET_Cb(code.ir32.get(i + 1)) == outReg) {
                        shiftAddresses(2, i);
                        code.ir32.removeAt(i);
                        code.ir32.removeAt(i);
                    }
                }
                break;
            }
        }
    }
}


/**
 * [0xb] 11:	movi #0, ebx
 * [0xc] 12:	smovr_2 ebx, fp+1
 *
 * to ->        istorel 0, fp+1
 */
void Optimizer::optimizeLocalVarInit() {
    CodeHolder &code = currentMethod->data.code;
    for(Int i = 0; i < code.size(); i++) {
        switch (GET_OP(code.ir32.get(i))) {
            case Opcode::IADD:
            case Opcode::ISUB:
            case Opcode::IMUL:
            case Opcode::IDIV:
            case Opcode::IMOD:
            case Opcode::IADDL:
            case Opcode::ISUBL:
            case Opcode::IMULL:
            case Opcode::IDIVL:
            case Opcode::IMODL:
            case Opcode::ISTORE:
            case Opcode::ISTOREL:
            case Opcode::ISADD:
            case Opcode::CMP:
            case Opcode::MOVN:
            case Opcode::INVOKE_DELEGATE:
                i++;
                break;
            case Opcode::MOVI: {

                if((i + 2) < code.ir32.size()) {
                    if (GET_OP(code.ir32.get(i + 2)) == Opcode::SMOVR_2) {
                        Int outReg = GET_Da(code.ir32.get(i));
                        Int value = code.ir32.get(i + 1);

                        if (GET_Ca(code.ir32.get(i + 2)) == outReg) {
                            Int frameAddress = GET_Cb(code.ir32.get(i + 2));

                            shiftAddresses(1, i + 1);
                            code.ir32.removeAt(i);

                            code.ir32.get(i) = SET_Di(tmpInstr, Opcode::ISTOREL,
                                                      abs(frameAddress), OpBuilder::posNeg(frameAddress));
                            code.ir32.get(i + 1) = value;
                            i += 1;
                        } else i++;
                    } else i++;
                }
                break;
            }
        }
    }
}


/**
 * [0xb] 11:	loadl ebx, fp+1
 * [0xc] 12:	rstore ebx
 *
 * to ->        ipushl fp+1
 */
void Optimizer::optimizeLocalStackPush() {
    CodeHolder &code = currentMethod->data.code;
    for(Int i = 0; i < code.size(); i++) {
        switch (GET_OP(code.ir32.get(i))) {
            case Opcode::MOVI:
            case Opcode::IADD:
            case Opcode::ISUB:
            case Opcode::IMUL:
            case Opcode::IDIV:
            case Opcode::IMOD:
            case Opcode::IADDL:
            case Opcode::ISUBL:
            case Opcode::IMULL:
            case Opcode::IDIVL:
            case Opcode::IMODL:
            case Opcode::ISTORE:
            case Opcode::ISTOREL:
            case Opcode::ISADD:
            case Opcode::CMP:
            case Opcode::MOVN:
            case Opcode::INVOKE_DELEGATE:
                i++;
                break;
            case Opcode::LOADL: {

                if((i + 1) < code.ir32.size()) {
                    if (GET_OP(code.ir32.get(i + 1)) == Opcode::RSTORE) {
                        Int outReg = GET_Ca(code.ir32.get(i));
                        Int frameAddress = GET_Cb(code.ir32.get(i));

                        if (GET_Da(code.ir32.get(i + 1)) == outReg) {
                            shiftAddresses(1, i + 1);
                            code.ir32.removeAt(i);

                            code.ir32.get(i) = OpBuilder::ipushl(frameAddress);
                        }
                    }
                }
                break;
            }
        }
    }
}


/**
 * [0xb] 11:	loadl ebx, fp+1
 * [0xc] 12:	movr egx, ebx
 * [0xd] 13:	inc ebx
 * [0xe] 14:	smovr_2 ebx, fp+1
 *
 * to ->        iaddl 1, fp+1
 */
void Optimizer::optimizeLocalVariableIncrement() {
    CodeHolder &code = currentMethod->data.code;
    for(Int i = 0; i < code.size(); i++) {
        switch (GET_OP(code.ir32.get(i))) {
            case Opcode::MOVI:
            case Opcode::IADD:
            case Opcode::ISUB:
            case Opcode::IMUL:
            case Opcode::IDIV:
            case Opcode::IMOD:
            case Opcode::IADDL:
            case Opcode::ISUBL:
            case Opcode::IMULL:
            case Opcode::IDIVL:
            case Opcode::IMODL:
            case Opcode::ISTORE:
            case Opcode::ISTOREL:
            case Opcode::ISADD:
            case Opcode::CMP:
            case Opcode::MOVN:
            case Opcode::INVOKE_DELEGATE:
                i++;
                break;

            case Opcode::LOADL: {

                if((i + 3) < code.ir32.size()) {
                    if (GET_OP(code.ir32.get(i + 1)) == Opcode::MOVR
                        && GET_OP(code.ir32.get(i + 2)) == Opcode::INC
                        && GET_OP(code.ir32.get(i + 3)) == Opcode::SMOVR_2) {
                        Int outReg = GET_Ca(code.ir32.get(i));
                        Int frameAddress = GET_Cb(code.ir32.get(i));

                        Int oldValReg = GET_Ca(code.ir32.get(i + 1));
                        Int movedReg = GET_Cb(code.ir32.get(i + 1));

                        if (outReg == movedReg && outReg == GET_Da(code.ir32.get(i + 2))
                            && outReg == GET_Ca(code.ir32.get(i + 3))) {
                            if(!isRegisterUsed((_register)oldValReg, i + 3)) {
                                shiftAddresses(2, i + 1);
                                code.ir32.removeAt(i);
                                code.ir32.removeAt(i);

                                opcode_instr *buf = OpBuilder::iaddl(1, frameAddress);
                                code.ir32.get(i) = buf[0];
                                code.ir32.get(i + 1) = buf[1];
                                i++;
                            }
                        }
                    }
                }
                break;
            }
        }
    }
}


/**
 * [0xb] 11:	ipushl #1
 * [0xc] 12:	movi #10000000, ebx
 * [0xd] 13:	loadval ecx
 * [0xe] 14:	lt ecx, ebx
 *
 * to ->        loadl ecx, fp+1
 *              movi #10000000, ebx
 *              lt ecx, ebx
 */
void Optimizer::optimizeRedundantIntegerPush() {
    CodeHolder &code = currentMethod->data.code;
    for(Int i = 0; i < code.size(); i++) {
        switch (GET_OP(code.ir32.get(i))) {
            case Opcode::MOVI:
            case Opcode::IADD:
            case Opcode::ISUB:
            case Opcode::IMUL:
            case Opcode::IDIV:
            case Opcode::IMOD:
            case Opcode::IADDL:
            case Opcode::ISUBL:
            case Opcode::IMULL:
            case Opcode::IDIVL:
            case Opcode::IMODL:
            case Opcode::ISTORE:
            case Opcode::ISTOREL:
            case Opcode::ISADD:
            case Opcode::CMP:
            case Opcode::MOVN:
            case Opcode::INVOKE_DELEGATE:
                i++;
                break;

            case Opcode::IPUSHL: {

                if((i + 3) < code.ir32.size()) {
                    if (GET_OP(code.ir32.get(i + 1)) == Opcode::MOVI
                        && GET_OP(code.ir32.get(i + 3)) == Opcode::LOADVAL
                        && GET_OP(code.ir32.get(i + 4)) == Opcode::LT) {
                        Int frameAddress = GET_Da(code.ir32.get(i));

                        Int rightIntValue = code.ir32.get(i + 2);
                        Int rightValReg = GET_Da(code.ir32.get(i + 1));
                        Int leftValReg = GET_Da(code.ir32.get(i + 3));

                        if (leftValReg == GET_Ca(code.ir32.get(i + 4))
                            && rightValReg == GET_Cb(code.ir32.get(i + 4))) {
                            shiftAddresses(1, i + 3);
                            code.ir32.removeAt(i);

                            opcode_instr *buf = OpBuilder::movi(rightIntValue, (_register)rightValReg);
                            code.ir32.get(i) = OpBuilder::loadl((_register)leftValReg, frameAddress);
                            code.ir32.get(i + 1) = buf[0];
                            code.ir32.get(i + 2) = buf[1];
                            code.ir32.get(i + 3) = OpBuilder::lt((_register)leftValReg, (_register)rightValReg);
                        }
                    }
                }
                break;
            }
        }
    }
}

/**
 * [0xb] 11:	movl 0
 * [0xc] 12:	pushobj
 *
 * to ->        pushl 0
 */
void Optimizer::optimizeRedundantLocalPush() {
    CodeHolder &code = currentMethod->data.code;
    for(Int i = 0; i < code.size(); i++) {
        switch (GET_OP(code.ir32.get(i))) {
            case Opcode::MOVI:
            case Opcode::IADD:
            case Opcode::ISUB:
            case Opcode::IMUL:
            case Opcode::IDIV:
            case Opcode::IMOD:
            case Opcode::IADDL:
            case Opcode::ISUBL:
            case Opcode::IMULL:
            case Opcode::IDIVL:
            case Opcode::IMODL:
            case Opcode::ISTORE:
            case Opcode::ISTOREL:
            case Opcode::ISADD:
            case Opcode::CMP:
            case Opcode::MOVN:
            case Opcode::INVOKE_DELEGATE:
                i++;
                break;

            case Opcode::MOVL: {

                if((i + 1) < code.ir32.size()) {
                    if (GET_OP(code.ir32.get(i + 1)) == Opcode::PUSHOBJ) {
                        Int frameAddress = GET_Da(code.ir32.get(i));
                        shiftAddresses(1, i);
                        code.ir32.removeAt(i);

                        code.ir32.get(i) = OpBuilder::pushl(frameAddress);
                    }
                }
                break;
            }
        }
    }
}

/**
 * [0xb] 11:	movl 0
 * [0xc] 12:	popobj
 *
 * to ->        popl 0
 */
void Optimizer::optimizeRedundantLocalPop() {
    CodeHolder &code = currentMethod->data.code;
    for(Int i = 0; i < code.size(); i++) {
        switch (GET_OP(code.ir32.get(i))) {
            case Opcode::MOVI:
            case Opcode::IADD:
            case Opcode::ISUB:
            case Opcode::IMUL:
            case Opcode::IDIV:
            case Opcode::IMOD:
            case Opcode::IADDL:
            case Opcode::ISUBL:
            case Opcode::IMULL:
            case Opcode::IDIVL:
            case Opcode::IMODL:
            case Opcode::ISTORE:
            case Opcode::ISTOREL:
            case Opcode::ISADD:
            case Opcode::CMP:
            case Opcode::MOVN:
            case Opcode::INVOKE_DELEGATE:
                i++;
                break;

            case Opcode::MOVL: {

                if((i + 1) < code.ir32.size()) {
                    if (GET_OP(code.ir32.get(i + 1)) == Opcode::POPOBJ) {
                        Int frameAddress = GET_Da(code.ir32.get(i));
                        shiftAddresses(1, i);
                        code.ir32.removeAt(i);

                        code.ir32.get(i) = OpBuilder::popl(frameAddress);
                    }
                }
                break;
            }
        }
    }
}

void Optimizer::optimize() {
    for(Int i = 0; i < allMethods->size(); i++) {
        currentMethod = allMethods->get(i);
        optimizeRedundantMovr();
        optimizeLocalVarInit();
        optimizeLocalStackPush();
        optimizeLocalVariableIncrement();
        optimizeRedundantIntegerPush();
        optimizeRedundantLocalPush();
        optimizeRedundantLocalPop();
    }
}

/**
 * Before removing the information this function must be called first
 *
 * @param offset
 * @param pc
 */
void Optimizer::shiftAddresses(Int offset, Int pc) {
    CodeHolder &code = currentMethod->data.code;

    // update the line table
    for(unsigned int i = 0; i < currentMethod->data.lineTable.size(); i++) {
        LineData &lineData = currentMethod->data.lineTable.get(i);

        if(lineData.start_pc > pc) {
            lineData.start_pc -= offset;
        }
    }

    // update the branch table
    for(unsigned int i = 0; i < currentMethod->data.branchTable.size(); i++) {
        BranchTable &branch = currentMethod->data.branchTable.get(i);

        if(branch.resolved) {
            if (!insideProtectedCodebase(branch.branch_pc)) {
                switch (GET_OP(code.ir32.get(branch.branch_pc))) {
                    case Opcode::MOVI:
                    case Opcode::IADD:
                    case Opcode::ISUB:
                    case Opcode::IMUL:
                    case Opcode::IDIV:
                    case Opcode::IMOD:
                    case Opcode::IADDL:
                    case Opcode::ISUBL:
                    case Opcode::IMULL:
                    case Opcode::IDIVL:
                    case Opcode::IMODL:
                    case Opcode::ISTORE:
                    case Opcode::ISTOREL:
                    case Opcode::ISADD:
                    case Opcode::CMP:
                        if(code.ir32.get(branch.branch_pc + 1) >= pc)
                            code.ir32.get(branch.branch_pc + 1) = code.ir32.get(branch.branch_pc + 1) - offset;
                        break;
                    case Opcode::JNE:
                        if(GET_Da(code.ir32.get(branch.branch_pc)) >= pc)
                            code.ir32.get(branch.branch_pc) = OpBuilder::jne(GET_Da(code.ir32.get(branch.branch_pc)) - offset);
                        break;
                    case Opcode::JE:
                        if(GET_Da(code.ir32.get(branch.branch_pc)) >= pc)
                            code.ir32.get(branch.branch_pc) = OpBuilder::je(GET_Da(code.ir32.get(branch.branch_pc)) - offset);
                        break;
                    case Opcode::JMP:
                        Int v = GET_Da(code.ir32.get(branch.branch_pc));
                        if(GET_Da(code.ir32.get(branch.branch_pc)) >= pc)
                            code.ir32.get(branch.branch_pc) = OpBuilder::jmp(GET_Da(code.ir32.get(branch.branch_pc)) - offset);
                        break;
                }
            }
        }

        if(pc <= branch.branch_pc)
            branch.branch_pc -= offset;
    }

    // update skips
    for(Int i = 0; i < code.size(); i++) {
        switch (GET_OP(code.ir32.get(i))) {
            case Opcode::MOVI:
            case Opcode::IADD:
            case Opcode::ISUB:
            case Opcode::IMUL:
            case Opcode::IDIV:
            case Opcode::IMOD:
            case Opcode::IADDL:
            case Opcode::ISUBL:
            case Opcode::IMULL:
            case Opcode::IDIVL:
            case Opcode::IMODL:
            case Opcode::ISTORE:
            case Opcode::ISTOREL:
            case Opcode::ISADD:
            case Opcode::CMP:
            case Opcode::MOVN:
            case Opcode::INVOKE_DELEGATE:
                i++;
                break;
            case Opcode::SKIP:
                if((GET_Da(code.ir32.get(i)) + i) > pc)
                    code.ir32.get(i) = OpBuilder::skip(GET_Da(code.ir32.get(i)) - offset);
                break;
            case Opcode::SKPE:
                if((GET_Cb(code.ir32.get(i)) + i) > pc)
                    code.ir32.get(i) = OpBuilder::skipife((_register)GET_Ca(code.ir32.get(i)), GET_Cb(code.ir32.get(i)) - offset);
                break;
            case Opcode::SKNE:
                if((GET_Cb(code.ir32.get(i)) + i) > pc)
                    code.ir32.get(i) = OpBuilder::skipifne((_register)GET_Ca(code.ir32.get(i)), GET_Cb(code.ir32.get(i)) - offset);
                break;
        }
    }

    for(Int i = 0; i < currentMethod->data.tryCatchTable.size(); i++) {
        TryCatchData &tryCatchData = currentMethod->data.tryCatchTable.get(i);
        if(pc <= tryCatchData.try_start_pc)
            tryCatchData.try_start_pc -= offset;
        if(pc <= tryCatchData.try_end_pc)
            tryCatchData.try_end_pc -= offset;
        if(pc <= tryCatchData.block_start_pc)
            tryCatchData.block_start_pc -= offset;
        if(pc <= tryCatchData.block_end_pc)
            tryCatchData.block_end_pc -= offset;

        for(Int j = 0; j < tryCatchData.catchTable.size(); j++) {
            CatchData &catchData = tryCatchData.catchTable.get(j);

            if(pc <= catchData.handler_pc) {
                catchData.handler_pc -= offset;
            }
        }

        if(tryCatchData.finallyData != NULL) {
            if(pc <= tryCatchData.finallyData->start_pc)
                tryCatchData.finallyData->start_pc -= offset;
            if(pc <= tryCatchData.finallyData->end_pc)
                tryCatchData.finallyData->end_pc -= offset;
        }
    }
}

bool Optimizer::insideProtectedCodebase(Int pc) {
    for(unsigned int i = 0; i < currentMethod->data.protectedCodeTable.size(); i++) {
        AsmData &asmData = currentMethod->data.protectedCodeTable.get(i);
        if(pc >= asmData.start_pc && pc < asmData.end_pc) {
            return true;
        }
    }
    return false;
}

bool Optimizer::isRegisterUsed(_register reg, Int start_pc) {
    CodeHolder &code = currentMethod->data.code;

    for(Int i = start_pc; i < code.size(); i++) {
        switch(GET_OP(code.ir32.get(i))) {
            case Opcode::IADDL:
            case Opcode::ISUBL:
            case Opcode::IMULL:
            case Opcode::IDIVL:
            case Opcode::IMODL:
            case Opcode::ISTORE:
            case Opcode::ISTOREL:
            case Opcode::ISADD:
            case Opcode::MOVN:
            case Opcode::INVOKE_DELEGATE:
                i++;
                break;

            case Opcode::IADD:
            case Opcode::ISUB:
            case Opcode::IMUL:
            case Opcode::IDIV:
            case Opcode::IMOD:
            case Opcode::MOVI:
                if(GET_Da(code.ir32.get(i)) == reg)
                    return false;
                i++;
                break;

            case Opcode::NEWARRAY:
            case Opcode::RSTORE:
            case Opcode::PUT:
            case Opcode::PUTC:
            case Opcode::CHECKLEN:
            case Opcode::SLEEP:
            case Opcode::MOVND:
            case Opcode::NEWOBJARRAY:
            case Opcode::RETURNVAL:
            case Opcode::CALLD:
                if(GET_Da(code.ir32.get(i)) == reg)
                    return true;
                break;

            case Opcode::CMP:
                if(GET_Da(code.ir32.get(i)) == reg)
                    return true;
                i++;
                break;

            case Opcode::INC:
            case Opcode::DEC:
            case Opcode::CAST:
            case Opcode::SIZEOF:
            case Opcode::LOADPC:
            case Opcode::LOADVAL:
            case Opcode::CHECKNULL:
            case Opcode::ITEST:
            case Opcode::GET:
                if(GET_Da(code.ir32.get(i)) == reg)
                    return false;
                break;

            case Opcode::MOV8:
            case Opcode::MOV16:
            case Opcode::MOV32:
            case Opcode::MOV64:
            case Opcode::MOVU8:
            case Opcode::MOVU16:
            case Opcode::MOVU32:
            case Opcode::MOVU64:
            case Opcode::MOVR:
            case Opcode::IALOAD:
            case Opcode::NOT:
                if(GET_Ca(code.ir32.get(i)) == reg)
                    return false;
                if(GET_Cb(code.ir32.get(i)) == reg)
                    return true;
                break;

            case Opcode::SKPE:
            case Opcode::SKNE:
            case Opcode::NEWCLASSARRAY:
            case Opcode::ADDL:
            case Opcode::SUBL:
            case Opcode::MULL:
            case Opcode::DIVL:
            case Opcode::MODL:
            case Opcode::LOADL:
            case Opcode::SMOVR:
            case Opcode::ANDL:
            case Opcode::ORL:
            case Opcode::XORL:
            case Opcode::SMOVR_2:
                if(GET_Ca(code.ir32.get(i)) == reg)
                    return true;
                break;

            case Opcode::SMOV:
            case Opcode::LDC:
                if(GET_Ca(code.ir32.get(i)) == reg)
                    return false;
                break;

            case Opcode::TEST:
            case Opcode::TNE:
            case Opcode::AND:
            case Opcode::UAND:
            case Opcode::OR:
            case Opcode::XOR:
            case Opcode::RMOV:
            case Opcode::NEG:
            case Opcode::LT:
            case Opcode::GT:
            case Opcode::LTE:
            case Opcode::GTE:
                if(GET_Ca(code.ir32.get(i)) == reg
                    || GET_Cb(code.ir32.get(i)) == reg)
                    return true;
                break;

            case Opcode::ADD:
            case Opcode::SUB:
            case Opcode::MUL:
            case Opcode::DIV:
            case Opcode::MOD:
            case Opcode::EXP:
            case Opcode::SHL:
            case Opcode::SHR:
                if(GET_Bc(code.ir32.get(i)) == reg)
                    return false;
                if(GET_Ba(code.ir32.get(i)) == reg
                    || GET_Bb(code.ir32.get(i)) == reg)
                    return true;
                break;
        }

    }

    return false;
}
