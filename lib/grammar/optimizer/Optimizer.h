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
    }

    void optimize();

    Int optimizedOpcodes;
private:
    List<Method*>* allMethods;
    Method *currentMethod;
    opcode_instr tmpInstr;

    void shiftAddresses(Int offset, Int pc);
    bool insideProtectedCodebase(Int pc);

    void optimizeRedundantMovr();
    void optimizeLocalStackPush();
    void optimizeLocalVarInit();
    void optimizeLocalVariableIncrement();
    void optimizeRedundantIntegerPush();
    bool isRegisterUsed(_register reg, Int start_pc);
};


#endif //SHARP_OPTIMIZER_H
