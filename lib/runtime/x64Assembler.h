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
    X86Mem getMemPtr(int64_t addr) override {
        return x86::qword_ptr(x86::rax, addr);
    }
};


#endif //SHARP_X64ASSEMBLER_H
