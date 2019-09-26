//
// Created by braxtonn on 8/23/2019.
//

#include "x64Assembler.h"
#ifdef BUILD_JIT

using namespace asmjit;
using namespace asmjit::x86;

void x64Assembler::initializeRegisters() {
    /* x86 Windows standard convention is followed */
    ctx   = rcx;           // registers ctx, value, and tmp are volitle and must be stored on the stack if deemed to be preserved
    ctx32 = ecx;

    tmp       = rax;        // tmp acts as a return value from functions
    tmp32     = eax;
    tmp16     = ax;
    tmp8      = al;
    value     = rdx;        // value acts as the second argument for function params
    fnPtr     = r12;       // registers fnPtr, arg, regPtr, & threadPtr are non volitile and do not have to be saved
    arg       = r13;
    regPtr    = r14;
    threadPtr = r15;

    fnArg3    = r8;        // additional function argument regs
    fnArg4    = r9;

    // stack manip registers
    bp = rbp;
    sp = rsp;

    // floating point calculation regs
    vec0 = xmm0;
    vec1 = xmm1;

    /* Argument registers are as follows for Windows: (ctx, value, fnArg3, fnArg4) */
}

x86::Mem x64Assembler::getMemPtr(x86int_t addr) {
    return x86::qword_ptr(ctx, addr);
}

x86::Mem x64Assembler::getMemPtr(x86::Gp reg, x86int_t addr) {
    return x86::qword_ptr(reg, addr);
}

x86::Mem x64Assembler::getMemPtr(x86::Gp reg) {
    return x86::qword_ptr(reg);
}

x86int_t x64Assembler::getRegisterSize() {
    return 8;
}

#endif
