//
// Created by bknun on 5/13/2018.
//

#ifndef SHARP_OPTIMIZER_H
#define SHARP_OPTIMIZER_H


#include "Method.h"

class Optimizer {

public:
    Optimizer()
    {
        unique_addr_lst.init();
    }

    void optimize(Method* method);

    ~Optimizer()
    {
        unique_addr_lst.free();
        assembler=NULL;
    }

    long getOptimizedOpcodes() { return optimizedOpcodes; }

private:
    Method* func;
    long optimizedOpcodes;
    Assembler* assembler;
    List <long> unique_addr_lst;

    void readjustAddresses(unsigned int);
    void optimizeLocalPops();
    void optimizeRedundantMovICall();
    void optimizeRedundantSelfInitilization();
    void optimizeLocalPush();
    void optimizeRedundantMovICall2();
    void optimizeObjectPush();
};



#endif //SHARP_OPTIMIZER_H
