//
// Created by braxtonn on 8/23/2019.
//

#ifndef SHARP_X64ASSEMBLER_H
#define SHARP_X64ASSEMBLER_H

#include "JitAssembler.h"

class x64Assembler : public JitAssembler {
public:
    x64Assembler()
    :
            JitAssembler()
    {
        initialize();
    }

private:
    x86::Mem getMemPtr(x86int_t addr) override ;
    x86::Mem getMemPtr(x86::Gp reg, x86int_t addr) override ;
    x86::Mem getMemPtr(x86::Gp reg) override ;
    x86int_t getRegisterSize() override ;
    void initializeRegisters() override ;
};


#endif //SHARP_X64ASSEMBLER_H
