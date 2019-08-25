//
// Created by braxtonn on 8/23/2019.
//

#include "x64Assembler.h"
#include "../util/jit/asmjit/src/asmjit/asmjit.h"

using namespace asmjit;
using namespace asmjit::x86;

void x64Assembler::initializeRegisters() {
    /* x86 Windows standard convention is followed */
    ctx   = rcx;           // registers ctx, value, and tmp are volitle and must be stored on the stack if deemed to be preserved
    ctx32 = ecx;

    tmp       = rax;
    tmp32     = eax;
    value     = rdx;
    fnPtr     = r12;       // registers fnPtr, arg, regPtr, & threadPtr are non volitile and do not have to be saved
    arg       = r13;
    regPtr    = r14;
    threadPtr = r15;

    // stack manip registers
    bp = rbp;
    sp = rsp;

    // floating point calculation regs
    vec0 = xmm0;
    vec1 = xmm1;
}

X86Mem x64Assembler::getMemPtr(x86int_t addr) {
    return x86::qword_ptr(ctx, addr);
}

X86Mem x64Assembler::getMemPtr(X86Gp reg, x86int_t addr) {
    return x86::qword_ptr(reg, addr);
}

X86Mem x64Assembler::getMemPtr(X86Gp reg) {
    return x86::qword_ptr(reg);
}

x86int_t x64Assembler::getRegisterSize() {
    return 8;
}
