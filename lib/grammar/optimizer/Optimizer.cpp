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
        if(insideProtectedCodebase(i)) {
            i = getSkippedProtectedCodebasePc(i) - 1;
            continue;
        }

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
        if(insideProtectedCodebase(i)) {
            i = getSkippedProtectedCodebasePc(i) - 1;
            continue;
        }

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
                    if(i > 0 && (GET_OP(code.ir32.get(i - 1)) == Opcode::SKIP
                        || GET_OP(code.ir32.get(i - 1)) == Opcode::SKPE
                        || GET_OP(code.ir32.get(i - 1)) == Opcode::SKNE)) {
                        i++;
                        continue;
                    }
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
        if(insideProtectedCodebase(i)) {
            i = getSkippedProtectedCodebasePc(i) - 1;
            continue;
        }

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
        if(insideProtectedCodebase(i)) {
            i = getSkippedProtectedCodebasePc(i) - 1;
            continue;
        }

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
        if(insideProtectedCodebase(i)) {
            i = getSkippedProtectedCodebasePc(i) - 1;
            continue;
        }

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
        if(insideProtectedCodebase(i)) {
            i = getSkippedProtectedCodebasePc(i) - 1;
            continue;
        }

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
 *
 */
void Optimizer::optimizeRedundantLocalPop() {
    CodeHolder &code = currentMethod->data.code;
    for(Int i = 0; i < code.size(); i++) {
        if(insideProtectedCodebase(i)) {
            i = getSkippedProtectedCodebasePc(i) - 1;
            continue;
        }

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

/**
 * [0xb] 11:	test ecx, ebx
 * [0xc] 12:	movr ebx, cmt
 * [0xd] 13:    return_val ebx
 *
 * to ->        test ecx, ebx
 *              return_val cmt
 *
 * result 94
 * 0.203445 % benefit
 */
void Optimizer::optimizeEbxReturn() {
    CodeHolder &code = currentMethod->data.code;

    for(Int i = 0; i < code.size(); i++) {
        if(insideProtectedCodebase(i)) {
            i = getSkippedProtectedCodebasePc(i) - 1;
            continue;
        }

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

            case Opcode::TEST: {

                if((i + 2) < code.ir32.size()) {
                    if (GET_OP(code.ir32.get(i + 1)) == Opcode::MOVR
                        && GET_Cb(code.ir32.get(i+1)) == CMT) {
                        Int receiverReg = GET_Ca(code.ir32.get(i+1));

                        if(GET_OP(code.ir32.get(i + 2)) == Opcode::RETURNVAL
                            && GET_Da(code.ir32.get(i + 2)) == receiverReg) {
                            i++;
                            shiftAddresses(1, i);
                            code.ir32.removeAt(i);

                            code.ir32.get(i) = OpBuilder::returnValue(CMT);
                        }
                    }
                }
                break;
            }
        }
    }
}

/**
 * [0xb] 11:	pushl 1
 * [0xc] 12:	 pushnull
 * [0xd] 13:     itest ebx
 *
 * to ->        movl 1
 *              checknull ebx
 *
 * result 26
 * 0.0563222 % benefit
 */
void Optimizer::optimizeNullCheck() {
    CodeHolder &code = currentMethod->data.code;

    for(Int i = 0; i < code.size(); i++) {
        if(insideProtectedCodebase(i)) {
            i = getSkippedProtectedCodebasePc(i) - 1;
            continue;
        }

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

            case Opcode::PUSHL: {

                if((i + 2) < code.ir32.size()) {
                    Int localVar = GET_Da(code.ir32.get(i));


                    if (GET_OP(code.ir32.get(i + 1)) == Opcode::PUSHNULL
                        && GET_OP(code.ir32.get(i + 2)) == Opcode::ITEST) {
                        Int receiverReg = GET_Da(code.ir32.get(i+2));
                        code.ir32.get(i) = OpBuilder::movl(localVar);
                        code.ir32.get(++i) = OpBuilder::checkNull((_register)receiverReg);
                        shiftAddresses(1, i);
                        code.ir32.removeAt(++i);
                    }
                }
                break;
            }
        }
    }
}

/**
 * [0xb] 11:	not ebx, ebx
 * [0xc] 12:	movr cmt, ebx
 *
 * to ->        not cmt, ebx
 *
 * fagments found: 18
 * net benefit: 18
 * % of codebase affeced: 0.0389923%
 */
void Optimizer::optimizeNot() {
    CodeHolder &code = currentMethod->data.code;

    for(Int i = 0; i < code.size(); i++) {
        if(insideProtectedCodebase(i)) {
            i = getSkippedProtectedCodebasePc(i) - 1;
            continue;
        }

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

            case Opcode::NOT: {

                if((i + 2) < code.ir32.size()) {
                    Int nottedReg = GET_Ca(code.ir32.get(i));

                    if ((GET_Ca(code.ir32.get(i)) == GET_Cb(code.ir32.get(i)))
                        && GET_OP(code.ir32.get(i + 1)) == Opcode::MOVR
                        && GET_Cb(code.ir32.get(i + 1)) == nottedReg
                        && GET_Ca(code.ir32.get(i + 1)) != nottedReg
                        && !(GET_OP(code.ir32.get(i + 2)) == Opcode::JNE
                            || GET_OP(code.ir32.get(i + 2)) == Opcode::JE
                            || GET_OP(code.ir32.get(i + 2)) == Opcode::JMP)) {
                        Int receiverReg = GET_Ca(code.ir32.get(i+1));
                        shiftAddresses(1, i);
                        code.ir32.removeAt(i);
                        code.ir32.get(i) = OpBuilder::_not((_register)receiverReg, (_register)nottedReg);
                    }
                }
                break;
            }
        }
    }
}

/**
 * [0xb] 11:	rstore ebx
 * [0xc] 12:	movl 0
 * [0xd] 13:	movn #1
 * [0xe] 14:	loadval ebx
 *
 * to ->        movl 0
 *              movn #1
 *
 * fagments found: 46
 * net benefit: 92
 * % of codebase affeced: 0.199294%
 */
void Optimizer::optimizeRedundantEbxStore() {
    CodeHolder &code = currentMethod->data.code;

    for(Int i = 0; i < code.size(); i++) {
        if(insideProtectedCodebase(i)) {
            i = getSkippedProtectedCodebasePc(i) - 1;
            continue;
        }

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

            case Opcode::RSTORE: {

                if((i + 4) < code.ir32.size()) {
                    Int storedReg = GET_Da(code.ir32.get(i));

                    if (GET_OP(code.ir32.get(i + 1)) == Opcode::MOVL
                        && GET_OP(code.ir32.get(i + 2)) == Opcode::MOVN
                        && GET_OP(code.ir32.get(i + 4)) == Opcode::LOADVAL
                        && GET_Da(code.ir32.get(i + 4)) == storedReg) {
                        shiftAddresses(1, i);
                        code.ir32.removeAt(i);
                        i+=3;
                        shiftAddresses(1, i);
                        code.ir32.removeAt(i);
                    }
                }
                break;
            }
        }
    }
}

/**
 * [0xb] 11:	tne ecx, ebx
 * [0xc] 12:	movr ebx, cmt
 * [0xd] 13:	return_val ebx
 *
 * to ->        tne ecx, ebx
 *              return_val cmt
 *
 * fagments found: 90
 * net benefit: 90
 * % of codebase affeced: 0.194961%
 */
void Optimizer::optimizeTNE() {
    CodeHolder &code = currentMethod->data.code;

    for(Int i = 0; i < code.size(); i++) {
        if(insideProtectedCodebase(i)) {
            i = getSkippedProtectedCodebasePc(i) - 1;
            continue;
        }

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

            case Opcode::TNE: {

                if((i + 2) < code.ir32.size()) {
                    if (GET_OP(code.ir32.get(i + 1)) == Opcode::MOVR
                        && GET_Cb(code.ir32.get(i + 1)) == CMT
                        && GET_OP(code.ir32.get(i + 2)) == Opcode::RETURNVAL
                        && GET_Ca(code.ir32.get(i + 1)) == GET_Da(code.ir32.get(i + 2))) {
                        i++;
                        shiftAddresses(1, i);
                        code.ir32.removeAt(i);

                        code.ir32.get(i) = OpBuilder::returnValue(CMT);
                    }
                }
                break;
            }
        }
    }
}

/**
 * [0xb] 11:	movi #128, ebx
 * [0xc] 12:	rstore ebx
 *
 * to ->        istore 128
 *
 * fagments found: 82
 * net benefit: 82
 * % of codebase affeced: 0.177631%
 */
void Optimizer::optimizeNumericStore() {
    CodeHolder &code = currentMethod->data.code;

    for(Int i = 0; i < code.size(); i++) {
        if(insideProtectedCodebase(i)) {
            i = getSkippedProtectedCodebasePc(i) - 1;
            continue;
        }

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
                    Int receivingReg = GET_Da(code.ir32.get(i));

                    if (GET_OP(code.ir32.get(i + 2)) == Opcode::RSTORE
                        && GET_Da(code.ir32.get(i + 2)) == receivingReg) {
                        shiftAddresses(1, i+2);
                        code.ir32.removeAt(i+2);
                        code.ir32.get(i) = OpBuilder::istore(0)[0]; // we only need th first part of the instruction because the second part is already present
                        i += 2;
                    }
                }
                break;
            }
        }
    }
}

/**
 * [0xb] 11:	movi #0, adx
 * [0xc] 12:	chklen adx
 * [0xd] 13:	iaload ebx, adx
 *
 * to ->        movi #0, adx
 *              iaload ebx, adx
 *
 * fagments found: 3364
 * net benefit: 3364
 * % of codebase affeced: 7.28722%
 */
void Optimizer::optimizeUnNessicaryLengthCheck() {
    CodeHolder &code = currentMethod->data.code;

    for(Int i = 0; i < code.size(); i++) {
        if(insideProtectedCodebase(i)) {
            i = getSkippedProtectedCodebasePc(i) - 1;
            continue;
        }

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

                if((i + 3) < code.ir32.size()) {
                    Int adxReg = GET_Da(code.ir32.get(i));

                    if (adxReg == ADX && GET_OP(code.ir32.get(i + 2)) == Opcode::CHECKLEN
                        && GET_Da(code.ir32.get(i + 2)) == adxReg
                        && GET_OP(code.ir32.get(i + 3)) == Opcode::IALOAD
                        && GET_Cb(code.ir32.get(i + 3)) == adxReg) {
                        shiftAddresses(1, i+2);
                        code.ir32.removeAt(i+2);
                        i+=2;
                    }
                }
                break;
            }
        }
    }
}

/**
 * [0xb] 11:	lt ecx, ebx
 * [0xc] 12:	movr ebx, cmt
 * [0xd] 13:	return_val ebx
 *
 * to ->        lt ecx, ebx
 *              return_val ebx
 *
 * fagments found: 269
 * net benefit: 269
 * % of codebase affeced: 0.582718%
 */
void Optimizer::optimizeUnnessicaryCMTMov() {
    CodeHolder &code = currentMethod->data.code;

    for(Int i = 0; i < code.size(); i++) {
        if(insideProtectedCodebase(i)) {
            i = getSkippedProtectedCodebasePc(i) - 1;
            continue;
        }

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

            case Opcode::GT:
            case Opcode::GTE:
            case Opcode::LTE:
            case Opcode::LT: {

                if((i + 2) < code.ir32.size()) {
                    if (GET_OP(code.ir32.get(i + 1)) == Opcode::MOVR
                        && GET_Cb(code.ir32.get(i + 1)) == CMT
                        && GET_OP(code.ir32.get(i + 2)) == Opcode::RETURNVAL
                        && GET_Ca(code.ir32.get(i + 1)) == GET_Da(code.ir32.get(i + 2))) {
                        i++;
                        shiftAddresses(1, i);
                        code.ir32.removeAt(i);

                        code.ir32.get(i) = OpBuilder::returnValue(CMT);
                    }
                }
                break;
            }
        }
    }
}


/**
 * [0xb] 11:	loadval ebx
 * [0xc] 12:	smovr_2 ebx, fp+4
 *
 * to ->        ipopl 4
 *
 * fagments found: 14
 * net benefit: 14
 * % of codebase affeced: 0.0303273%
 */
void Optimizer::optimizeUnnessicaryLocalIntPop() {
    CodeHolder &code = currentMethod->data.code;

    for(Int i = 0; i < code.size(); i++) {
        if(insideProtectedCodebase(i)) {
            i = getSkippedProtectedCodebasePc(i) - 1;
            continue;
        }

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

            case Opcode::LOADVAL: {

                if((i + 1) < code.ir32.size()) {
                    Int recievingReg = GET_Da(code.ir32.get(i));

                    if (GET_OP(code.ir32.get(i + 1)) == Opcode::SMOVR_2
                        && GET_Ca(code.ir32.get(i + 1)) == recievingReg) {
                        Int localVar = GET_Cb(code.ir32.get(i + 1));
                        shiftAddresses(1, i);
                        code.ir32.removeAt(i);
                        code.ir32.get(i) = OpBuilder::ipopl(localVar);
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
        if(currentMethod->fnType != fn_delegate) {
            preCodebaseSize += currentMethod->data.code.size();
            optimizeRedundantMovr();
            optimizeLocalVarInit();
            optimizeLocalStackPush();
            optimizeLocalVariableIncrement();
            optimizeRedundantIntegerPush();
            optimizeRedundantLocalPush();
            optimizeRedundantLocalPop();
            optimizeEbxReturn();
            optimizeNullCheck();
            optimizeNot();
            optimizeRedundantEbxStore();
            optimizeTNE();
            optimizeNumericStore();
            optimizeUnNessicaryLengthCheck();
            optimizeUnnessicaryCMTMov();
            optimizeUnnessicaryLocalIntPop();
            postCodebaseSize += currentMethod->data.code.size();
        }
    }
}

void Optimizer::printResults() {
    cout << "Optimization scout results:\n";
    cout << "fagments found: " << fragmentsFound << endl;
    cout << "net benefit: " << (netBenefit * fragmentsFound) << endl;
    cout << "% of codebase affeced: " << (((double)(netBenefit * fragmentsFound) / (double)postCodebaseSize) * 100) << "%" << endl;
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
                        if(code.ir32.get(branch.branch_pc + 1) > pc)
                            code.ir32.get(branch.branch_pc + 1) = code.ir32.get(branch.branch_pc + 1) - offset;
                        break;
                    case Opcode::JNE:
                        if(GET_Da(code.ir32.get(branch.branch_pc)) > pc)
                            code.ir32.get(branch.branch_pc) = OpBuilder::jne(GET_Da(code.ir32.get(branch.branch_pc)) - offset);
                        break;
                    case Opcode::JE:
                        if(GET_Da(code.ir32.get(branch.branch_pc)) > pc)
                            code.ir32.get(branch.branch_pc) = OpBuilder::je(GET_Da(code.ir32.get(branch.branch_pc)) - offset);
                        break;
                    case Opcode::JMP:
                        Int v = GET_Da(code.ir32.get(branch.branch_pc));
                        if(GET_Da(code.ir32.get(branch.branch_pc)) > pc)
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
                if(pc > i && pc <= (GET_Da(code.ir32.get(i)) + i))
                    code.ir32.get(i) = OpBuilder::skip(GET_Da(code.ir32.get(i)) - offset);
                break;
            case Opcode::SKPE:
                if(pc > i && pc <= (GET_Cb(code.ir32.get(i)) + i))
                    code.ir32.get(i) = OpBuilder::skipife((_register)GET_Ca(code.ir32.get(i)), GET_Cb(code.ir32.get(i)) - offset);
                break;
            case Opcode::SKNE:
                if(pc > i && pc <= (GET_Cb(code.ir32.get(i)) + i))
                    code.ir32.get(i) = OpBuilder::skipifne((_register)GET_Ca(code.ir32.get(i)), GET_Cb(code.ir32.get(i)) - offset);
                break;
        }
    }

    for(Int i = 0; i < currentMethod->data.tryCatchTable.size(); i++) {
        TryCatchData &tryCatchData = currentMethod->data.tryCatchTable.get(i);
        if(pc < tryCatchData.try_start_pc)
            tryCatchData.try_start_pc -= offset;
        if(pc < tryCatchData.try_end_pc)
            tryCatchData.try_end_pc -= offset;
        if(pc < tryCatchData.block_start_pc)
            tryCatchData.block_start_pc -= offset;
        if(pc < tryCatchData.block_end_pc)
            tryCatchData.block_end_pc -= offset;

        for(Int j = 0; j < tryCatchData.catchTable.size(); j++) {
            CatchData &catchData = tryCatchData.catchTable.get(j);

            if(pc < catchData.handler_pc) {
                catchData.handler_pc -= offset;
            }
        }

        if(tryCatchData.finallyData != NULL) {
            if(pc < tryCatchData.finallyData->start_pc)
                tryCatchData.finallyData->start_pc -= offset;
            if(pc < tryCatchData.finallyData->end_pc)
                tryCatchData.finallyData->end_pc -= offset;
        }
    }

    for(unsigned int i = 0; i < currentMethod->data.protectedCodeTable.size(); i++) {
        AsmData &asmData = currentMethod->data.protectedCodeTable.get(i);
        if(pc <= asmData.start_pc) {
            asmData.start_pc -= offset;
        }

        if(pc <= asmData.end_pc) {
            asmData.end_pc -= offset;
        }
    }
}

Int Optimizer::getSkippedProtectedCodebasePc(Int pc) {
    for(unsigned int i = 0; i < currentMethod->data.protectedCodeTable.size(); i++) {
        AsmData &asmData = currentMethod->data.protectedCodeTable.get(i);
        if(pc >= asmData.start_pc && pc < asmData.end_pc) {
            return asmData.end_pc;
        }
    }
    return -1;
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
