//
// Created by BNunnally on 5/25/2020.
//

#ifndef SHARP_OPTIMIZER_H
#define SHARP_OPTIMIZER_H

#include "../backend/Compiler.h"

class Optimizer {
public:
    Optimizer(List<Method*> *allMethods)
    :
        allMethods(allMethods)
    {
        preCodebaseSize = 0;
        postCodebaseSize = 0;
        fragmentsFound = 0;
    }

    void optimize();

    uInt preCodebaseSize;
    uInt postCodebaseSize;
private:
    List<Method*>* allMethods;
    Method *currentMethod;
    opcode_instr tmpInstr;
    uInt fragmentsFound;
    Int netBenefit;

    void shiftAddresses(Int offset, Int pc);
    bool insideProtectedCodebase(Int pc);
    Int getSkippedProtectedCodebasePc(Int pc);

    void optimizeRedundantMovr();
    void optimizeLocalStackPush();
    void optimizeLocalVarInit();
    void optimizeLocalVariableIncrement();
    void optimizeRedundantIntegerPush();
    void optimizeRedundantLocalPush();
    void optimizeRedundantLocalPop();
    void optimizeEbxReturn();
    void printResults();
    void optimizeNullCheck();
    void optimizeNot();
    void optimizeRedundantEbxStore();
    void optimizeTNE();
    void optimizeNumericStore();
    void optimizeUnNessicaryLengthCheck();
    void optimizeUnnessicaryCMTMov();
    void optimizeUnnessicaryLocalIntPop();
    void optimizeEmptyCall();
    void optimizeIntReturn();
    void optimizeCmtReturn();
    bool isRegisterUsed(_register reg, Int start_pc);
};


#endif //SHARP_OPTIMIZER_H
